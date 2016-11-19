/*
    conn_log.cpp
    James H. Loving
*/

#include <arpa/inet.h>    // convert IPv6 strings <-> sockaddr_in6
#include <cstring>
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

#include "tcp_client.cpp"
#include "../bloom/bloom_filter.hpp"   // Bloom filter, by Arash Partow

const int SUBLOG_LENGTH = 3600;            // sublog length in seconds
const int SUBLOG_FUZZINESS = 10;           // number of seconds of fuzziness in checking sublogs
const int DEFAULT_COUNT = 2000000;         // default max capacity for Bloom filter
const float DEFAULT_PROBABILITY = 0.001;   // default probability for Bloom filter false positive

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

            // create timestamp
            time_t timestamp = time(nullptr);
            
            // check and create filter based on current timeslot
            c.send_data("create " + std::to_string(timestamp / SUBLOG_LENGTH) + "\n");
            std::string reply = c.receive(1024);
            std::cout << reply;

            // set string(timestamp / SUBLOG_LENGTH) string(mac_address + port) 
            c.send_data("set " + std::to_string(timestamp / SUBLOG_LENGTH) + " "
                        + mac_address + "|" + std::to_string(port) + "\n");
            reply = c.receive(1024);
            std::cout << reply;
        }

        bool has_ipv4(std::string mac_address,
                      uint16_t port,
                      time_t timestamp) const
        {
            if (!valid_mac(mac_address))
            {
                throw std::invalid_argument("Invalid conn_log.has_ipv4(mac_address): "
                        + mac_address);
            }

            // list(timestamp / SUBLOG_LENGTH)
            // if "Filter does not exist"
            // then return false
            // throw std::out_of_range("Invalid timestamp - no ipv4 log covering the timestamp's period");

            // check string(timestamp / SUBLOG_LENGTH) string(mac_address + port)
            return true;
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
                        timestamp - it->get_creation_time() < SUBLOG_LENGTH)
                {
                    // fuzzy-check for MAC+address combination in log (and potentially neighbors)
                    // if timestamp is in first 10sec of sublog period, check previous sublog AND current sublog
                    if (timestamp - it->get_creation_time() < SUBLOG_FUZZINESS) {
                        return (it->has_ipv6(mac_address, ipv6_address) || (it-1)->has_ipv6(mac_address, ipv6_address));
                    }
                    // if timestamp is in last 10sec of sublog period, check next sublog AND current sublog
                    else if (timestamp - it->get_creation_time() > SUBLOG_LENGTH - SUBLOG_FUZZINESS) {
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
