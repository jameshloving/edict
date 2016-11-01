#include <arpa/inet.h>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <unordered_map>

#include "../libs/ip_log/ip_log.cpp"

extern "C"
{
    #include <libnetfilter_log/libnetfilter_log.h>
}

struct iphdr_v4
{
    // little Endian (reverse ihl & version if big endian)
    __u8    ihl:4,
        version:4;
    __u8    tos;
    __u16   tot_len;
    __u16   id;
    __u16   frag_off;
    __u8    ttl;
    __u8    protocol;
    __u16   check;
    __u32   saddr;
    __u32   daddr;
};

struct iphdr_v6
{
    __u32   version:4,
        traffic_class:8,
        flow_label:20;
    __u16   payload_len;
    __u8    next_hdr;
    __u8    hop_limit;
    unsigned char saddr[16];
    unsigned char daddr[16];    
};

struct device_log_entry
{
    std::string make_model;
    time_t first_seen;      
};

struct region
{
    ip_log conn_log;
};
struct region *shm;

std::unordered_map<std::string, struct device_log_entry> device_log;

std::string hexStr_to_charStr(std::string hexStr)
{
    std::stringstream ss;
    std::string charStr;
    unsigned int buffer;
    int offset = 0;

    hexStr = hexStr.substr(2, hexStr.length()-2);

    while (offset < hexStr.length())
    {
        ss.clear();
        ss << std::hex << hexStr.substr(offset, 2);
        ss >> buffer;
        charStr.push_back(static_cast<unsigned char>(buffer));
        offset += 2;
    }
    
    return charStr;
}

static int print_pkt(struct nflog_data *ldata)
{
    struct nfulnl_msg_packet_hdr *ph = nflog_get_msg_packet_hdr(ldata);
    char *payload;
    nflog_get_payload(ldata, &payload); 

    struct nfulnl_msg_packet_hw *packet_hw = nflog_get_packet_hw(ldata);
    struct iphdr_v4 *packet_header_v4 = (struct iphdr_v4*)payload;
    struct iphdr_v6 *packet_header_v6;

    //std::cout << "Time:" << time(nullptr) << " ";

    std::string mac_address = "";

    if (packet_hw)
    {
        char temp[2];
        
        for (int i = 0; i < ntohs(packet_hw->hw_addrlen); ++i)
        {
            sprintf(temp, "%02x", packet_hw->hw_addr[i]);
            mac_address += temp;
        }
    }

    std::cout << "S_MAC:" << std::hex << mac_address << std::dec << " ";
    printf("V:%u ", packet_header_v4->version);

    if (!(device_log.count(mac_address)))
    {
        struct device_log_entry entry;
        entry.make_model = ""; // TODO: get make_model from wifi code
        entry.first_seen = time(nullptr);
        device_log.insert(std::pair<std::string, struct device_log_entry>(mac_address, entry));
        //printf("\n*** New Device! ***\n");
    }

    if (packet_header_v4->version == 4)
    {
        char str[INET_ADDRSTRLEN];
        struct in_addr *addr = (struct in_addr*)&(packet_header_v4->saddr);
        inet_ntop(AF_INET, addr, str, INET_ADDRSTRLEN);
        std::cout << "S_IPv4:" << std::setw(15) << std::left << str << " ";

        printf("IHL:%u ", packet_header_v4->ihl);
        uint16_t *source_port = (uint16_t*)(packet_header_v4 + (packet_header_v4->ihl * 4) + -4);  
        std::cout << "S_Port:" << std::setw(5) << std::left << (*source_port) << " ";
        //std::cout << "S_Port:" << std::setw(5) << std::left << ntohs(*source_port) << " ";

        addr = (struct in_addr*)&(packet_header_v4->daddr);
        inet_ntop(AF_INET, addr, str, INET_ADDRSTRLEN);
        std::cout << "D_IPv4:" << std::setw(15) << std::left << str << " ";

        uint16_t *dest_port = (uint16_t*)(packet_header_v4 + (packet_header_v4->ihl * 4) + 2);
        std::cout << "D_Port:" << std::setw(5) << std::left << (*dest_port) << " ";
        //std::cout << "D_Port:" << std::setw(5) << std::left << ntohs(*dest_port) << " ";

        //shm->conn_log.add_ipv4_connection(mac_address, *source_port);
    }
    else if (packet_header_v4->version == 6)
    {
        // TODO fix this entire section
        packet_header_v6 = (struct iphdr_v6*)payload;
        std::string source_address(packet_header_v6->saddr, packet_header_v6->saddr + sizeof packet_header_v6->saddr / sizeof packet_header_v6->saddr[0]);
        //printf("S_IPv6:%s ", source_address); // TODO: fix this
        std::cout << "S_IPv6:" << source_address << " ";
        __u16 *source_port = (__u16*)(packet_header_v4 + (packet_header_v4->ihl * 4));  
        printf("S_Port:%u ", *source_port);
        // TODO: log.add_ipv6_connection(string_mac_address, string_ipv6_address)
    }

    std::cout << "\n";
    return 0;
}

static int cb(struct nflog_g_handle *gh, struct nfgenmsg *nfmsg,
        struct nflog_data *nfa, void *data)
{
    print_pkt(nfa);
    return 0;
}

int main(int argc, char **argv)
{
    struct nflog_handle *h;
    struct nflog_g_handle *qh;
    struct nflog_g_handle *qh100;
    int rv, fd_nflog, fd_shm;
    char buf[4096];
    char *payload;

    // setup NFLog
    h = nflog_open();
    if (!h)
    {
        fprintf(stderr, "error during nflog_open()\n");
        exit(1);
    }
    
    printf("unbinding existing nf_log handler for AF_INET (if any)\n");
    if (nflog_unbind_pf(h, AF_INET) < 0)
    {
        fprintf(stderr, "error nflog_unbind_pf()\n");
        exit(1);
    }

    printf("binding nfnetlink_log to AF_INET\n");
    if (nflog_bind_pf(h, AF_INET) < 0)
    {
        fprintf(stderr, "error during nflog_bind_pf()\n");
        exit(1);
    }
    printf("binding this socket to group 2\n");
    qh = nflog_bind_group(h, 2);
    if (!qh)
    {
        fprintf(stderr, "no handle for group 2\n");
        exit(1);
    }

    printf("setting copy_packet mode\n");
    if (nflog_set_mode(qh, NFULNL_COPY_PACKET, 0xffff) < 0)
    {
        fprintf(stderr, "can't set packet copy mode\n");
        exit(1);
    }

    fd_nflog = nflog_fd(h);

    printf("registering callback for group 2\n");
    nflog_callback_register(qh, &cb, NULL);

    // Create shared memory object and set its size
    fd_shm = shm_open("/myregion", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd_shm == -1)
    {
        fprintf(stderr, "can't shm_open /myregion\n");
        exit(1);
    }

    if (ftruncate(fd_shm, sizeof(struct region)) == -1)
    {
        fprintf(stderr, "can't ftruncate fd\n");
        exit(1);
    }

    shm = static_cast<struct region *>(mmap(NULL, sizeof(struct region),
                                            PROT_READ | PROT_WRITE, MAP_SHARED,
                                            fd_shm, 0));

    shm = new struct region();

    if (shm == MAP_FAILED)
    {
        fprintf(stderr, "can't mmap shm\n");
        exit(1);
    }

    // process packets as they are received
    printf("going into main loop\n");
    while ((rv = recv(fd_nflog, buf, sizeof(buf), 0)) && rv >= 0)
    {
        nflog_handle_packet(h, buf, rv);
    }

    printf("unbinding from group 2\n");
    nflog_unbind_group(qh);

#ifdef INSANE
    /* normally, applications SHOULD NOT issue this command,
     * since it detaches other programs/sockets from AF_INET, too ! */
    printf("unbinding from AF_INET\n");
    nflog_unbind_pf(h, AF_INET);
#endif

    printf("closing handle\n");
    nflog_close(h);

    return EXIT_SUCCESS;
}
