/**
  ip_log.cpp
  James Loving
**/

#include <chrono>
#include <exception>
#include <iostream>
#include <queue>
#include <string>
#include <stdint.h>

#include "libs/bloom/bloom_filter.hpp"

const int SUBLOG_LENGTH = 3600; // sublog length in seconds
const int DEFAULT_COUNT = 1000000; // default max capacity for Bloom filter
const float DEFAULT_PROBABILITY = 0.001; // default probability for Bloom filter false positive

// parent class for IPv4 and IPv6 sublogs
class ip_sublog_parent
{
protected:
  bloom_parameters parameters;
  bloom_filter filter;
  std::chrono::steady_clock::time_point creation_time;

public:
  ip_sublog_parent(int count, float probability)
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
    
    creation_time = std::chrono::steady_clock::now();
  }
};

// structure for IPv4 source
struct ipv4_source
{
  std::string mac;
  uint16_t port;
};

// structure for IPv6 source
struct ipv6_source
{
  std::string mac;
  // TODO: IPv6 source address
};

// child class for IPv4 sublog
class ipv4_sublog
: protected ip_sublog_parent
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

  void add_connection(std::string mac_address, uint16_t port)
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
      throw std::invalid_argument("Invalid ipv4_sublog.add_connection(port): "
                                  + std::to_string(port));
    }

    if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now()
                                                         - creation_time).count() < SUBLOG_LENGTH)
    {
      filter.insert(source);
    }
    else
    {
      throw std::out_of_range("Beyond ipv4 sublog length");
    }
  }

  bool has_connection(std::string mac_address, uint16_t port)
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
    
    return filter.contains(source);
  }
};

// TODO: child class for IPv6 sublog

class ip_log
{
private:
  std::queue<ipv4_sublog> ipv4_log;
  // TODO: IPv6 queue

public:
  void add_ipv4_connection(std::string mac_address, uint16_t port)
  {
    if (ipv4_log.size() == 0)
    { 
      ipv4_sublog sublog;
      ipv4_log.push(sublog);
    }

    try
    {
      ipv4_log.back().add_connection(mac_address, port);
    }
    catch (const std::out_of_range& e)
    {
      ipv4_sublog sublog;
      ipv4_log.push(sublog);
      ipv4_log.back().add_connection(mac_address, port);
    }
     // try to add to queue.first sublog
      // catch exception
      // push new sublog, 
      // and add to new queue.first sbblog
  }
  // TODO: add new sublog to log (done at end of timeslot)
  // TODO: delete oldest sublog (done at end of timeslot) IFF out of space
  // TODO: add connection to current sublog
  // TODO: determine relevant sublog by timestamp of connection
  // TODO: query relevant sublog for connection
  // TODO: IPv6 stuff
};

// TODO: remove, this is for testing only
int main()
{
  ipv4_sublog log(1000000, 0.001);
  log.add_connection("aa-bb-cc-dd-ee-ff", 10);
  std::cout << log.has_connection("aa-bb-cc-dd-ee-ff",10) << "\n";
  std::cout << sizeof(log) << " bytes\n";
  return 0;
}
