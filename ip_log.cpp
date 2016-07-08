/**
  ip_log.cpp
  James Loving
**/

#include <arpa/inet.h>    // convert IPv6 strings <-> sockaddr_in6
#include <exception>      // exception handling
#include <iostream>       // output
#include <netinet/in.h>   // sockaddr_in6 IPv6 struct
#include <queue>          // basis for log-of-sublogs
#include <string>         // string class
#include <stdint.h>       // int vars of atypical size (16b, 32b)
#include <time.h>         // time(), etc.

#include "libs/bloom/bloom_filter.hpp"   // Bloom filter, by Arash Partow

const int SUBLOG_LENGTH = 5;             // sublog length in seconds, TODO: change to 3600
const int DEFAULT_COUNT = 2000000;       // default max capacity for Bloom filter
const float DEFAULT_PROBABILITY = 0.001; // default probability for Bloom filter false positive

// sublog for each timeslot in log 
class ip_sublog
{
protected:
  bloom_parameters parameters;
  bloom_filter *filter;
  time_t creation_time;

  // TODO: fix these functions
  bool valid_mac(std::string mac) const
  {
    return true;
  }

  bool valid_port(uint16_t port) const
  {
    return true; 
  }

  bool valid_ipv6(std::string ipv6) const
  {
    return true;
  }

public:
  ip_sublog()
  {
    parameters.projected_element_count = DEFAULT_COUNT;
    parameters.false_positive_probability = DEFAULT_PROBABILITY;

    parameters.compute_optimal_parameters();

    filter = new bloom_filter(parameters);
    
    creation_time = time(nullptr);
  }

  time_t get_creation_time() const
  {
    return creation_time;
  }

  void add_ipv4(std::string mac_address,
                uint16_t port)
  {
    if (!valid_mac(mac_address))
    {
      throw std::invalid_argument("Invalid ipv4_sublog.add_connection(mac_address): "
                                  + mac_address);
    }

    if (!valid_port(port))
    {
      throw std::invalid_argument("Invalid ipv4_sublog.add_connection(port): "
                                  + std::to_string(port));
    }

    if ((time(nullptr) - creation_time) < SUBLOG_LENGTH)
    {
      std::cout << "    Inserted @ " << time(nullptr) << "\n";
      filter->insert(mac_address + std::to_string(port));
    }
    else
    {
      throw std::out_of_range("Beyond ipv4 sublog length");
    }
  }

  bool has_ipv4(std::string mac_address,
                uint16_t port) const
  {
    if (!valid_mac(mac_address))
    {
      throw std::invalid_argument("Invalid ipv4_sublog.add_connection(mac_address): "
                                  + mac_address);
    }

    if (!valid_port(port))
    {
      throw std::invalid_argument("Invalid ipv4_sublog.has_connection(port): "
                                  + std::to_string(port));
    }
    
    return filter->contains(mac_address + std::to_string(port));
  }

  // TODO: add_ipv6
  // TODO: has_ipv6
};

// log of sublogs
class ip_log
{
private:
  std::deque<ip_sublog> log;

public:
  void add_ipv4_connection(std::string mac_address,
                           uint16_t port)
  {
    if (log.size() == 0)
    {
      ip_sublog *sublog = new ip_sublog;
      log.push_back(*sublog);
    }

    try
    {
      log.back().add_ipv4(mac_address, port);
    }
    catch (const std::out_of_range& e)
    {
      ip_sublog *sublog = new ip_sublog;
      log.push_back(*sublog);
      log.back().add_ipv4(mac_address, port);
    }
  }

  bool has_ipv4_connection(std::string mac_address,
                           uint16_t port,
                           time_t timestamp)
  {
    int i = 0;
    for(auto it = log.cbegin(); it != log.cend(); it++)
    {
      std::cout << "    sublog: " << i++ << "\n";
      std::cout << "      creation_time: " << it->get_creation_time() << "\n";
      std::cout << "    timestamp: " << timestamp << "\n";
      std::cout << "    front().has_connection(): " << log.front().has_ipv4(mac_address, port) << "\n";
      std::cout << "    back().has_connection(): " << log.back().has_ipv4(mac_address, port) << "\n";
      // compare timestamp and creation times to get correct log
      if (timestamp >= it->get_creation_time() &&
          timestamp - it->get_creation_time() < SUBLOG_LENGTH)
      {
       // check for MAC+port combination in log
        return it->has_ipv4(mac_address, port);
      }
    }
    throw std::out_of_range("Invalid timestamp - no ipv4 log covering the timestamp's period");
  }
  // TODO: delete oldest sublog (done at end of timeslot) IFF out of space
  // TODO: determine relevant sublog by timestamp of connection
  // TODO: query relevant sublog for connection
  // TODO: IPv6 stuff
};
