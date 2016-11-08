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
#include <unordered_map>

#include "../libs/conn_log/conn_log.cpp"
#include "../libs/device_log/device_log.cpp"

struct device_log_entry
{
	std::string make_model;
	time_t first_seen;		
};

struct region
{
    conn_log connections;
};
struct region *shm;

bool check_ipv4(time_t timestamp, uint16_t source_port)
{
    return true;
}

bool check_ipv6(time_t timestamp, std::string source_ipv6)
{
    return true;
}

int main(int argc, char **argv)
{
	if (argc != 4)
	{
		std::cout << "Usage: " << argv[0] << " <timestamp> <\"v4\" or \"v6\"> <source port OR IPv6 source address>\n";
		return 0;
	}

    time_t timestamp;
    int fd_shm;

	// TODO: validate timestamp
    if (true)
    {
        timestamp = static_cast<time_t>(atoi(argv[1]));
    }	
    else
    {
        std::cout << "Invalid timestamp.\n";
        return 0;
    }

    device_log d;
    std::unordered_map<std::string, struct device_log_entry> = d.get_devices();

    // Create shared memory object and set its size
    fd_shm = shm_open("/myregion", O_RDONLY, S_IRUSR | S_IWUSR);
    if (fd_shm == -1)
    {
        std::cerr << "can't shm_open /myregion: " << errno << "\n";
        exit(1);
    }

    /*
    if (ftruncate(fd_shm, sizeof(struct region)) == -1)
    {
        std::cerr << "can't ftruncate fd_shm: " << errno << "\n";
        exit(1);
    }
    */

    shm = static_cast<struct region *>(mmap(NULL, sizeof(struct region),
                                            PROT_READ, MAP_SHARED,
                                            fd_shm, 0));

    shm = new struct region();

    if (shm == MAP_FAILED)
    {
        fprintf(stderr, "can't mmap shm\n");
        exit(1);
    }

    if (strcmp(argv[2], "v4") == 0)
    {
        if (true) // TODO: validate IPv4 source port
        {
            uint16_t source_port = static_cast<uint16_t>(atoi(argv[3]));
            std::cout << check_ipv4(timestamp, source_port) << "\n";
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

	return 0;
}
