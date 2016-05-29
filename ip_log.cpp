/**
  ip_log.cpp
  James Loving
**/

#include <iostream>
#include <string>
#include <exception>
#include <stdint.h>

#include "libs/bloom/bloom_filter.hpp"

// superclass for IPv4 and IPv6 timeslot logs
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

// subclass for IPv4 timeslot log
class ipv4_log
: protected ip_log
{
public:
  ipv4_log(int count, float probability)
  : ip_log(count, probability) {}

  // TODO: IPv4 data type
	void add_connection(uint32_t address, uint16_t port)
	{
		filter.insert(address);
	}
};

// TODO: subclass for IPv6 timeslot log

// TODO: log-of-logs for IPv4 and IPv6

// TODO: remove, this is for testing only
int main()
{
	ipv4_log log(10000, 0.001);

	log.add_connection(3, 10);

	return 0;
}
