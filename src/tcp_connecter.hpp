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

#ifndef __TCP_CONNECTER_HPP_INCLUDED__
#define __TCP_CONNECTER_HPP_INCLUDED__

#include "stream_connecter.hpp"

namespace zmq
{

    class tcp_connecter_t : public stream_connecter_t
    {
    public:
        tcp_connecter_t (zmq::io_thread_t *io_thread_,
            zmq::session_base_t *session_, const options_t &options_,
            const address_t *addr_, bool delayed_start_) :
            stream_connecter_t ("tcp", io_thread_, session_, options_,
                    addr_, delayed_start_) {}

    private:
        void tune (fd_t fd_);
        int open ();
        void close ();
        fd_t connect ();

        tcp_connecter_t (const tcp_connecter_t&);
        const tcp_connecter_t &operator = (const tcp_connecter_t&);
    };

}

#endif
