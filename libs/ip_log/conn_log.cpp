/**
  conn_log.cpp
  James Loving
**/

#include <arpa/inet.h>    // convert IPv6 strings <-> sockaddr_in6
#include <exception>      // exception handling
#include <iostream>       // output
#include <memory>         // unique_ptr
#include <netinet/in.h>   // sockaddr_in6 IPv6 struct
#include <queue>          // basis for log-of-sublogs
#include <string>         // string class
#include <stdint.h>       // int vars of atypical size (16b, 32b)
#include <time.h>         // time(), etc.

#include "../bloom/bloom_filter.hpp"   // Bloom filter, by Arash Partow

const int SUBLOG_LENGTH = 3600;            // sublog length in seconds
const int SUBLOG_FUZZINESS = 10;           // number of seconds of fuzziness in checking sublogs
const int DEFAULT_COUNT = 2000000;         // default max capacity for Bloom filter
const float DEFAULT_PROBABILITY = 0.001;   // default probability for Bloom filter false positive
const unsigned int DEFAULT_CAPACITY = 168; // default number of timeslots' sublogs conn_log stores

// sublog for each timeslot in log 
class conn_sublog
{
protected:
  bloom_parameters parameters; // parameters for Bloom filter
  std::shared_ptr<bloom_filter> filter; // Bloom filter for this timeslot (stores IPv4 and IPv6)
  time_t creation_time;        // time Bloom filter was instantiated

public:
  // set parameters and generate Bloom filter
  conn_sublog()
  {
    parameters.projected_element_count = DEFAULT_COUNT;
    parameters.false_positive_probability = DEFAULT_PROBABILITY;

    parameters.compute_optimal_parameters();

    filter.reset(new bloom_filter(parameters));
    creation_time = time(nullptr);
  }

  time_t get_creation_time() const
  {
    return creation_time;
  }

  void add_ipv4(std::string mac_address,
                uint16_t port)
  {
    // insert the connection IFF still within duration of sublog
    if ((time(nullptr) - creation_time) < SUBLOG_LENGTH)
    {
      filter->insert(mac_address + std::to_string(port));
    }
    // otherwise, notify conn_log to create a new sublog
    else
    {
      throw std::out_of_range("Attempt to insert ipv6 beyond conn_sublog timespan");
    }
  }

  bool has_ipv4(std::string mac_address,
                uint16_t port) const
  {
    return filter->contains(mac_address + std::to_string(port));
  }

  void add_ipv6(std::string mac_address,
                std::string ipv6_address)
  {
    if ((time(nullptr) - creation_time) < SUBLOG_LENGTH)
    {
      filter->insert(mac_address + ipv6_address);
    }
    else
    {
      throw std::out_of_range("Attempt to insert ipv6 beyond conn_sublog timespan");
    }
  }

  bool has_ipv6(std::string mac_address,
                std::string ipv6_address) const
  {
    return filter->contains(mac_address + ipv6_address);
  }
};

// log of sublogs
class conn_log
{
private:
  std::deque<conn_sublog> log; // double-ended que of timeslots' sublogs
  unsigned int capacity;     // number of timeslots' sublogs the log will store

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
  conn_log(unsigned int c=DEFAULT_CAPACITY)
  {
    capacity = c;
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

    // instantiate log with first sublog
    if (log.size() == 0)
    {
      conn_sublog *sublog = new conn_sublog;
      log.push_back(*sublog);
    }

    // add connection to current Bloom filter or, if past timeslot duration,
    // create new filter and add to that
    try
    {
      log.back().add_ipv4(mac_address, port);
    }
    catch (const std::out_of_range& e)
    {
      conn_sublog *sublog = new conn_sublog;
      log.push_back(*sublog);
      log.back().add_ipv4(mac_address, port);
      if (log.size() == capacity)
      {
        log.pop_front();
      }
    }
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

    // find the log containing the time of the connection to check
    for(auto it = log.cbegin(); it != log.cend(); it++)
    {
      // compare timestamp and creation times to get correct log
      if (timestamp >= it->get_creation_time() &&
          timestamp - it->get_creation_time() < SUBLOG_LENGTH)
      {
        // fuzzy-check for MAC+port combination in log (and potentially neighbors)
        // if timestamp is in first 10sec of sublog period, check previous sublog AND current sublog
        if (timestamp - it->get_creation_time() < SUBLOG_FUZZINESS) {
          return (it->has_ipv4(mac_address, port) || (it-1)->has_ipv4(mac_address, port));
        }
        // if timestamp is in last 10sec of sublog period, check next sublog AND current sublog
        else if (timestamp - it->get_creation_time() > SUBLOG_LENGTH - SUBLOG_FUZZINESS) {
          return (it->has_ipv4(mac_address, port) || (it+1)->has_ipv4(mac_address, port));
        }
        // timestamp is solidly in current sublog, so only check current sublog
        else {
          return it->has_ipv4(mac_address, port);
        }
      }
    }

    throw std::out_of_range("Invalid timestamp - no ipv4 log covering the timestamp's period");
  }

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
};
