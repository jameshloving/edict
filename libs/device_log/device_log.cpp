/**
  device_log.cpp
  James Loving
**/

#include <exception>      // exception handling
#include <iostream>       // output
#include <fstream>        // file IO
#include <string>         // string class
#include <time.h>         // time(), etc.
#include <vector>

const char DEVICE_LOG_FILE[] = "../stor/device_log.txt";

class device_log
{
protected:
    struct device_log_entry
    {
        std::string make_model;
        time_t first_seen;
    };

public:
    device_log()
    {
    }

    void add_device(std::string mac_address,
                    std::string make_model)
    {
        std::ofstream outfile;
        outfile.open(DEVICE_LOG_FILE, std::ios::out | std::ios::app);

        time_t now;
        time(&now);
        char time_buf[sizeof("1111-11-11T11:11:11Z")];
        strftime(time_buf, sizeof(time_buf), "%F %TZ", gmtime(&now));
        outfile << "\"" << time_buf << "\",";

        outfile << "\"" << mac_address << "\",";

        outfile << "\"" << make_model << "\",";

        outfile << "\n";
        outfile.close();
    }

/*
    std::vector<std::string, struct device_log_entry> read_devices()
    {
    }
*/
};
