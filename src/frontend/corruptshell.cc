/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <vector>
#include <string>
#include <iostream>

#include <getopt.h>

#include "loss_queue.hh"
#include "util.hh"
#include "ezio.hh"
#include "packetshell.cc"

using namespace std;

void usage( const string & program_name )
{
    throw runtime_error( "Usage: " + program_name + " uplink|downlink chksumok|chknotok EVERY_N [COMMAND...]" );
}

int main( int argc, char *argv[] )
{
    try {
        /* clear environment while running as root */
        char ** const user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc < 4 ) {
            usage( argv[ 0 ] );
        }

        const double every_n = myatof( argv[ 3 ] );
        if ( 0 <= every_n ) {
            /* do nothing */
        } else {
            cerr << "Error: N must be non-negative." << endl;
            usage( argv[ 0 ] );
        }

        double uplink_loss = 0, downlink_loss = 0;

        const string link = argv[ 1 ];
        if ( link == "uplink" ) {
            uplink_loss = every_n;
        } else if ( link == "downlink" ) {
            downlink_loss = every_n;
        } else {
            usage( argv[ 0 ] );
        }

        bool chk_ok = false;
        const string chk_type = argv[ 2 ];
        if ( chk_type == "chksumok" ) {
          chk_ok = true;
        } else if ( chk_type == "chknotok" ) {
          chk_ok = false;
        } else {
          usage( argv[ 0 ] );
        }

        vector<string> command;

        if ( argc == 4 ) {
            command.push_back( shell_path() );
        } else {
            for ( int i = 4; i < argc; i++ ) {
                command.push_back( argv[ i ] );
            }
        }

        PacketShell<EveryNCorrupt> corrupt_app( "corrupt", user_environment );

        string shell_prefix = "[corrupt ";
        if ( link == "uplink" ) {
            shell_prefix += "up=";
        } else {
            shell_prefix += "down=";
        }
        shell_prefix += argv[ 2 ];
        shell_prefix += " ";
        if ( chk_ok ) {
          shell_prefix += "chksum_ok";
        } else {
          shell_prefix += "chksum_mangled";
        }
        shell_prefix += "] ";

        corrupt_app.start_uplink( shell_prefix,
                                  command,
                                  uplink_loss,
                                  chk_ok );
        corrupt_app.start_downlink( downlink_loss,
                                    chk_ok );
        return corrupt_app.wait_for_exit();
    } catch ( const exception & e ) {
        print_exception( e );
        return EXIT_FAILURE;
    }
}
