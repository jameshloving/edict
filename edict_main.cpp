//=============================================================================
//
// Name:        edict.cpp
// Authors:     James H. Loving
// Description: This file, along with edict_main.cpp, defines the command line
//              interface for EDICT, as well as its principal functions:
//              starting logging, stopping logging, and querying logs.
//              This file was split from edict_main.cpp to support unit
//              testing of the contained functions.
//
//=============================================================================

#include "libs/conn_log/conn_log.hpp"
#include "libs/device_log/device_log.hpp"
#include "edict.hpp"

int main(int argc, char **argv)
{
    struct args_struct args = parse_args(argc, argv);   
    
    if (args.command == "start")
    {
        conn_log connections;
        device_log devices;
        start_edict(connections, devices); 
    } 
    else if (args.command == "query")
    {
        conn_log connections;
        device_log devices;

        std::cout << "format: " << args.print_format << "\n";

        print_results(query_edict(connections, devices, args),
                      args.print_format);
    }
    else if (args.command == "help")
    {
        print_help();
    }
    else
    {
        std::cout << "Invalid command.\n";
        print_help();
    }

	return 0;
}
