/* lib/rpc_connection.cc --
   Written and Copyright (C) 2017-2021 by vmc.

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

#include <woinc/rpc_connection.h>

#include <cassert>
#include <limits>
#include <sstream>

#ifdef WOINC_LOG_RPC_CONNECTION
#include <iostream>
#endif

#include "socket.h"
#include "visibility.h"

namespace {
    const char EOM = 0x03;
    enum { BUFFER_SIZE = 32*1024 };
}

namespace woinc { namespace rpc {

// ---- Connection::Impl ----

class WOINC_LOCAL Connection::Impl {
    public:
        Connection::Result open(const std::string &hostname, std::uint16_t port);
        void close();

        Connection::Result do_rpc(const std::string &request, std::ostream &response);

        bool is_localhost() const;

    private:
        std::unique_ptr<woinc::Socket> socket_;
        bool connected_ = false;
};

Connection::Result Connection::Impl::open(const std::string &hostname, std::uint16_t port) {
    if (connected_)
        close();

    // let the network stack decide which version to use

    socket_ = Socket::create(Socket::VERSION::ALL);

    if (socket_ && socket_->connect(hostname, port))
        return Result();

    // network stack doesn't support VERSION::ALL, so let't try which one to use

    socket_ = Socket::create(Socket::VERSION::IPv6);

    std::string error_msg;

    if (socket_.get() != nullptr) {
        Socket::Result result_connect = socket_->connect(hostname, port);
        if (result_connect)
            return Result();
        error_msg = std::move(result_connect.error);
    }

    socket_ = Socket::create(Socket::VERSION::IPv4);

    if (socket_.get() != nullptr) {
        Socket::Result result_connect = socket_->connect(hostname, port);
        if (result_connect)
            return Result();
        error_msg = std::move(result_connect.error);
    }

    return Result(CONNECTION_STATUS::ERROR, std::move(error_msg));
}

void Connection::Impl::close() {
    if (connected_)
        socket_->close();
    connected_ = false;
}

Connection::Result Connection::Impl::do_rpc(const std::string &request, std::ostream &response) {
    char buffer[BUFFER_SIZE];

#ifdef WOINC_LOG_RPC_CONNECTION
    std::cerr << "------------- REQUEST ------------\n"
        << request
        << "------------- END REQUEST ------------\n";
#endif

    {
        Socket::Result result = socket_->send(request.c_str(), request.size());
        if (!result)
            return Result(CONNECTION_STATUS::ERROR, std::move(result.error));

        result = socket_->send(&EOM, sizeof(EOM));
        if (!result)
            return Result(CONNECTION_STATUS::ERROR, std::move(result.error));
    }

#ifdef WOINC_LOG_RPC_CONNECTION
    std::cerr << "------------- RESPONSE ------------\n";
#endif

    bool eom = false;
    while (!eom) {
        size_t bytes_read = 0;

        {
            Socket::Result result = socket_->receive(buffer, BUFFER_SIZE, bytes_read);
            if (!result)
                return Result(CONNECTION_STATUS::ERROR, std::move(result.error));
        }

        if (bytes_read == 0)
            return Result(CONNECTION_STATUS::DISCONNECTED);

#ifdef WOINC_LOG_RPC_CONNECTION
        std::cerr.write(buffer, bytes_read);
#endif

        if (bytes_read > static_cast<size_t>(std::numeric_limits<std::streamsize>::max()))
            return Result(CONNECTION_STATUS::ERROR, "Ouch");
        auto to_write = static_cast<std::streamsize>(bytes_read);
        assert(to_write > 0);

        if ((eom = (buffer[to_write - 1] == EOM)))
            to_write --;

        if (!response.write(buffer, to_write))
            return Result(CONNECTION_STATUS::ERROR);
    }

#ifdef WOINC_LOG_RPC_CONNECTION
    std::cerr << "------------- END RESPONSE ------------" << std::endl;
#endif

    return Result();
}

bool Connection::Impl::is_localhost() const {
    return socket_->is_localhost();
}

// ---- Connection ----

Connection::Connection()
    : impl_(std::make_unique<Impl>())
{}

Connection::~Connection() {
    close();
}

Connection::Result Connection::open(const std::string &hostname, std::uint16_t port) {
    return impl_->open(hostname, port);
}

void Connection::close() {
    impl_->close();
}

Connection::Result Connection::do_rpc(const std::string &request, std::ostream &response) {
    return impl_->do_rpc(request, response);
}

bool Connection::is_localhost() const {
    return impl_->is_localhost();
}

}}
