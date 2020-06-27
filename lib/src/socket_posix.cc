/* lib/socket_posix.cc --
   Written and Copyright (C) 2017, 2018 by vmc.

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

#include "socket.h"

#ifdef WOINC_USE_POSIX_SOCKETS

extern "C" {
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
} // extern "C"

#include <cassert>
#include <cerrno>
#include <cstring>

#ifndef NDEBUG
#include <iostream>
#endif

namespace {

}

namespace woinc {

Socket::Socket(int version) : version_(version) {}

Socket::~Socket() {
    close();
}

Socket::Result Socket::connect(const std::string &host, std::uint16_t port) {
    if (connected_)
        return Result(STATUS::ALREADY_CONNECTED);

    //unsigned char buf[sizeof(struct in6_addr)];

    //if (inet_pton(version, host, buf) == 1) {
        //if (::connect(socket_, rp->ai_addr, rp->ai_addrlen) == -1) {
            //::close(socket_);
            //continue;
        //}
    //}

    // resolve a list of IPs for given host (see man 3 getaddrinfo)

    addrinfo hints;
    addrinfo *result;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = version_;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int resolving_status = ::getaddrinfo(host.c_str(), NULL, &hints, &result);

    if (resolving_status != 0)
        return Result (
            STATUS::RESOLVING_ERROR,
            resolving_status == EAI_SYSTEM ? strerror(errno) : gai_strerror(resolving_status)
        );

    std::cout << "Huhu\n";


    // we got a list of addresses to connect to, try to connect to one of them

    for (auto *rp = result; rp != nullptr; rp = rp->ai_next) {

        if ((socket_ = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1)
            continue;

        std::cout << "have socket\n";

        if (rp->ai_family == AF_INET) {
            sockaddr_in *addr_in = reinterpret_cast<sockaddr_in *>(rp->ai_addr);
            addr_in->sin_port = htons(port);
        } else {
            assert(rp->ai_family == AF_INET6);
            sockaddr_in6 *addr_in = reinterpret_cast<sockaddr_in6 *>(rp->ai_addr);
            addr_in->sin6_port = htons(port);
        }

        if (::connect(socket_, rp->ai_addr, rp->ai_addrlen) == -1) {
            perror("asd");
            ::close(socket_);
            continue;
        }

        char addr[64];
        if (rp->ai_family == AF_INET) {
            sockaddr_in *addr_in = reinterpret_cast<sockaddr_in *>(rp->ai_addr);
            if (::inet_ntop(AF_INET, &addr_in->sin_addr, addr, sizeof(addr)))
                is_localhost_ = std::string(addr) == "127.0.0.1";
        } else {
            assert(rp->ai_family == AF_INET6);
            sockaddr_in6 *addr_in = reinterpret_cast<sockaddr_in6 *>(rp->ai_addr);
            if (::inet_ntop(AF_INET6, &addr_in->sin6_addr, addr, sizeof(addr)))
                is_localhost_ = std::string(addr) == "::1";
        }

        connected_ = true;
        break;
    }

    ::freeaddrinfo(result);

    if (!connected_) {
        return Result(STATUS::SOCKET_ERROR, "Could not connect to " + host);
    } else {
        // set 10s timeouts for reads and writes
        timeval timeout;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;

        setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        return Result();
    }
}

void Socket::close() {
    if (connected_) {
        // we ignore the return value here, because we can't do anything anyway
        ::close(socket_);
        connected_ = false;
    }
}

Socket::Result Socket::send(const void *data, std::size_t length) {
    if (!connected_)
        return Result(STATUS::NOT_CONNECTED);

    ssize_t bytes_sent = ::send(socket_, data, length, MSG_NOSIGNAL);

    if (bytes_sent < 0)
        return Result(STATUS::SOCKET_ERROR, strerror(errno));

    if (static_cast<size_t>(bytes_sent) != length)
        return Result(STATUS::SOCKET_ERROR);

    return Result();
}

Socket::Result Socket::receive(void *buffer, std::size_t max_length, std::size_t &bytes_read) {
    if (!connected_)
        return Result(STATUS::NOT_CONNECTED);

    ssize_t read = ::recv(socket_, buffer, max_length, 0);

    if (read < 0)
        return Result(STATUS::SOCKET_ERROR, strerror(errno));

    bytes_read = static_cast<size_t>(read);
    return Result();
}

bool Socket::is_localhost() const {
    return is_localhost_;
}

Socket *Socket::create(Socket::VERSION v) {
    switch (v) {
        case VERSION::ALL:
            std::cout << "Create ALL\n";
            return new Socket(AF_UNSPEC);
        case VERSION::IPv4:
            std::cout << "Create v4\n";
            return new Socket(AF_INET);
        case VERSION::IPv6:
            std::cout << "Create v6\n";
            return new Socket(AF_INET6);
        /* no default to get warnings on compile time when VERSION has been changed */
    }
    assert(false);
    return nullptr;
}

}

#endif // WOINC_USE_POSIX_SOCKETS

