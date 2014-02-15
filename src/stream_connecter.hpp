/*
    Copyright (c) 2007-2014 Contributors as noted in the AUTHORS file

    This file is part of 0MQ.

    0MQ is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    0MQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __STREAM_CONNECTER_HPP_INCLUDED__
#define __STREAM_CONNECTER_HPP_INCLUDED__

#include "fd.hpp"
#include "own.hpp"
#include "stdint.hpp"
#include "io_object.hpp"
#include "../include/zmq.h"

namespace zmq
{

    class io_thread_t;
    class session_base_t;
    struct address_t;

    class stream_connecter_t : public own_t, public io_object_t
    {
    public:

        //  If 'reconnect_ivl' is not zero, wait before connecting.
        stream_connecter_t (const char *protocol_name_,
            zmq::io_thread_t *io_thread_, zmq::session_base_t *session_,
            const options_t &options_, const address_t *addr_,
            int reconnect_ivl_);
        ~stream_connecter_t ();

        //  Will modify the current_ivl used for next call.
        //  Returns the currently used interval.
        static int calc_new_reconnect_ivl (
            int &current_ivl, int base_ivl, int max_ivl);

    protected:

        //  Address to connect to. Owned by session_base_t.
        const address_t *addr;

        //  Underlying socket.
        fd_t s;

        // String representation of endpoint to connect to
        std::string endpoint;

        // Socket
        zmq::socket_base_t *socket;

    private:

        //  ID of the timer used to delay the reconnection.
        enum {reconnect_timer_id = 1};

        //  Handlers for incoming commands.
        void process_plug ();
        void process_term (int linger_);

        //  Handlers for I/O events.
        void in_event ();
        void tune (fd_t fd_);
        void out_event ();
        void timer_event (int id_);

        //  Internal function to start the actual connection establishment.
        void start_connecting ();

        //  Internal function to add a reconnect timer
        void add_reconnect_timer();

        //  Open connecting socket. Returns -1 in case of error,
        //  0 if connect was successfull immediately. Returns -1 with
        //  EAGAIN errno if async connect was launched.
        virtual int open () = 0;

        //  Close the connecting socket.
        virtual void close () = 0;

        //  Get the file descriptor of newly created connection. Returns
        //  retired_fd if the connection was unsuccessfull.
        virtual fd_t connect () = 0;

        //  Handle corresponding to the listening socket.
        handle_t handle;

        //  If true file descriptor is registered with the poller and 'handle'
        //  contains valid value.
        bool handle_valid;

        //  True iff a timer has been started.
        bool timer_started;

        //  Reference to the session we belong to.
        zmq::session_base_t *session;

        //  Current reconnect ivl, updated for backoff strategy
        int current_reconnect_ivl;
    };

}

#endif
