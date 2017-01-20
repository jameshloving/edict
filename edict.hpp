//=============================================================================
//
// Name:        edict.hpp
// Authors:     James H. Loving
// Description: This file declares supporting functions for edict_main.cpp:
//              parsing command line arguments, starting EDICT, and querying
//              edict for IPv4 and IPv6 connections.
//              This file was split from edict_main.cpp to support unit
//              testing of the contained functions.
//
//=============================================================================

#include <arpa/inet.h>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unordered_set>
#include <unistd.h>

extern "C"
{
    #include <libnetfilter_log/libnetfilter_log.h>
}

#include "libs/conn_log/conn_log.hpp"
#include "libs/device_log/device_log.hpp"

/**
    Store logs, used for passing logs to log_packet via callback and void* ptr.
*/
struct log_struct
{
    conn_log *connections;
    device_log *devices;
};

/**
    Store parsed command-line arguments, for ease of use.
*/
struct args_struct
{
    std::string command;
    std::string query_timestamp;
    std::string query_version;
    std::string query_metadata;
    std::string print_format;
};

/**
    Parse command-line arguments into a struct args_struct, for ease of use.

    \param arg_count Number of command-line arguments, including the program
        execution (==argc from main()).
    \param arg_vector Array of command-line arguments, including the program
        execution (==argv from main()).

    \return struct args_struct, a structure of the parsed arguments.
*/
struct args_struct parse_args(int arg_count,
                              char **arg_vector);

/**
    Print the command line arguments' help to stdout.
*/
void print_help();

/**
    Pretty print a packet's metadata to stdout.

    \param mac_address String-encoded MAC address
    \param source_address String-encoded source IP address (v4 or v6)
    \param dest_address String-encoded destination IP address (v4 or v6)
    \param source_port uint16_t-encoded source TCP/UDP port
    \param dest_port uint16_t-encoded destination TCP/UDP port
*/
void pprint_packet(std::string mac_address,
                  std::string source_address,
                  std::string dest_address,
                  uint16_t source_port,
                  uint16_t dest_port);

/**
    Log a packet into the device_log and conn_log.

    \param ldata Pointer to packet's nflog metadata.
*/
static int log_packet(struct nflog_data *ldata,
                      conn_log connections,
                      device_log devices);

/**
    Establish a callback function for the packet's data.
*/
static int cb(struct nflog_g_handle *gh,
              struct nfgenmsg *nfmsg,
              struct nflog_data *nfa,
              void *data);

/**
    Start (or restart) EDICT's device and connection logging.
*/
int start_edict(conn_log connections,
                device_log devices);

/**
    Query EDICT's logs for a specific TCP/UDP connection, defined in an args_struct.

    \param devices Device log cache, as stored as a std::map<std::string, struct device_log_entry>,
        where the std::string key is a MAC address.
    \param args struct args_struct containing the metadata to search for
*/
std::map<std::string, struct device_log_entry> query_edict(conn_log connections,
                                                           device_log devices,
                                                           struct args_struct args);

/**
    Check EDICT's connection log for a specific IPv4 connection.

    \param devices Device log cache, as stored as a std::map<std::string, struct device_log_entry>,
        where the std::string key is a MAC address.
    \param timestamp time_t-encoded timestamp (in UTC, if applicable) of connection to check
    \param source_port uint16_t-encoded TCP/UDP source port of connection to check

    \return std::map of (MAC_address, device_log_entry) of all matching connections
*/
std::map<std::string, struct device_log_entry> check_ipv4(conn_log connections,
                                                          std::map<std::string, struct device_log_entry> devices,
                                                          time_t timestamp,
                                                          uint16_t source_port);

/**
    Check EDICT's connection log for a specific IPv6 connection.

    \param devices Device log cache, as stored as a std::map<std::string, struct device_log_entry>,
        where the std::string key is a MAC address.
    \param timestamp time_t-encoded timestamp (in UTC, if applicable) of connection to check
    \param ipv6_address std::string-encoded source IPv6 address of connection to check

    \return std::map of (MAC_address, device_log_entry) of all matching connections
*/
std::map<std::string, struct device_log_entry> check_ipv6(conn_log connections,
                                                          std::map<std::string, struct device_log_entry> devices,
                                                          time_t timestamp,
                                                          std::string ipv6_address);

/**
    Print a query's results, formatted properly.

    \param results Map of query results, returned from query_edict().
    \param format std::string-encoded format for printing. Current options:
        "plain" - plaintext, printed to stdout (suitable for command line viewing)
        "xml" - XML format, printed to stdout (suitable for piping to HTML server, etc)
*/
void print_results(std::map<std::string, struct device_log_entry> results,
                   std::string format);
