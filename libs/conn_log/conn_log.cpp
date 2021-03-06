//=============================================================================
//
// Name:        conn_log.cpp
// Authors:     James H. Loving
// Description: This file defines the conn_log class, used to log IPv4 and
//              IPv6 communications on a local Bloomd server.
//
//=============================================================================

#include "conn_log.hpp"

conn_log::conn_log()
{
    // open socket                
    c.conn("localhost", 8673);

    // test connection to Bloomd
    c.send_data("list\n");
    std::string reply = c.receive(1024);
    
    if (reply.substr(0,5) != "START")
    {
        // error
        std::cout << reply;
    }
}

bool conn_log::valid_mac(std::string mac) const
{
    if (mac.length() != 12)
    {
        return false;
    }

    for (short i = 0; i < mac.length(); ++i)
    {
        if (mac[i] < 48 ||
            mac[i] > 102 ||
            (mac[i] > 57 && mac[i] < 97))
        {
            return false;
        }
    }

    return true;
}

bool conn_log::valid_ipv6(std::string ipv6) const
{
    struct sockaddr_in6 sa;

    if (inet_pton(AF_INET6, ipv6.c_str(), &(sa.sin6_addr)))
    {
        return true;
    }
    else
    {
        return false;
    }
}

unsigned int conn_log::get_filter_size()
{
    unsigned int sum = 0;

    c.send_data("list\n");
    std::string reply = c.receive(4096);

    if (reply.substr(0, 5) != "START")
    {
        // error    
        throw std::runtime_error("prune_filters() received invalid info from bloomd");
    }
    
    std::vector<char> reply_v(reply.begin(), reply.end());
    reply_v.push_back('\0');
    for (const char *line = strtok(&reply_v[0], "\n"); line; line = strtok(NULL, "\n")) 
    {
        std::string s = std::string(line);
        if (s.substr(0,5) != "START" && s.substr(0,3) != "END")
        {
            c.send_data("info " + s.substr(0, s.find_first_of(' ')) + "\n");
            std::string info = c.receive(2048);
            unsigned int filter = atoi(info.substr(info.find("storage ")+8, info.length()).c_str());
            sum += filter;
        }
    }

    return sum;
}

void conn_log::prune_filters()
{
    //for (int i = 0; get_filter_size() > MAX_FILTER_SIZE; ++i)
    while (get_filter_size() > MAX_FILTER_SIZE)
    {
        // drop the oldest filter
        c.send_data("list\n");
        std::string reply = c.receive(4096);

        if (reply.substr(0, 5) != "START")
        {
            // error    
            throw std::runtime_error("prune_filters() received invalid info from bloomd");
        }
        
        std::string victim = reply.substr(6, reply.find_first_of('0'));
        victim = victim.substr(0, victim.length() - 1);

        c.send_data("drop " + victim + "\n");
        reply = c.receive(1024);
        std::cout << "size=" << get_filter_size() << "\n";
    }
}

void conn_log::add_ipv4(std::string mac_address,
                        uint16_t port)
{
    // check for invalid MAC addresses
    if (!valid_mac(mac_address))
    {
        throw std::invalid_argument("Invalid conn_log.add_ipv4(mac_address): "
                + mac_address);
    }

    // check for oversized conn_log and prune old filters every 10k connections
    static int i = 0;
    if (i++ % PRUNE_CHECK_FREQ == 0)
    {
        prune_filters();
    }

    // create timestamp
    time_t timestamp = time(nullptr);
    
    // check and create filter based on current timeslot
    c.send_data("create " + std::to_string(timestamp / FILTER_LENGTH) + "\n");
    std::string reply = c.receive(1024);

    // set string(timestamp / FILTER_LENGTH) string(mac_address + port) 
    c.send_data("set " + std::to_string(timestamp / FILTER_LENGTH) + " "
                + mac_address + "|" + std::to_string(port) + "\n");
    reply = c.receive(1024);

    std::cout << "conn_log.add_ipv4(" << timestamp << "," 
              << mac_address << "," << port << ") - "
              << std::to_string(timestamp / FILTER_LENGTH) + ":"
              << mac_address + "|" + std::to_string(port) + "\n";
}

bool conn_log::has_ipv4(std::string mac_address,
                        uint16_t port,
                        time_t timestamp)
{
    if (!valid_mac(mac_address))
    {
        throw std::invalid_argument("Invalid conn_log.has_ipv4(mac_address): "
                + mac_address);
    }

    // check 'correct' filter
    c.send_data("check " + std::to_string(timestamp / FILTER_LENGTH) + " "
                + mac_address + "|" + std::to_string(port) + "\n");
    std::string reply = c.receive(1024);
    
    if (reply.substr(0,3) == "Yes")
    {
        return true;
    }

    // fuzzy check at start of filter
    time_t filter_start = (timestamp / FILTER_LENGTH) * FILTER_LENGTH;
    if ((timestamp - filter_start) < FUZZINESS)
    {
        c.send_data("check " + std::to_string((timestamp / FILTER_LENGTH) - 1)
                    + " " + mac_address + "|" + std::to_string(port) + "\n");
        std::string reply = c.receive(1024);
        
        if (reply.substr(0,3) == "Yes")
        {
            return true;
        }
    }
    
    // fuzzy check at end of filter
    time_t filter_end = ((timestamp / FILTER_LENGTH) + 1) * FILTER_LENGTH;
    if ((filter_end - timestamp) < FUZZINESS)
    {
        c.send_data("check " + std::to_string((timestamp / FILTER_LENGTH) + 1)
                    + " " + mac_address + "|" + std::to_string(port) + "\n");
        std::string reply = c.receive(1024);
        
        if (reply.substr(0,3) == "Yes")
        {
            return true;
        }
    }

    return false;
}

void conn_log::add_ipv6(std::string mac_address,
                        std::string ipv6_address)
{
    // check for invalid MAC addresses
    if (!valid_mac(mac_address))
    {
        throw std::invalid_argument("Invalid conn_log.add_ipv6(mac_address): "
                + mac_address);
    }

    // check for invalid IPv6 addresses
    if (!valid_ipv6(ipv6_address))
    {
        throw std::invalid_argument("Invalid conn_log.has_ipv6(ipv6_address): "
                + ipv6_address);
    }

    // check for oversized conn_log and prune old filters every 10k connections
    static int i = 0;
    if (i++ % PRUNE_CHECK_FREQ == 0)
    {
        prune_filters();
    }

    // create timestamp
    time_t timestamp = time(nullptr);
    
    // check and create filter based on current timeslot
    c.send_data("create " + std::to_string(timestamp / FILTER_LENGTH) + "\n");
    std::string reply = c.receive(1024);
    std::cout << reply;

    // set string(timestamp / FILTER_LENGTH) string(mac_address + ipv6) 
    c.send_data("set " + std::to_string(timestamp / FILTER_LENGTH) + " "
                + mac_address + "|" + ipv6_address + "\n");
    reply = c.receive(1024);
    std::cout << reply;
}

bool conn_log::has_ipv6(std::string mac_address,
                        std::string ipv6_address,
                        time_t timestamp)
{
    if (!valid_mac(mac_address))
    {
        throw std::invalid_argument("Invalid conn_log.has_ipv6(mac_address): "
                + mac_address);
    }

    if (!valid_ipv6(ipv6_address))
    {
        throw std::invalid_argument("Invalid conn_log.has_ipv6(ipv6_address): "
                + ipv6_address);
    }

    // check 'correct' filter
    c.send_data("check " + std::to_string(timestamp / FILTER_LENGTH) + " "
                + mac_address + "|" + ipv6_address + "\n");
    std::string reply = c.receive(1024);
    
    if (reply.substr(0,3) == "Yes")
    {
        return true;
    }

    // fuzzy check at start of filter
    time_t filter_start = (timestamp / FILTER_LENGTH) * FILTER_LENGTH;
    if ((timestamp - filter_start) < FUZZINESS)
    {
        c.send_data("check " + std::to_string((timestamp / FILTER_LENGTH) - 1)
                    + " " + mac_address + "|" + ipv6_address + "\n");
        std::string reply = c.receive(1024);
        
        if (reply.substr(0,3) == "Yes")
        {
            return true;
        }
    }
    
    // fuzzy check at end of filter
    time_t filter_end = ((timestamp / FILTER_LENGTH) + 1) * FILTER_LENGTH;
    if ((filter_end - timestamp) < FUZZINESS)
    {
        c.send_data("check " + std::to_string((timestamp / FILTER_LENGTH) + 1)
                    + " " + mac_address + "|" + ipv6_address + "\n");
        std::string reply = c.receive(1024);
        
        if (reply.substr(0,3) == "Yes")
        {
            return true;
        }
    }

    return false;
}
