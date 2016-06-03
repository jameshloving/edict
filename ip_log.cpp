/**
  ip_log.cpp
  James Loving
**/

#include <iostream>
#include <string>
#include <exception>
#include <stdint.h>

#include "libs/bloom/bloom_filter.hpp"

// parent class for IPv4 and IPv6 timeslot logs
class ip_log
{
protected:
  bloom_parameters parameters;
  bloom_filter filter;

public:
  ip_log(int count, float probability)
  {
    if (count > 0)
    {
      parameters.projected_element_count = count;
    }
    else
    {
      throw std::invalid_argument("Invalid ip_log.projected_element_count: " + std::to_string(count));
    }

    if (probability > -1.0 && probability < 1.0)
    {
      parameters.false_positive_probability = probability;
    }
    else
    {
      throw std::invalid_argument("Invalid ip_log.false_positive_probability: " + std::to_string(probability));
    }

    parameters.compute_optimal_parameters();

    bloom_filter filter_temp(parameters);
    filter = filter_temp;
  }
};

// structure for IPv4 source
struct ipv4_source
{
  uint32_t mac;
  uint16_t port;
};

// child class for IPv4 timeslot log
class ipv4_log
: protected ip_log
{
private:
  // TODO: MAC_string_to_uint32()
  uint32_t MAC_string_to_uint32(std::string string)
  {
    uint32_t x = 0;
    return x;
  }

public:
  ipv4_log(int count, float probability)
  : ip_log(count, probability) {}
  
  void add_connection(std::string mac_address, uint16_t port)
  {
    ipv4_source source;
    source.mac = MAC_string_to_uint32(mac_address);
    source.port = port;
    filter.insert(source);
  }

  bool has_connection(std::string mac_address, uint16_t port)
  {
    ipv4_source source;
    source.mac = MAC_string_to_uint32(mac_address);
    source.port = port;
    return filter.contains(source);
  }
};

// TODO: child class for IPv6 timeslot log

// structure for IPv6 source
struct ipv6_source
{
  // TODO: IPv6 source address
  uint16_t port;
};

// TODO: log-of-logs for IPv4 and IPv6

// TODO: remove, this is for testing only
int main()
{
  ipv4_log log(10000, 0.001);
  log.add_connection("aa-bb-cc-dd-ee-ff", 10);
  std::cout << log.has_connection("aa-bb-cc-dd-ee-ff",10) << "\n";
  std::cout << sizeof(log) << " bytes\n";
  return 0;
}
