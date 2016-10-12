#include <arpa/inet.h>
#include <unordered_map>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <time.h>

#include "../libs/ip_log/ip_log.cpp"

struct device_log_entry
{
	std::string make_model;
	time_t first_seen;		
};

// TODO: put these into shared memory
//std::unordered_map<std::string, struct device_log_entry> device_log;

//ip_log conn_log;

int main(int argc, char **argv)
{
	if (argc != 4)
	{
		std::cout << "Usage: " << argv[0] << " <timestamp> <\"-4\" or \"-6\"> <source port OR IPv6 source address>\n";
		return 0;
	}

	// TODO: validate timestamp
	// TODO: convert timestamp to time_t

	// TODO: determine source port vs. IPv6 address
	// TODO: IF source port, validate source port and assign
	// TODO: IF source IPv6, validate address and assign

	// TODO: locate memory segment

	// TODO: attach mem segment to data space

	// TODO: read what edict.cpp put into memory

	// TODO: query the log

	// TODO: output the result

	return 0;
}
