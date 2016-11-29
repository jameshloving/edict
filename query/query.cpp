#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "../libs/conn_log/conn_log.cpp"
#include "../libs/device_log/device_log.cpp"

conn_log connections;
device_log d;
std::map<std::string, struct device_log_entry> devices = d.get_devices();

std::map<std::string, struct device_log_entry> check_ipv4(time_t timestamp, uint16_t source_port)
{
    std::map<std::string, struct device_log_entry> has_ipv4;
    
    for (std::map<std::string, struct device_log_entry>::iterator it=devices.begin(); it != devices.end(); ++it)
    {
        // potential mismatch between MAC formats between device log and conn log
        // appearance: device log stores w/out colons, conn log stores w/
        std::string mac = it->first;
        
        if (connections.has_ipv4(mac, source_port, timestamp))
        {
            has_ipv4.insert(std::pair<std::string, struct device_log_entry>(mac, devices[mac]));
        }
    } 

    return has_ipv4;
}

bool check_ipv6(time_t timestamp, std::string source_ipv6)
{
    return true;
}

int main(int argc, char **argv)
{
	if (argc != 4)
	{
		std::cout << "Usage: " << argv[0] << " <UTC timestamp> <\"v4\" or \"v6\"> <source port OR IPv6 source address>\n";

        time_t now;
        time(&now);
        char time_buf[sizeof("1111-11-11T11:11:11Z")];
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now)); 
        std::cout << "Current UTC time: " << time_buf << "\n";

		return 0;
	}

    time_t timestamp;
    int fd_shm;

	// TODO: validate timestamp
    struct tm t{};
    if (strptime(argv[1], "%Y-%m-%dT%H:%M:%SZ", &t) != NULL)
    {
        timestamp = mktime(&t) + (&t)->tm_gmtoff;
    }	
    else
    {
        std::cout << "Invalid timestamp.\n";
        return 0;
    }

    std::cout << "checking time: " << timestamp << "\n";

    std::cout << "START\n";

    if (strcmp(argv[2], "v4") == 0)
    {
        if (true) // TODO: validate IPv4 source port
        {
            uint16_t source_port = static_cast<uint16_t>(atoi(argv[3]));
            std::map<std::string, struct device_log_entry> results = check_ipv4(timestamp, source_port);
            if (results.empty())
            {
                std::cout << "No match.\n";
            }
            else
            {
                for (std::map<std::string, struct device_log_entry>::iterator it=results.begin(); it != results.end(); ++it)
                {
                    char time_buf[sizeof("1111-11-11T11:11:11Z")];
                    strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&it->second.first_seen));
                    std::cout << "Match:"
                              << "\n  Make & model: " << it->second.make_model
                              << "\n  MAC address: " << it->first
                              << "\n  First seen: " << time_buf
                              << "\n";
                }
            }
        }
        else
        {
            std::cout << "Invalid source port.\n";
            return 0;
        }
    }
    else if (strcmp(argv[2], "v6") == 0)
    {
        if (true) // TODO: validate IPv6 source address
        {
            std::string source_ipv6(argv[3]);
            std::cout << check_ipv6(timestamp, source_ipv6) << "\n";
        }
        else
        {
            std::cout << "Invalid source IPv6 address.\n";
            return 0;
        }
    }
    else
    {
        std::cout << "Invalid protocol.\n";
		return 0;
    }

    std::cout << "END\n";

	return 0;
}
