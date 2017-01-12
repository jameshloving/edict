//=============================================================================
//
// Name:        edict.cpp
// Authors:     James H. Loving
// Description: This file defines the conn_log class, used to log IPv4 and
//              IPv6 communications on a local Bloomd server.
//
//=============================================================================

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

/**
    Pretty print a packet's metadata to stdout.

    \param mac_address String-encoded MAC address
    \param source_address String-encoded source IP address (v4 or v6)
    \param dest_address String-encoded destination IP address (v4 or v6)
    \param source_port uint16_t-encoded source TCP port
    \param dest_port uint16_t-encoded destination TCP port
*/
void pprint_packet(std::string mac_address,
                  std::string source_address,
                  std::string dest_address,
                  uint16_t source_port,
                  uint16_t dest_port)
{
    std::cout << "\"" << mac_address << "\"\n"
              << "Source: "
              << std::setw(8) << std::left << source_port
              << std::setw(80) << std::left << source_address
              << "\nDest:   "
              << std::setw(8) << std::left << dest_port
              << std::setw(80) << std::left << dest_address
              << "\n";
}

/**
    Log a packet into the device_log and conn_log.

    \param ldata Pointer to packet's nflog metadata.
*/
static int log_packet(struct nflog_data *ldata)
{
    struct nfulnl_msg_packet_hdr *ph = nflog_get_msg_packet_hdr(ldata);
    char *payload;
    nflog_get_payload(ldata, &payload); 

    struct nfulnl_msg_packet_hw *packet_hw = nflog_get_packet_hw(ldata);

    struct iphdr *packet_header_v4 = (struct iphdr*) payload;

    // get MAC address
    std::string mac_address = "";
    if (packet_hw)
    {
        char temp[2];
        
        // format MAC address as std::string of char-encoded hex values
        for (int i = 0; i < ntohs(packet_hw->hw_addrlen); ++i)
        {
            sprintf(temp, "%02x", packet_hw->hw_addr[i]);
            mac_address += temp;
        }
    }

    // ignore packets from devices that are on DO-NOT-TRACK list
    if (!devices.should_log(mac_address))
    {
        return 0;
    }
  
    // if the device is new, add it to device_log
    if (!(devices.count(mac_address)))
    {
        std::string make_model = "make_model"; // TODO: get make_model from wifi code
        devices.add_device(mac_address, make_model);
        printf("\n*** New Device! ***\n");
    }

    // process IPv4 packets
    if (packet_header_v4->version == 4)
    {
        std::string source_address, dest_address;
        uint16_t source_port, dest_port;

        // get the source IPv4 address
        char str[INET_ADDRSTRLEN];
        struct in_addr *addr = (struct in_addr*)&(packet_header_v4->saddr);
        inet_ntop(AF_INET, addr, str, INET_ADDRSTRLEN);
        source_address = str;

        // get the destination IPv4 address
        addr = (struct in_addr*)&(packet_header_v4->daddr);
        inet_ntop(AF_INET, addr, str, INET_ADDRSTRLEN);
        dest_address = str;

        // get the source & destination TCP ports
        int off_tl = packet_header_v4->ihl << 2;
        char *tl = (char *) packet_header_v4 + off_tl;
        struct tcphdr *tcphdr = (struct tcphdr*) tl;
        source_port = ntohs(tcphdr->th_sport);
        dest_port = ntohs(tcphdr->th_dport);

        pprint_packet(mac_address, source_address, dest_address, source_port, dest_port);

        connections.add_ipv4(mac_address, source_port);
    }

    // process IPv6 packets
    else if (packet_header_v4->version == 6)
    {
        std::string source_address, dest_address;
        uint16_t source_port, dest_port;

        struct ip6_hdr *packet_header_v6 = (struct ip6_hdr*) payload;

        // get the source IPv6 address
        char str[INET6_ADDRSTRLEN];
        struct in6_addr *addr = &packet_header_v6->ip6_src;
        inet_ntop(AF_INET6, addr, str, INET6_ADDRSTRLEN);
        source_address = str;

        // get the destination IPv6 address
        addr = (struct in6_addr*)&(packet_header_v6->ip6_dst);
        inet_ntop(AF_INET6, addr, str, INET6_ADDRSTRLEN);
        dest_address = str;

        pprint_packet(mac_address, source_address, dest_address, 0, 0);

        connections.add_ipv6(mac_address, source_address);
    }

    std::cout << "\n";
    return 0;
}

/**
    Establish a callback function for the packet's data.
*/
static int cb(struct nflog_g_handle *gh, struct nfgenmsg *nfmsg,
        struct nflog_data *nfa, void *data)
{
    log_packet(nfa);
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
