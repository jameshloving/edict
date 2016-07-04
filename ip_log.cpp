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
const int DEFAULT_COUNT = 1000000;       // default max capacity for Bloom filter
const float DEFAULT_PROBABILITY = 0.001; // default probability for Bloom filter false positive

// parent class for IPv4 and IPv6 sublogs
class ip_sublog_parent
{
protected:
  bloom_parameters parameters;
  bloom_filter filter;
  time_t creation_time;

  // TODO: fix this function
  bool valid_mac(std::string)
  {
    return true;
  }

public:
  ip_sublog_parent(int count,
                   float probability)
  {
    if (count > 0)
    {
      parameters.projected_element_count = count;
    }
    else
    {
      throw std::invalid_argument("Invalid ip_sublog_parent.projected_element_count: "
                                  + std::to_string(count));
    }

    if (probability > -1.0 && probability < 1.0)
    {
      parameters.false_positive_probability = probability;
    }
    else
    {
      throw std::invalid_argument("Invalid ip_sublog_parent.false_positive_probability: "
                                  + std::to_string(probability));
    }

    parameters.compute_optimal_parameters();

    bloom_filter filter_temp(parameters);
    filter = filter_temp;
    
    creation_time = time(nullptr);
  }

  time_t get_creation_time()
  {
    return creation_time;
  }
};

// struct for IPv4 source
struct ipv4_source
{
  std::string mac;
  uint16_t port;
};

// struct for IPv6 source
struct ipv6_source
{
  std::string mac;
  sockaddr_in6 ipv6;
};

// child class for IPv4 sublog
class ipv4_sublog
: public ip_sublog_parent
{
private:
  bool valid_port(uint16_t port)
  {
    return (port >= 0 && port < 65536);
  }
  
public:
  ipv4_sublog()
  : ip_sublog_parent(DEFAULT_COUNT, DEFAULT_PROBABILITY) {}

  ipv4_sublog(int count, float probability)
  : ip_sublog_parent(count, probability) {}

  void add_connection(std::string mac_address,
                      uint16_t port)
  {
    ipv4_source source;
    
    if (valid_mac(mac_address))
    {
      source.mac = mac_address;
    }
    else
    {
      throw std::invalid_argument("Invalid ipv4_sublog.add_connection(mac_address): "
                                  + mac_address);
    }

    if (valid_port(port))
    {
      source.port = port;
    }
    else
    {
      throw std::invalid_argument("Invalid ipv4_sublog.add_connection(port): "
                                  + std::to_string(port));
    }

    if ((time(nullptr) - creation_time) < SUBLOG_LENGTH)
    {
      std::cout << "    Inserted @ " << time(nullptr) << "\n";
      filter.insert(source);
    }
    else
    {
      throw std::out_of_range("Beyond ipv4 sublog length");
    }
  }

  bool has_connection(std::string mac_address,
                      uint16_t port)
  {
    ipv4_source source;
    source.mac = mac_address;
    // TODO: MAC address validation

    if (valid_port(port))
    {
      source.port = port;
    }
    else
    {
      throw std::invalid_argument("Invalid ipv4_sublog.has_connection(port): "
                                  + std::to_string(port));
    }
    
    std::cout << "      " << mac_address << ", " << port << "\n";
    std::cout << "      " << source.mac << ", " << source.port << "\n";
    std::cout << "      " << filter.contains(source) << "\n";
    return filter.contains(source);
  }
};

// TODO: child class for IPv6 sublog
class ipv6_sublog
: public ip_sublog_parent
{
private:
  // TODO: validate IPv6 address
  bool valid_ipv6(sockaddr_in6 ipv6_address)
  {
    return true;
  }

public:
  ipv6_sublog()
  : ip_sublog_parent(DEFAULT_COUNT, DEFAULT_PROBABILITY) {}

  ipv6_sublog(int count, float probability)
  : ip_sublog_parent(count, probability) {}

  void add_connection(std::string mac_address,
                      sockaddr_in6 ipv6_address)
  {
    ipv6_source source;
    
    if (valid_mac(mac_address))
    {
      source.mac = mac_address;
    }
    else
    {
      throw std::invalid_argument("Invalid ipv6_sublog.add_connection(mac_address): "
                                  + mac_address);
    }

    if (valid_ipv6(ipv6_address))
    {
      source.ipv6 = ipv6_address;
    }
    else
    {
      // TODO: include printed ipv6_address
      throw std::invalid_argument("Invalid ipv6_sublog.add_connection(ipv6_address)");
    }

    if ((time(nullptr) - creation_time) < SUBLOG_LENGTH)
    {
      filter.insert(source);
    }
    else
    {
      throw std::out_of_range("Beyond ipv6 sublog length");
    }
  }

  bool has_connection(std::string mac_address,
                      sockaddr_in6 ipv6_address)
  {
    return false;
  }
};
 
class ip_log
{
private:
  std::deque<ipv4_sublog> ipv4_log;
  std::deque<ipv6_sublog> ipv6_log;

public:
  void add_ipv4_connection(std::string mac_address,
                           uint16_t port)
  {
    if (ipv4_log.size() == 0)
    { 
      ipv4_sublog sublog;
      ipv4_log.push_back(sublog);
    }

    try
    {
      ipv4_log.back().add_connection(mac_address, port);
      std::cout << "          " << ipv4_log.back().has_connection(mac_address, port) << "\n";
    }
    catch (const std::out_of_range& e)
    {
      ipv4_sublog sublog;
      ipv4_log.push_back(sublog);
      ipv4_log.back().add_connection(mac_address, port);
      std::cout << "          " << ipv4_log.back().has_connection(mac_address, port) << "\n";
    }
  }

  bool has_ipv4_connection(std::string mac_address,
                           uint16_t port,
                           time_t timestamp)
  {
    int i = 0;
    for(auto it = ipv4_log.cbegin(); it != ipv4_log.cend(); ++it)
    {
      std::cout << "    sublog: " << i++ << "\n";
      std::cout << "    mac_address: " << mac_address << "\n";
      std::cout << "    port: " << port << "\n";
      std::cout << "    creation_time: " << it->get_creation_time() << "\n";
      std::cout << "    timestamp: " << timestamp << "\n";
      std::cout << "    " << ipv4_log.front().has_connection(mac_address, port) << "\n";
      // compare timestamp and creation times to get correct log
      // TODO: reverse timestamp & SUBLOG_LENGTH in second IF CHECK to mirror line 204
      if (timestamp >= it->get_creation_time() &&
          timestamp - it->get_creation_time() < SUBLOG_LENGTH)
      {
        // check for MAC+port combination in log
        return it->has_connection(mac_address, port);
      }
    }
    throw std::out_of_range("Invalid timestamp - no ipv4 log covering the timestamp's period");
  }
  // TODO: delete oldest sublog (done at end of timeslot) IFF out of space
  // TODO: determine relevant sublog by timestamp of connection
  // TODO: query relevant sublog for connection
  // TODO: IPv6 stuff
};
