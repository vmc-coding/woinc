/* woinc/rpc_connection.h --
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

#ifndef WOINC_RPC_CONNECTION_H_
#define WOINC_RPC_CONNECTION_H_

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>

#include <woinc/defs.h>

namespace woinc { namespace rpc {

enum class ConnectionStatus {
    Ok,
    Disconnected,
    Error
};

class Connection {
    public:
        enum { DefaultBOINCPort = 31416 };

        struct Result {
            ConnectionStatus status;
            std::string error;

            explicit Result(ConnectionStatus s = ConnectionStatus::Ok, std::string err = "")
                : status(s), error(std::move(err)) {}

            operator bool() const {
                return status == ConnectionStatus::Ok;
            }
        };

    public:
        Connection();
        virtual ~Connection();

        virtual Result open(const std::string &hostname, std::uint16_t port = DefaultBOINCPort);
        virtual void close();

        virtual Result do_rpc(const std::string &request, std::ostream &response);

        virtual bool is_localhost() const;

    protected:
        struct Impl;
        std::unique_ptr<Impl> impl_;
};

}}

#endif
