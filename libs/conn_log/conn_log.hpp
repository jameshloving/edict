//=============================================================================
//
// Name:        conn_log.hpp
// Authors:     James H. Loving
// Description: This file declares the conn_log class, used to log IPv4 and
//              IPv6 communications on a local Bloomd server.
//
//=============================================================================

#include <exception>      // exception handling
#include <iostream>       // output
#include <regex>          // MAC and IP address validation
#include <string>         // string class
#include <stdint.h>       // int vars of atypical size (16b, 32b)
#include <time.h>         // time(), etc.

#include "tcp_client.hpp"

/**
    Log IPv4 and IPv6 communications on a local Bloomd server.
*/
class conn_log
{
    private:
        const unsigned int FILTER_LENGTH = 3600;    /**< sublog length (seconds) */
        const unsigned int FUZZINESS = 10;          /**< how fuzzy to check
                                                         sublogs (seconds) */
        const int MAX_FILTER_SIZE = 102400000;      /**< max sum of filters (bytes) */
        const unsigned int PRUNE_CHECK_FREQ = 10000;/**< how often to check for 
                                                         oversized filter (seconds */
        tcp_client c;                               /**< TCP client for communication
                                                         with Bloomd */

        /**
            Get the current total size of all Bloomd filters in bytes.

            \return Unsigned sum of all Bloomd filters, in bytes.
        */
        unsigned int get_filter_size();
        
        /**
            Delete the oldest Bloomd filter until the sum of all filters'
            size in bytes is less than MAX_FILTER_SIZE.
        */
        void prune_filters();

    public:
        /**
            Initialize & test the connection to the local Bloomd server.
        */
        conn_log();

        // TODO: fix these functions
        /**
            Test a string-encoded MAC address for validity. A valid MAC
            has 12 hex characters, no colons/spaces/hyphens.

            \param mac 48-bit/12-char MAC address (ex "aabbccddeeff").

            \return Boolean indicator of MAC validity.
        */
        bool valid_mac(std::string mac) const;
        
        /**
            Test a string-encoded IPv6 address for validity.

            \param ipv6 128-bit IPv6 address, encoded as a string.

            \return Boolean indicator of IPv6 address validity.
        */
        bool valid_ipv6(std::string ipv6) const;

        /**
            Add an IPv4/TCP connection to the current Bloomd filter.

            \param mac_address String-encoded MAC address.
                See conn_log::valid_mac for validity rules.
            \param port TCP source port, 0-65535.
        */
        void add_ipv4(std::string mac_address,
                      uint16_t port);
        
        /**
            Determine the appropriate Bloomd filter and check if it contains
            a specific IPv4 connection.

            \param mac_address MAC address of connection to check.
                See conn_log::valid_mac for validity rules.
            \param port TCP source port (0-65535) of connection to check.
            \param timestamp Time_t-encoded timestamp of connection to check.

            \return Boolean indicator of the connection's presence in filters.
        */
        bool has_ipv4(std::string mac_address,
                      uint16_t port,
                      time_t timestamp);

        /**
            Add an IPv6/TCP connection to the current Bloomd filter.

            \param mac_address String-encoded MAC address.
                See conn_log::valid_mac for validity rules.
            \param ipv6_address 128-bit IPv6 address, encoded as a string.
                See conn_log::valid_ipv6 for validity rules.
        */
        void add_ipv6(std::string mac_address,
                      std::string ipv6_address);
        
        /**
            Determine the appropriate Bloomd filter and check if it contains
            a specific IPv6 connection.

            \param mac_address MAC address of connection to check.
                See conn_log::valid_mac for validity rules.
            \param ipv6_address 128-bit IPv6 address to check, encoded as a
                string. See conn_log::valid_ipv6 for validity rules.
            \param timestamp Time_t-encoded timestamp of connection to check.

            \return Boolean indicator of the connection's presence in filters.
        */
        bool has_ipv6(std::string mac_address,
                      std::string ipv6_address,
                      time_t timestamp);
};
