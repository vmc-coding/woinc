/* tests/commands/generic_command_tests.h --
   Written and Copyright (C) 2017, 2019 by vmc.

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

#ifndef WOINC_TESTS_GENERIC_COMMAND_TESTS_H_
#define WOINC_TESTS_GENERIC_COMMAND_TESTS_H_

#include <memory>

#include "rpc_connection_mock.h"
#include "xml_helpers.h"

namespace test {

// these functions have to be defined by the command tests

static std::string get_request_tag();
static std::string get_response_tag();

static woinc::rpc::Command *create_valid_command();
static void validate_request_cmd_node(woinc::xml::Node &cmd_node);

static void create_positive_response(std::ostream &response);
static void validate_positive_response(woinc::rpc::Command*);

}

namespace test { namespace commands {

// these functions may be used by the command tests

static void test_positive();
static void test_not_boinc_response();
static void test_wrong_cmd_response();

// ----------------------------------------------------------------

namespace positive {

struct ConnectionMock : public ConnectionMockStub {
    virtual ~ConnectionMock() = default;
    woinc::rpc::Connection::Result mock_do_rpc(woinc::xml::Tree &request_tree, std::ostream &response) final {
        // check nodes in the request

        auto &cmd_node = need_cmd_node(request_tree.root, get_request_tag());

        validate_request_cmd_node(cmd_node);

        // all ok, send response

        create_positive_response(response);

        return woinc::rpc::Connection::Result();
    }
};

void doit() {
    ConnectionMock connection;
    std::unique_ptr<woinc::rpc::Command> cmd(create_valid_command());

    assert_equals("Executing the command failed: " + cmd->error(),
                  cmd->execute(connection),
                  woinc::rpc::COMMAND_STATUS::OK);

    validate_positive_response(cmd.get());
}

}

void test_positive() {
    positive::doit();
}

// ----------------------------------------------------------------

namespace not_boinc_response {

struct ConnectionMock : public ConnectionMockStub {
    virtual ~ConnectionMock() = default;
    woinc::rpc::Connection::Result mock_do_rpc(woinc::xml::Tree &request_tree, std::ostream &response) final {
        // check nodes in the request

        woinc::xml::Node &cmd_node = need_cmd_node(request_tree.root, get_request_tag());

        validate_request_cmd_node(cmd_node);

        // all ok, send response with invalid BOINC root tag

        response <<
            "<boinc_gui_rpc_reply_not>\n"\
            "  <" << get_response_tag() << "/>\n"\
            "</boinc_gui_rpc_reply_not>\n";

        return woinc::rpc::Connection::Result();
    }
};

void doit() {
    ConnectionMock connection;
    std::unique_ptr<woinc::rpc::Command> cmd(create_valid_command());

    assert_true("Command accepted broken response",
                cmd->execute(connection) == woinc::rpc::COMMAND_STATUS::PARSING_ERROR);
}

}

void test_not_boinc_response() {
    not_boinc_response::doit();
}


// ----------------------------------------------------------------

namespace wrong_cmd_response {

struct ConnectionMock : public ConnectionMockStub {
    virtual ~ConnectionMock() = default;
    woinc::rpc::Connection::Result mock_do_rpc(woinc::xml::Tree &request_tree, std::ostream &response) final {
        // check nodes in the request

        woinc::xml::Node &cmd_node = need_cmd_node(request_tree.root, get_request_tag());

        validate_request_cmd_node(cmd_node);

        // all ok, send response with wrong command node

        response <<
            "<boinc_gui_rpc_reply>\n"\
            "  <" << get_response_tag() << "_not/>\n"\
            "</boinc_gui_rpc_reply>\n";

        return woinc::rpc::Connection::Result();
    }
};

void doit() {
    ConnectionMock connection;
    std::unique_ptr<woinc::rpc::Command> cmd(create_valid_command());

    assert_true("Command accepted broken response",
                cmd->execute(connection) == woinc::rpc::COMMAND_STATUS::PARSING_ERROR);
}

}

void test_wrong_cmd_response() {
    wrong_cmd_response::doit();
}

}}

#endif
