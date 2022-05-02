/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <vector>
#include <string>
#include <iostream>

#include <getopt.h>
#include <stdio.h>

#include "loss_queue.hh"
#include "util.hh"
#include "ezio.hh"
#include "packetshell.cc"

using namespace std;

void usage( const string & program_name )
{
    throw runtime_error( "Usage: " + program_name + " uplink|downlink TRACEDROP [COMMAND...]" );
}

int main( int argc, char *argv[] )
{
    try {
        /* clear environment while running as root */
        char ** const user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc < 3 ) {
            usage( argv[ 0 ] );
        }

        ifstream trace_file( argv[ 2 ] );
        if ( trace_file.is_open() == false ) {
            cerr << "Error: Unable to open the trace file." << endl;
            usage( argv[ 0 ] );
        }

        string uplink_loss = "";
        string downlink_loss = "";

        const string link = argv[ 1 ];
        if ( link == "uplink" ) {
            uplink_loss = argv[ 2 ];
        } else if ( link == "downlink" ) {
            downlink_loss = argv[ 2 ];
        } else {
            usage( argv[ 0 ] );
        }

        vector<string> command;

        if ( argc == 3 ) {
            command.push_back( shell_path() );
        } else {
            for ( int i = 3; i < argc; i++ ) {
                command.push_back( argv[ i ] );
            }
        }

        PacketShell<TraceDrop> drop_app( "drop", user_environment );

        string shell_prefix = "[drop ";
        if ( link == "uplink" ) {
            shell_prefix += "up=";
        } else {
            shell_prefix += "down=";
        }
        shell_prefix += argv[ 2 ];
        shell_prefix += "] ";

        drop_app.start_uplink( shell_prefix,
                               command,
                               uplink_loss );
        drop_app.start_downlink( downlink_loss );
        return drop_app.wait_for_exit();
    } catch ( const exception & e ) {
        print_exception( e );
        return EXIT_FAILURE;
    }
}
