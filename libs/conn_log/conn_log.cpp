/*
    conn_log.cpp
    James H. Loving
*/

#include <arpa/inet.h>    // convert IPv6 strings <-> sockaddr_in6
#include <cstring>
#include <cstdlib>
#include <exception>      // exception handling
#include <iostream>       // output
#include <memory>         // unique_ptr
#include <netdb.h>
#include <netinet/in.h>   // sockaddr_in6 IPv6 struct
#include <queue>          // basis for log-of-sublogs
#include <regex>          // for MAC and IP address validation
#include <sys/socket.h>   // for network sockets
#include <sys/types.h>
#include <string>         // string class
#include <stdint.h>       // int vars of atypical size (16b, 32b)
#include <time.h>         // time(), etc.
#include <unistd.h>
#include <vector>

#include "tcp_client.cpp"
#include "../bloom/bloom_filter.hpp"   // Bloom filter, by Arash Partow

const unsigned int FILTER_LENGTH = 3600;            // sublog length in seconds
const unsigned int SUBLOG_FUZZINESS = 10;           // number of seconds of fuzziness in checking sublogs
const int MAX_FILTER_SIZE = 102400000;                 // maximum size of logs to store (in bytes)
const unsigned int PRUNE_CHECK_FREQ = 10000;        // check for oversized logs every 1/FREQ connections

class conn_log
{
    private:
        tcp_client c;

        // TODO: fix these functions
        bool valid_mac(std::string mac) const
        {
            std::regex pattern("^([0-9a-F]{1,2}[:-]){5}([0-9a-F]{1,2})$", std::regex_constants::basic);
            return true;
            return std::regex_match(mac, pattern);
        }

        bool valid_ipv6(std::string ipv6) const
        {
            std::regex pattern("(([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}|([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}|[0-9a-fA-F]{1,4}:((:[0-9a-fA-F]{1,4}){1,6})|:((:[0-9a-fA-F]{1,4}){1,7}|:)|fe80:(:[0-9a-fA-F]{0,4}){0,4}\%[0-9a-zA-Z]{1,}|::(ffff(:0{1,4}){0,1}:){0,1}((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])|([0-9a-fA-F]{1,4}:){1,4}:((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9]))");
            return true;
            return std::regex_match(ipv6, pattern);
        }

        unsigned int get_filter_size()
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

        void prune_filters()
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

    public:
        conn_log()
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

        void add_ipv4(std::string mac_address,
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
            std::cout << reply;

            // set string(timestamp / FILTER_LENGTH) string(mac_address + port) 
            c.send_data("set " + std::to_string(timestamp / FILTER_LENGTH) + " "
                        + mac_address + "|" + std::to_string(port) + "\n");
            reply = c.receive(1024);
            std::cout << reply;
        }

        bool has_ipv4(std::string mac_address,
                      uint16_t port,
                      time_t timestamp)
        {
            if (!valid_mac(mac_address))
            {
                throw std::invalid_argument("Invalid conn_log.has_ipv4(mac_address): "
                        + mac_address);
            }

            c.send_data("check " + std::to_string(timestamp / FILTER_LENGTH) + " "
                        + mac_address + "|" + std::to_string(port) + "\n");
            std::string reply = c.receive(1024);
            
            if (reply.substr(0,4) == "Yes")
            {
                return true;
            }

            // fuzzy check at start of filter
            time_t filter_start = (timestamp / FILTER_LENGTH) * FILTER_LENGTH;
            if (timestamp - filter_start < 60)
            {
                c.send_data("check " + std::to_string((timestamp / FILTER_LENGTH) - 1) + " "
                            + mac_address + "|" + std::to_string(port) + "\n");
                std::string reply = c.receive(1024);
                
                if (reply.substr(0,4) == "Yes")
                {
                    return true;
                }
            }
            
            // fuzzy check at end of filter
            time_t filter_end = ((timestamp / FILTER_LENGTH) + 1) * FILTER_LENGTH
            if (filter_end - timestamp) < 60)
            {
                c.send_data("check " + std::to_string((timestamp / FILTER_LENGTH) + 1) + " "
                            + mac_address + "|" + std::to_string(port) + "\n");
                std::string reply = c.receive(1024);
                
                if (reply.substr(0,4) == "Yes")
                {
                    return true;
                }
            }

            return false;
        }
/*
        void add_ipv6(std::string mac_address,
                      std::string ipv6_address)
        {
            if (!valid_mac(mac_address))
            {
                throw std::invalid_argument("Invalid conn_log.add_ipv6(mac_address): "
                        + mac_address);
            }

            if (!valid_ipv6(ipv6_address))
            {
                throw std::invalid_argument("Invalid conn_log.add_ipv6(ipv6_address): "
                        + ipv6_address);
            }

            if (log.size() == 0)
            {
                conn_sublog *sublog = new conn_sublog;
                log.push_back(*sublog);
            }

            try
            {
                log.back().add_ipv6(mac_address, ipv6_address);
            }
            catch (const std::out_of_range& e)
            {
                conn_sublog *sublog = new conn_sublog;
                log.push_back(*sublog);
                log.back().add_ipv6(mac_address, ipv6_address);
                if (log.size() == capacity)
                {
                    log.pop_front();
                }
            }
        }

        bool has_ipv6(std::string mac_address,
                      std::string ipv6_address,
                      time_t timestamp) const
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

            for(auto it = log.cbegin(); it != log.cend(); it++)
            {
                // compare timestamp and creation times to get correct log
                if (timestamp >= it->get_creation_time() &&
                        timestamp - it->get_creation_time() < FILTER_LENGTH)
                {
                    // fuzzy-check for MAC+address combination in log (and potentially neighbors)
                    // if timestamp is in first 10sec of sublog period, check previous sublog AND current sublog
                    if (timestamp - it->get_creation_time() < SUBLOG_FUZZINESS) {
                        return (it->has_ipv6(mac_address, ipv6_address) || (it-1)->has_ipv6(mac_address, ipv6_address));
                    }
                    // if timestamp is in last 10sec of sublog period, check next sublog AND current sublog
                    else if (timestamp - it->get_creation_time() > FILTER_LENGTH - SUBLOG_FUZZINESS) {
                        return (it->has_ipv6(mac_address, ipv6_address) || (it+1)->has_ipv6(mac_address, ipv6_address));
                    }
                    // timestamp is solidly in current sublog, so only check current sublog
                    else {
                        return it->has_ipv6(mac_address, ipv6_address);
                    }
                }
            }

            throw std::out_of_range("Invalid timestamp - no ipv4 log covering the timestamp's period");
        }
*/
};
