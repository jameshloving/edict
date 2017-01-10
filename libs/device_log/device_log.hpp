//=============================================================================
//
// Name:        device_log.hpp
// Authors:     James H. Loving
// Description: This file declares the device_log class, used to log devices
//              that have connected to EDICT's router.
//
//=============================================================================

#ifndef DEVICE_LOG_HPP
#define DEVICE_LOG_HPP

#include <ctime>
#include <exception>      // exception handling
#include <iostream>       // output
#include <fstream>        // file IO
#include <sstream>
#include <string>         // string class
#include <time.h>         // time(), etc.
#include <map>
#include <unordered_set>

/**
    Each entry in the device log stores the device's make&model and
    the timestamp the device was first seen.
*/
struct device_log_entry
{
    std::string make_model; /**< Model and manufacturer of device */
    time_t first_seen;      /**< Timestamp of device's initial connection */
};

/**
    Log devices' make, model, and timestamp of first connection as they
    connect to the router.
*/
class device_log
{
    protected:
        const char DEVICE_LOG_FILE[] = "../stor/device_log.txt";
                                                /**< file location to store
                                                     device log entries */
        std::unordered_set<std::string> macs;   /**< cache of stored MACs */

    public:
        /**
            Initiate the device_log and MAC cache from file.
        */
        device_log();

        /**
            Add a device to the log.

            \param mac_address Valid 12-char MAC address of device to store.
            \param make_model String-encoded info on device's make and model,
                derived from Google taxonomy code.
        */
        void add_device(std::string mac_address,
                        std::string make_model);

        /**
            Get the number of devices logged under a specified MAC address.

            \param mac_address Valid 12-char MAC address of device to search.

            \return Number of devices stored under that MAC address.
        */
        unsigned int count(std::string mac_address);

        /**
            Recreate the MAC cache from file.

            \return MAC cache, as an unordered set of String-encoded MACs.
        */
        std::unordered_set<std::string> get_macs();
        
        /**
            Recreate the device log from file.

            \return Device log, as a map of MACs to device_log entries.
        */
        std::map<std::string, struct device_log_entry> get_devices();
};

#endif
