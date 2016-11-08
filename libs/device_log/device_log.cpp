/**
  device_log.cpp
  James Loving
**/

#include <ctime>
#include <exception>      // exception handling
#include <iostream>       // output
#include <fstream>        // file IO
#include <sstream>
#include <string>         // string class
#include <time.h>         // time(), etc.
#include <map>
#include <unordered_set>

const char DEVICE_LOG_FILE[] = "../stor/device_log.txt";

struct device_log_entry
{
    std::string make_model;
    time_t first_seen;
};

class device_log
{
protected:
    std::unordered_set<std::string> macs;

public:
    device_log()
    {
    }

    void add_device(std::string mac_address,
                    std::string make_model)
    {
        macs.insert(mac_address);

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

    unsigned int count(std::string mac_address)
    {
        return macs.count(mac_address);
    }

    std::map<std::string, struct device_log_entry> read_devices()
    {
        std::map<std::string, struct device_log_entry> devices;

        std::ifstream infile;
        infile.open(DEVICE_LOG_FILE, std::ios::in);

        std::string line;
        while(std::getline(infile,line))
        {
            std::stringstream lineStream(line);
            std::string timestamp, mac_address, make_model;

            std::getline(lineStream, timestamp, ',');
            std::getline(lineStream, mac_address, ',');
            std::getline(lineStream, make_model, ',');

            struct device_log_entry entry;
            
            entry.make_model = make_model;

            struct tm t;
            strptime(timestamp.c_str(), "%Y-%m-%d %H:%M:%S", &t);
            entry.first_seen = mktime(&t);

            devices.insert(std::pair<std::string, struct device_log_entry>(mac_address, entry));
        }
    
        return devices;
    }
};
