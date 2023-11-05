/* lib/socket.h --
   Written and Copyright (C) 2017-2023 by vmc.

   This file is part of woinc.

   woinc is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   woinc is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with woinc. If not, see <http://www.gnu.org/licenses/>. */

#ifndef WOINC_SOCKET_H_
#define WOINC_SOCKET_H_

#include <cstdint>
#include <memory>
#include <string>

#include "visibility.h"

// force the POSIX variant until we have alternative implementations
#ifndef WOINC_USE_POSIX_SOCKETS
#define WOINC_USE_POSIX_SOCKETS
#endif

namespace woinc {

// We only provide TCP sockets by this interface
struct WOINC_LOCAL Socket {
    public:
        enum class Version { All, IPv4, IPv6 };
        enum class Status { Ok, NotConnected, AlreadyConnected, ResolvingError, SocketError };

        struct Result {
            Status status;
            std::string error;

            explicit Result(Status s = Status::Ok, std::string err = "")
                : status(s), error(std::move(err)) {}

            operator bool() const {
                return status == Status::Ok;
            }
        };

#ifdef WOINC_USE_POSIX_SOCKETS
    protected:
        explicit Socket(int version);
#endif

    public:
        ~Socket();

        Socket(const Socket &) = delete;
        Socket &operator=(const Socket &) = delete;

    public:
        Result connect(const std::string &host, std::uint16_t port);
        void close();

        Result send(const void *data, std::size_t length);
        Result receive(void *buffer, std::size_t max_length, std::size_t &bytes_read);

        bool is_localhost() const;

    public:
        static std::unique_ptr<Socket> create(Version v);

#ifdef WOINC_USE_POSIX_SOCKETS
    private:
        const int version_;
        int socket_ = -1;

        bool connected_ = false;
        bool is_localhost_ = false;
#endif
};

}

#endif
