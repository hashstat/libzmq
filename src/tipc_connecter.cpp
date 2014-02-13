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

#include "tipc_connecter.hpp"

#if defined ZMQ_HAVE_TIPC

#include <new>
#include <string>

#include "stream_engine.hpp"
#include "io_thread.hpp"
#include "platform.hpp"
#include "err.hpp"
#include "ip.hpp"
#include "address.hpp"
#include "tipc_address.hpp"
#include "session_base.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

int zmq::tipc_connecter_t::open ()
{
    zmq_assert (s == retired_fd);

    //  Create the socket.
    s = open_socket (AF_TIPC, SOCK_STREAM, 0);
    if (s == -1)
        return -1;

    //  Set the non-blocking flag.
    unblock_socket (s);
    //  Connect to the remote peer.
    int rc = ::connect (
      s, addr->resolved.tipc_addr->addr (),
      addr->resolved.tipc_addr->addrlen ());

    //  Connect was successfull immediately.
    if (rc == 0)
        return 0;

    //  Translate other error codes indicating asynchronous connect has been
    //  launched to a uniform EINPROGRESS.
    if (rc == -1 && errno == EINTR) {
        errno = EINPROGRESS;
        return -1;
    }
    //  Forward the error.
    return -1;
}

void zmq::tipc_connecter_t::close ()
{
    zmq_assert (s != retired_fd);
    int rc = ::close (s);
    errno_assert (rc == 0);
    socket->event_closed (endpoint, s);
    s = retired_fd;
}

zmq::fd_t zmq::tipc_connecter_t::connect ()
{
    //  Following code should handle both Berkeley-derived socket
    //  implementations and Solaris.
    int err = 0;
    socklen_t len = sizeof (err);

    int rc = getsockopt (s, SOL_SOCKET, SO_ERROR, (char*) &err, &len);
    if (rc == -1)
        err = errno;
    if (err != 0) {

        //  Assert if the error was caused by 0MQ bug.
        //  Networking problems are OK. No need to assert.
        errno = err;
        errno_assert (errno == ECONNREFUSED || errno == ECONNRESET ||
            errno == ETIMEDOUT || errno == EHOSTUNREACH ||
            errno == ENETUNREACH || errno == ENETDOWN);

        return retired_fd;
    }
    fd_t result = s;
    s = retired_fd;
    return result;
}

#endif

