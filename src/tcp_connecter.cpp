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

#include <new>
#include <string>

#include "tcp_connecter.hpp"
#include "stream_engine.hpp"
#include "io_thread.hpp"
#include "platform.hpp"
#include "err.hpp"
#include "ip.hpp"
#include "tcp.hpp"
#include "address.hpp"
#include "tcp_address.hpp"
#include "session_base.hpp"

#if defined ZMQ_HAVE_WINDOWS
#include "windows.hpp"
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#ifdef ZMQ_HAVE_OPENVMS
#include <ioctl.h>
#endif
#endif


void zmq::tcp_connecter_t::tune (fd_t fd_)
{
    tune_tcp_socket (fd_);
    tune_tcp_keepalives (fd_, options.tcp_keepalive, options.tcp_keepalive_cnt, options.tcp_keepalive_idle, options.tcp_keepalive_intvl);

    // remember our fd for ZMQ_SRCFD in messages
    socket->set_fd(fd_);
}

int zmq::tcp_connecter_t::open ()
{
    zmq_assert (s == retired_fd);

    //  Create the socket.
    s = open_socket (addr->resolved.tcp_addr->family (), SOCK_STREAM, IPPROTO_TCP);
#ifdef ZMQ_HAVE_WINDOWS
    if (s == INVALID_SOCKET) {
        errno = wsa_error_to_errno (WSAGetLastError ());
        return -1;
    }
#else
    if (s == -1)
        return -1;
#endif

    //  On some systems, IPv4 mapping in IPv6 sockets is disabled by default.
    //  Switch it on in such cases.
    if (addr->resolved.tcp_addr->family () == AF_INET6)
        enable_ipv4_mapping (s);

    // Set the IP Type-Of-Service priority for this socket
    if (options.tos != 0)
        set_ip_type_of_service (s, options.tos);

    // Set the socket to non-blocking mode so that we get async connect().
    unblock_socket (s);

    //  Set the socket buffer limits for the underlying socket.
    if (options.sndbuf != 0)
        set_tcp_send_buffer (s, options.sndbuf);
    if (options.rcvbuf != 0)
        set_tcp_receive_buffer (s, options.rcvbuf);

    // Set the IP Type-Of-Service for the underlying socket
    if (options.tos != 0)
        set_ip_type_of_service (s, options.tos);

    //  Connect to the remote peer.
    int rc = ::connect (
        s, addr->resolved.tcp_addr->addr (),
        addr->resolved.tcp_addr->addrlen ());

    //  Connect was successfull immediately.
    if (rc == 0)
        return 0;

    //  Translate error codes indicating asynchronous connect has been
    //  launched to a uniform EINPROGRESS.
#ifdef ZMQ_HAVE_WINDOWS
    const int error_code = WSAGetLastError ();
    if (error_code == WSAEINPROGRESS || error_code == WSAEWOULDBLOCK)
        errno = EINPROGRESS;
    else
        errno = wsa_error_to_errno (error_code);
#else
    if (errno == EINTR)
        errno = EINPROGRESS;
#endif
    return -1;
}

zmq::fd_t zmq::tcp_connecter_t::connect ()
{
    //  Async connect has finished. Check whether an error occurred
    int err = 0;
#if defined ZMQ_HAVE_HPUX
    int len = sizeof (err);
#else
    socklen_t len = sizeof (err);
#endif

    int rc = getsockopt (s, SOL_SOCKET, SO_ERROR, (char*) &err, &len);

    //  Assert if the error was caused by 0MQ bug.
    //  Networking problems are OK. No need to assert.
#ifdef ZMQ_HAVE_WINDOWS
    zmq_assert (rc == 0);
    if (err != 0) {
        if (err == WSAECONNREFUSED ||
            err == WSAETIMEDOUT ||
            err == WSAECONNABORTED ||
            err == WSAEHOSTUNREACH ||
            err == WSAENETUNREACH ||
            err == WSAENETDOWN ||
            err == WSAEACCES ||
            err == WSAEINVAL)
            return retired_fd;
        wsa_assert_no (err);
    }
#else

    //  Following code should handle both Berkeley-derived socket
    //  implementations and Solaris.
    if (rc == -1)
        err = errno;
    if (err != 0) {
        errno = err;
        errno_assert (
            errno == ECONNREFUSED ||
            errno == ECONNRESET ||
            errno == ETIMEDOUT ||
            errno == EHOSTUNREACH ||
            errno == ENETUNREACH ||
            errno == ENETDOWN ||
            errno == EINVAL);
        return retired_fd;
    }
#endif

    //  Return the newly connected socket.
    fd_t result = s;
    s = retired_fd;
    return result;
}

void zmq::tcp_connecter_t::close ()
{
    zmq_assert (s != retired_fd);
#ifdef ZMQ_HAVE_WINDOWS
    int rc = closesocket (s);
    wsa_assert (rc != SOCKET_ERROR);
#else
    int rc = ::close (s);
    errno_assert (rc == 0);
#endif
    socket->event_closed (endpoint, s);
    s = retired_fd;
}
