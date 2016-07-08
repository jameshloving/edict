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

const int SUBLOG_LENGTH = 5;               // sublog length in seconds, TODO: change to 3600
const int DEFAULT_COUNT = 2000000;         // default max capacity for Bloom filter
const float DEFAULT_PROBABILITY = 0.001;   // default probability for Bloom filter false positive
const unsigned int DEFAULT_CAPACITY = 168; // default number of timeslots' sublogs ip_log stores

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
      throw std::invalid_argument("Invalid ip_sublog.add_ipv4(mac_address): "
                                  + mac_address);
    }

    if ((time(nullptr) - creation_time) < SUBLOG_LENGTH)
    {
      std::cout << "    Inserted @ " << time(nullptr) << "\n";
      filter->insert(mac_address + std::to_string(port));
    }
    else
    {
      throw std::out_of_range("Attempt to insert ipv6 beyond ip_sublog timespan");
    }
  }

  bool has_ipv4(std::string mac_address,
                uint16_t port) const
  {
    if (!valid_mac(mac_address))
    {
      throw std::invalid_argument("Invalid ip_sublog.has_ipv4(mac_address): "
                                  + mac_address);
    }

    return filter->contains(mac_address + std::to_string(port));
  }

  void add_ipv6(std::string mac_address,
                std::string ipv6_address)
  {
    if (!valid_mac(mac_address))
    {
      throw std::invalid_argument("Invalid ip_sublog.add_ipv6(mac_address): "
                                  + mac_address);
    }

    if ((time(nullptr) - creation_time) < SUBLOG_LENGTH)
    {
      filter->insert(mac_address + ipv6_address);
    }
    else
    {
      throw std::out_of_range("Attempt to insert ipv6 beyond ip_sublog timespan");
    }
  }

  bool has_ipv6(std::string mac_address,
                std::string ipv6_address) const
  {
    if (!valid_mac(mac_address))
    {
      throw std::invalid_argument("Invalid ip_sublog.has_ipv6(mac_address): "
                                  + mac_address);
    }

    if (!valid_ipv6(ipv6_address))
    {
      throw std::invalid_argument("Invalid ip_sublog.has_ipv6(ipv6_address): "
                                  + ipv6_address);
    }

    return filter->contains(mac_address + ipv6_address);
  }
};

// log of sublogs
class ip_log
{
private:
  std::deque<ip_sublog> log;
  unsigned int capacity;  // number of timeslots' sublogs the log will store

public:
  ip_log(unsigned int c=DEFAULT_CAPACITY)
  {
    capacity = c;
  }

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
      if (log.size() == capacity)
      {
        log.pop_front();
      }
    }
  }

  bool has_ipv4_connection(std::string mac_address,
                           uint16_t port,
                           time_t timestamp) const
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


};
