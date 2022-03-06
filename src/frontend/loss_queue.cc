/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <limits>

#include "loss_queue.hh"
#include "timestamp.hh"

using namespace std;

LossQueue::LossQueue()
    : prng_( random_device()() )
{}

void LossQueue::read_packet( const string & contents )
{
    if ( not drop_packet( contents ) ) {
        packet_queue_.emplace( contents );
    }
}

void LossQueue::write_packets( FileDescriptor & fd )
{
    while ( not packet_queue_.empty() ) {
        fd.write( packet_queue_.front() );
        packet_queue_.pop();
    }
}

unsigned int LossQueue::wait_time( void )
{
    return packet_queue_.empty() ? numeric_limits<uint16_t>::max() : 0;
}

bool IIDLoss::drop_packet( const string & packet __attribute((unused)) )
{
    return drop_dist_( prng_ );
}

EveryNDrop::EveryNDrop( const int every_n )
        : every_n_(every_n),
          rolling_counter_(0)
{}

bool EveryNDrop::drop_packet( const string & packet __attribute((unused)) )
{
    rolling_counter_ += 1;
    if ( (every_n_ > 0) and (rolling_counter_ >= every_n_) ) {
        rolling_counter_ = 0;
        return true;
    } else {
        return false;
    }
}

EveryNCorrupt::EveryNCorrupt( const int every_n, const bool chk_ok )
        : every_n_(every_n),
          chk_ok_(chk_ok),
          rolling_counter_(0)
{}

bool EveryNCorrupt::drop_packet( const string & packet __attribute((unused)) )
{
    rolling_counter_ += 1;
    if ( ( every_n_ > 0) and (rolling_counter_ >= every_n_ ) )
        rolling_counter_ = 0;
    return false;
}

void EveryNCorrupt::write_packets( FileDescriptor & fd )
{
    while ( not packet_queue_.empty() ) {
        std::string packet = packet_queue_.front();
        if ( ( every_n_ > 0 ) and ( rolling_counter_ == 0 ) ) {
            /* corrupt the packet */
            if ( packet.length() > 4 ) {
                /* Need at least 4 bytes to corrupt */
                int last_index  = packet.length() - 1;
                int penul_index = last_index - 2;
                packet[last_index] -= 1;
                if ( chk_ok_ ) // corrupt but preserve chksum
                    packet[penul_index] += 1;
            }
        }
        fd.write( packet );
        packet_queue_.pop();
    }
}

static const double MS_PER_SECOND = 1000.0;

SwitchingLink::SwitchingLink( const double mean_on_time, const double mean_off_time )
    : link_is_on_( false ),
      on_process_( 1.0 / (MS_PER_SECOND * mean_off_time) ),
      off_process_( 1.0 / (MS_PER_SECOND * mean_on_time) ),
      next_switch_time_( timestamp() )
{}

uint64_t bound( const double x )
{
    if ( x > (1 << 30) ) {
        return 1 << 30;
    }

    return x;
}

unsigned int SwitchingLink::wait_time( void )
{
    const uint64_t now = timestamp();

    while ( next_switch_time_ <= now ) {
        /* switch */
        link_is_on_ = !link_is_on_;
        /* worried about integer overflow when mean time = 0 */
        next_switch_time_ += bound( (link_is_on_ ? off_process_ : on_process_)( prng_ ) );
    }

    if ( LossQueue::wait_time() == 0 ) {
        return 0;
    }

    if ( next_switch_time_ - now > numeric_limits<uint16_t>::max() ) {
        return numeric_limits<uint16_t>::max();
    }

    return next_switch_time_ - now;
}

bool SwitchingLink::drop_packet( const string & packet __attribute((unused)) )
{
    return !link_is_on_;
}
