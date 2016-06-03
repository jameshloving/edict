/**
  ip_log.cpp
  James Loving
**/

#include <exception>
#include <iostream>
#include <queue>
#include <string>
#include <stdint.h>

#include "libs/bloom/bloom_filter.hpp"

// parent class for IPv4 and IPv6 sublogs
class ip_sublog_parent
{
protected:
  bloom_parameters parameters;
  bloom_filter filter;

public:
  ip_sublog_parent(int count, float probability)
  {
    if (count > 0)
    {
      parameters.projected_element_count = count;
    }
    else
    {
      throw std::invalid_argument("Invalid ip_log.projected_element_count: "
                                  + std::to_string(count));
    }

    if (probability > -1.0 && probability < 1.0)
    {
      parameters.false_positive_probability = probability;
    }
    else
    {
      throw std::invalid_argument("Invalid ip_log.false_positive_probability: "
                                  + std::to_string(probability));
    }

    parameters.compute_optimal_parameters();

    bloom_filter filter_temp(parameters);
    filter = filter_temp;
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
public:
  ipv4_sublog(int count, float probability)
  : ip_sublog_parent(count, probability) {}

  void add_connection(std::string mac_address, uint16_t port)
  {
    ipv4_source source;
    source.mac = mac_address;
    
    if (port >= 0 && port < 65536)
    {
      source.port = port;
    }
    else
    {
      throw std::invalid_argument("Invalid ipv4_sublog.add_connection(port): "
                                  + std::to_string(port));
    }

    filter.insert(source);
  }

  bool has_connection(std::string mac_address, uint16_t port)
  {
    ipv4_source source;
    source.mac = mac_address;

    if (port >= 0 && port < 65536)
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
  ipv4_sublog log(10000, 0.001);
  log.add_connection("aa-bb-cc-dd-ee-ff", 10);
  std::cout << log.has_connection("aa-bb-cc-dd-ee-ff",10) << "\n";
  std::cout << sizeof(log) << " bytes\n";
  return 0;
}
