#include <arpa/inet.h>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sstream>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <unordered_map>

#include "../libs/conn_log/conn_log.cpp"
#include "../libs/device_log/device_log.cpp"

extern "C"
{
    #include <libnetfilter_log/libnetfilter_log.h>
}

conn_log connections;

device_log devices;

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

    struct iphdr *packet_header_v4 = (struct iphdr*) payload;

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

    if (!(devices.count(mac_address)))
    {
        std::string make_model = "make_model"; // TODO: get make_model from wifi code
        devices.add_device(mac_address, make_model);
        printf("\n*** New Device! ***\n");
    }

    if (packet_header_v4->version == 4)
    {
        char str[INET_ADDRSTRLEN];
        struct in_addr *addr = (struct in_addr*)&(packet_header_v4->saddr);
        inet_ntop(AF_INET, addr, str, INET_ADDRSTRLEN);
        std::cout << std::setw(15) << std::left << str << " > ";

        addr = (struct in_addr*)&(packet_header_v4->daddr);
        inet_ntop(AF_INET, addr, str, INET_ADDRSTRLEN);
        std::cout << std::setw(15) << std::left << str << " ";

        printf("IHL:%u ", packet_header_v4->ihl);
        std::cout << "&PH:" << packet_header_v4 << " ";

        int off_tl = packet_header_v4->ihl << 2;
        char *tl = (char *) packet_header_v4 + off_tl;
        
        struct tcphdr *tcphdr = (struct tcphdr*) tl;

        std::cout << "\n  S_Port:";
        std::cout << std::setw(5) << std::left << ntohs(tcphdr->th_sport) << ",";
        std::cout << "\n  ";
        std::cout << "D_Port:" << std::setw(5) << std::left << ntohs(tcphdr->th_dport) << " ";

        connections.add_ipv4(mac_address, ntohs(tcphdr->th_sport));
    }
    else if (packet_header_v4->version == 6)
    {
        struct ip6_hdr *packet_header_v6 = (struct ip6_hdr*) payload;

        char str[INET6_ADDRSTRLEN];
        struct in6_addr *addr = &packet_header_v6->ip6_src;
        inet_ntop(AF_INET6, addr, str, INET6_ADDRSTRLEN);

        std::cout << str << " > ";

        addr = (struct in6_addr*)&(packet_header_v6->ip6_dst);
        inet_ntop(AF_INET6, addr, str, INET6_ADDRSTRLEN);
        std::cout << str << " ";

        // TODO: add_ipv6(mac_address, source_address)
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
    int rv, fd_nflog;
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
