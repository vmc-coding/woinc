/* libui/src/client.h --
   Written and Copyright (C) 2017-2022 by vmc.

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

#ifndef WOINC_UI_CLIENT_H_
#define WOINC_UI_CLIENT_H_

#include <string>
#include <cstdint>

#include <woinc/rpc_command.h>
#include <woinc/rpc_connection.h>

#include "visibility.h"

namespace woinc { namespace ui {

// The client is not threadsafe! Should only be called by the worker thread for this host.
class WOINCUI_LOCAL Client {
    public:
        ~Client();

    public:
        bool connect(std::string host, std::uint16_t port);
        bool authorize(const std::string &password);
        void disconnect();

        woinc::rpc::CommandStatus execute(woinc::rpc::Command &cmd);

        const std::string &host() const;

    private:
        bool connected_ = false;
        std::string host_;
        woinc::rpc::Connection rpc_connection_;
};

}}

#endif
