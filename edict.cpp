//=============================================================================
//
// Name:        edict.cpp
// Authors:     James H. Loving
// Description: This file defines supporting functions for edict_main.cpp:
//              parsing command line arguments, starting EDICT, and querying
//              edict for IPv4 and IPv6 connections.
//              This file was split from edict_main.cpp to support unit
//              testing of the contained functions.
//              For additional documentation on functions, see 'edict.hpp'.
//
//=============================================================================

#include "edict.hpp"

struct args_struct parse_args(int arg_count,
                              char **arg_vector)
{
    struct args_struct args;
    
    if (arg_count > 1)
    {
        args.command = arg_vector[1];
    }
    else
    {
        args.command = "help";
    }

    if (args.command == "start")
    {
        if (arg_count != 2)
        {
            args.command = "invalid";
        }
    }
    else if (args.command == "query")
    {
        if (arg_count == 6)
        {
            args.query_timestamp = arg_vector[2];
            args.query_version = arg_vector[3];
            args.query_metadata = arg_vector[4];   
            args.print_format = arg_vector[5];
        }
        else
        {
            args.command = "invalid";
        }
    }

    return args;
}

void print_help()
{
    std::cout << "Usage: edict <command> <subcommands>\n\n";
    std::cout << "<command> may be one of the following:\n";
    std::cout << std::setw(5) << "" << std::setw(10) << std::left << "start" << "Start EDICT logging.\n";
    std::cout << std::setw(5) << "" << std::setw(10) << std::left << "query" << "Query EDICT's logs. Usage: edict query <timestamp> <version> <metadata> <format>\n";
    std::cout << std::setw(20) << "" << std::setw(15) << std::left << "<timestamp>" << "ISO 8601-formatted timestamp of connection (in UTC)\n";
    std::cout << std::setw(20) << "" << std::setw(15) << std::left << "<version>" << "IP version: 'v4' or 'v6' (no quotes)\n";
    std::cout << std::setw(20) << "" << std::setw(15) << std::left << "<metadata>" << "Source port (for IPv4) or source IPv6 address\n";
    std::cout << std::setw(20) << "" << std::setw(15) << std::left << "<format>" << "Format for printing, 'plain' or 'xml' (no quotes)\n";
    std::cout << "\n";

    time_t now;
    time(&now);
    char time_buf[sizeof("1111-11-11T11:11:11Z")];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now)); 
    std::cout << "Current UTC time: " << time_buf << "\n";
}

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

static int log_packet(struct nflog_data *ldata,
                      conn_log *connections,
                      device_log *devices)
{
    struct nfulnl_msg_packet_hdr *ph = nflog_get_msg_packet_hdr(ldata);
    char *payload;
    nflog_get_payload(ldata, &payload); 

    struct nfulnl_msg_packet_hw *packet_hw = nflog_get_packet_hw(ldata);

    // TODO: reinterpret_cast
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
    if (!devices->should_log(mac_address))
    {
        return 0;
    }
  
    // if the device is new, add it to device_log
    if (!(devices->count(mac_address)))
    {
        std::string make_model = "make_model"; // TODO: get make_model from wifi code
        devices->add_device(mac_address, make_model);
        printf("\n*** New Device! ***\n");
    }

    // process IPv4 packets
    if (packet_header_v4->version == 4)
    {
        std::string source_address, dest_address;
        uint16_t source_port, dest_port;

        // get the source IPv4 address
        char str[INET_ADDRSTRLEN];
        // TODO: reinterpret_cast
        struct in_addr *addr = (struct in_addr*)&(packet_header_v4->saddr);
        inet_ntop(AF_INET, addr, str, INET_ADDRSTRLEN);
        source_address = str;

        // get the destination IPv4 address
        // TODO: reinterpret_cast
        addr = (struct in_addr*)&(packet_header_v4->daddr);
        inet_ntop(AF_INET, addr, str, INET_ADDRSTRLEN);
        dest_address = str;

        // get the source & destination TCP/UDP ports
        int off_tl = packet_header_v4->ihl << 2;
        // TODO: reinterpret_cast
        char *tl = (char *) packet_header_v4 + off_tl;
        // TODO: reinterpret_cast
        struct tcphdr *tcphdr = (struct tcphdr*) tl;
        source_port = ntohs(tcphdr->th_sport);
        dest_port = ntohs(tcphdr->th_dport);

        pprint_packet(mac_address, source_address, dest_address, source_port, dest_port);

        connections->add_ipv4(mac_address, source_port);
    }

    // process IPv6 packets
    else if (packet_header_v4->version == 6)
    {
        std::string source_address, dest_address;
        uint16_t source_port, dest_port;

        // TODO: reinterpret_cast
        struct ip6_hdr *packet_header_v6 = (struct ip6_hdr*) payload;

        // get the source IPv6 address
        char str[INET6_ADDRSTRLEN];
        struct in6_addr *addr = &packet_header_v6->ip6_src;
        inet_ntop(AF_INET6, addr, str, INET6_ADDRSTRLEN);
        source_address = str;

        // get the destination IPv6 address
        // TODO reinterpret_cast
        addr = (struct in6_addr*)&(packet_header_v6->ip6_dst);
        inet_ntop(AF_INET6, addr, str, INET6_ADDRSTRLEN);
        dest_address = str;

        pprint_packet(mac_address, source_address, dest_address, 0, 0);

        connections->add_ipv6(mac_address, source_address);
    }

    std::cout << "\n";
    return 0;
}

static int cb(struct nflog_g_handle *gh,
              struct nfgenmsg *nfmsg,
              struct nflog_data *nfa,
              void *data)
{
    // grab the passed logs via the log_struct pointer
    struct log_struct *ls = reinterpret_cast<struct log_struct *>(data);

    log_packet(nfa, ls->connections, ls->devices);

    return 0;
}

int start_edict(conn_log connections,
                device_log devices)
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
        throw std::runtime_error("start_edict: error during nflog_open()");
    }
    
    printf("unbinding existing nf_log handler for AF_INET (if any)\n");
    if (nflog_unbind_pf(h, AF_INET) < 0)
    {
        throw std::runtime_error("start_edict: error during nflog_unbind_pf()");
    }

    printf("binding nfnetlink_log to AF_INET\n");
    if (nflog_bind_pf(h, AF_INET) < 0)
    {
        throw std::runtime_error("start_edict: error during nflog_bind_pf()");
    }

    printf("binding this socket to group 2\n");
    qh = nflog_bind_group(h, 2);
    if (!qh)
    {
        throw std::runtime_error("start_edict: no handle for group 2");
    }

    printf("setting copy_packet mode\n");
    if (nflog_set_mode(qh, NFULNL_COPY_PACKET, 0xffff) < 0)
    {
        throw std::runtime_error("start_edict: can't set packet copy mode");
    }

    fd_nflog = nflog_fd(h);

    // register callback, and pass logs to callback via void* ptr to log_struct
    printf("registering callback for group 2\n");
    struct log_struct ls;
    ls.connections = &connections;
    ls.devices = &devices;
    nflog_callback_register(qh, &cb, reinterpret_cast<void*>(&ls));

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

std::map<std::string, struct device_log_entry> query_edict(conn_log connections,
                                                           device_log devices,
                                                           struct args_struct args)
{
    time_t timestamp;
    struct tm t{};

    std::map<std::string, struct device_log_entry> devices_cache = devices.get_devices();

    if (strptime(args.query_timestamp.c_str(), "%Y-%m-%dT%H:%M:%SZ", &t) != NULL)
    {
        timestamp = mktime(&t) + (&t)->tm_gmtoff;
    }	
    else
    {
        throw std::invalid_argument("query_edict: invalid timestamp");
    }

    if (args.query_version == "v4")
    {
        uint16_t source_port;
        try
        {
            source_port = static_cast<uint16_t>(std::stoi(args.query_metadata));
        }
        catch (int e)
        {
            throw std::invalid_argument("query_edict: invalid IPv4 source port");
        }

        return check_ipv4(connections, devices_cache, timestamp, source_port);
    }
    else if (args.query_version == "v6")
    {
        if (!connections.valid_ipv6(args.query_metadata))
        {
            throw std::invalid_argument("query_edict: invalid IPv6 source address");
        }

        return check_ipv6(connections, devices_cache, timestamp, args.query_metadata);
    }
    else
    {
        throw std::invalid_argument("query_edict: invalid protocol");
    }
}

std::map<std::string, struct device_log_entry> check_ipv4(conn_log connections,
                                                          std::map<std::string, struct device_log_entry> devices,
                                                          time_t timestamp,
                                                          uint16_t source_port)
{
    std::map<std::string, struct device_log_entry> has_ipv4;
    
    for (std::map<std::string, struct device_log_entry>::iterator it=devices.begin(); it != devices.end(); ++it)
    {
        // TODO: potential mismatch between MAC formats between device log and conn log
        // appearance: device log stores w/out colons, conn log stores w/
        std::string mac = it->first;
        
        if (connections.has_ipv4(mac, source_port, timestamp))
        {
            has_ipv4.insert(std::pair<std::string, struct device_log_entry>(mac, devices[mac]));
        }
    } 

    return has_ipv4;
}

std::map<std::string, struct device_log_entry> check_ipv6(conn_log connections,
                                                          std::map<std::string, struct device_log_entry> devices,
                                                          time_t timestamp,
                                                          std::string ipv6_address)
{
    std::map<std::string, struct device_log_entry> has_ipv6;

    for (std::map<std::string, struct device_log_entry>::iterator it=devices.begin(); it != devices.end(); ++it)
    {
        std::string mac = it->first;
        
        if (connections.has_ipv6(mac, ipv6_address, timestamp))
        {
            has_ipv6.insert(std::pair<std::string, struct device_log_entry>(mac, devices[mac]));
        }
    } 

    return has_ipv6;
}

void print_results(std::map<std::string, struct device_log_entry> results,
                   std::string format)
{
    if (format == "plain")
    {
        if (results.empty())
        {
            std::cout << "No match.\n";
        } 
        else
        {
            std::cout << "[RESULTS START]\n";
            for (std::map<std::string, struct device_log_entry>::iterator it=results.begin(); it != results.end(); ++it)
            {
                char time_buf[sizeof("1111-11-11T11:11:11Z")];
                strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&it->second.first_seen));
                std::cout << "Match:"
                          << "\n  Make & model: " << it->second.make_model
                          << "\n  MAC address: " << it->first
                          << "\n  First seen: " << time_buf
                          << "\n";
            }
            std::cout << "[RESULTS END]\n";
        }
    }
    // else if (format == "xml")
    else
    {
        throw std::invalid_argument("print_results: invalid format");
    }
}
