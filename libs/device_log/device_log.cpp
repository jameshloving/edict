//=============================================================================
//
// Name:        device_log.cpp
// Authors:     James H. Loving
// Description: This file defines the device_log class, used to log devices
//              that have connected to EDICT's router.
//
//=============================================================================

#include "device_log.hpp"

device_log::device_log()
{
    dnt = get_dnt();
    macs = get_macs();
}

void device_log::add_device(std::string mac_address,
                            std::string make_model)
{
    if (current_mac_size < MAX_LOG_SIZE)
    { 
        // add the device
        ++current_log_size;
        macs.insert(mac_address);

        std::ofstream outfile;
        outfile.open(DEVICE_LOG_FILE, std::ios::out | std::ios::app);

        time_t now;
        time(&now);
        char time_buf[sizeof("1111-11-11T11:11:11Z")];
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));

        outfile << time_buf << ",";
        outfile << mac_address << ",";
        outfile << make_model << "\n";

        outfile.close();

        std::cout << "device_log.add_device(" << time_buf << "," << mac_address 
                  << "," << make_model << ")\n";
    }
    else
    {
        // TODO: get a better answer. Delete oldest? Delete newest?
        throw std::runtime_error("Device_log is at MAX_LOG_SIZE. Please backup to an external source.");
    }
}

unsigned int device_log::count(std::string mac_address)
{
    return macs.count(mac_address);
}

bool device_log::should_log(std::string mac_address)
{
    if (dnt.count(mac_address))
    {
        return false;
    }
    else
    {
        return true;
    }
}

std::unordered_set<std::string> device_log::get_macs()
{
    std::unordered_set<std::string> mac_addresses;

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

        mac_addresses.insert(mac_address);
    }    

    return mac_addresses;
}

std::unordered_set<std::string> device_log::get_dnt()
{
    std::unordered_set<std::string> dnt;

    std::ifstream infile;
    infile.open(DNT_FILE, std::ios::in);

    std::string line;
    while(std::getline(infile,line))
    {
        std::stringstream lineStream(line);
        std::string mac_address;

        std::getline(lineStream, mac_address, '\n');

        dnt.insert(mac_address);
    }

    return dnt;
}

std::map<std::string, struct device_log_entry> device_log::get_devices()
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

        struct tm t{};
        if (strptime(timestamp.c_str(), "%Y-%m-%dT%H:%M:%SZ", &t) != NULL)
        {
            entry.first_seen = mktime(&t) + (&t)->tm_gmtoff;
        }

        devices.insert(std::pair<std::string, struct device_log_entry>(mac_address, entry));
    }

    return devices;
}
