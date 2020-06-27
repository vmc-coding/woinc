/* libui/src/client.cc --
   Written and Copyright (C) 2017-2019 by vmc.

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

#include "client.h"

namespace woinc { namespace ui {

Client::~Client() {
    disconnect();
}

bool Client::connect(const std::string &host, std::uint16_t port) {
    disconnect();

    host_ = host;
    connected_ = rpc_connection_.open(host, port);

    return connected_;
}

void Client::disconnect() {
    if (connected_) {
        rpc_connection_.close();
        connected_ = false;
    }
}

woinc::rpc::COMMAND_STATUS Client::execute(woinc::rpc::Command &cmd) {
    if (connected_)
        return cmd.execute(rpc_connection_);
    else
        return woinc::rpc::COMMAND_STATUS::DISCONNECTED;
}

const std::string &Client::host() const {
    return host_;
}

}}
