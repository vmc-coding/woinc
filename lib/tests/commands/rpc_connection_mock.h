/* tests/commands/rpc_connection_mock.h --
   Written and Copyright (C) 2017 by vmc.

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

#ifndef WOINC_TESTS_RPC_CONNECTION_MOCK_H_
#define WOINC_TESTS_RPC_CONNECTION_MOCK_H_

#include <woinc/rpc_connection.h>
#include <woinc/rpc_command.h>

#include "../../src/xml.h"

#include "xml_helpers.h"

woinc::xml::Tree create_response_tree() {
    return woinc::xml::Tree("boinc_gui_rpc_reply");
}

struct ConnectionMockStub : public woinc::rpc::Connection {
    virtual ~ConnectionMockStub() = default;

    woinc::rpc::Connection::Result do_rpc(const std::string &request, std::ostream &response) final {
        woinc::xml::Tree request_tree;

        // basic request check

        std::stringstream request_stream(request);
        std::string error;

        if (!request_tree.parse(request_stream, error)) {
            std::cerr << "ConnectionMockStub::parse(): Could not parse request:\n" << request;
            exit(EXIT_FAILURE);
        }

        if (request_tree.root.tag != "boinc_gui_rpc_request") {
            std::cerr << "ConnectionMockStub::parse(): 'boinc_gui_rpc_request' tag not found:\n" << request;
            exit(EXIT_FAILURE);
        }

        // let the individual tests mock the rpc call
        return mock_do_rpc(request_tree, response);
    }

    protected:
        virtual woinc::rpc::Connection::Result mock_do_rpc(
            woinc::xml::Tree &request_tree, std::ostream &response) = 0;
};

#endif
