/* tests/commands/authorize_cmd_test.cc --
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

#include <cassert>

#include "../test.h"

#include "rpc_connection_mock.h"

static void test_mandatory_password();
static void test_positive_auth();
static void test_negative_auth();
static void test_client_error_auth1();
static void test_client_error_auth2();

void get_tests(Tests &tests) {
    tests["01 Test for mandatory password"] = test_mandatory_password;
    tests["02 Positive authentication"]     = test_positive_auth;
    tests["03 Negative authentication"]     = test_negative_auth;
    tests["04 Client error in auth1"]       = test_client_error_auth1;
    tests["05 Client error in auth2"]       = test_client_error_auth2;
}

using namespace woinc::rpc;
using namespace woinc::xml;

/*----------------------------------------------------------------------------*/

void test_mandatory_password() {
    Connection connection;
    AuthorizeCommand cmd;

    assert_equals("Command accepted empty password",
                  cmd.execute(connection),
                  woinc::rpc::COMMAND_STATUS::LOGIC_ERROR);
}

/*----------------------------------------------------------------------------*/

namespace test { namespace positive {

struct ConnectionMock : ConnectionMockStub {
    virtual ~ConnectionMock() = default;
    Result mock_do_rpc(Tree &request_tree, std::ostream &response) final override {
        Tree response_tree(create_response_tree());

        if (request_tree.root.has_child("auth1")) {
            response_tree.root["nonce"] = std::string("foobar");
            response << response_tree;

            return Result();
        }

        if (request_tree.root.has_child("auth2")) {
            Node &node = request_tree.root["auth2"];

            need_node_with_value(node, "nonce_hash", std::string("a1d4e05ab31d457ff58c59524b68d9e5"));

            response_tree.root["authorized"];
            response << response_tree;
            return Result();
        }

        std::cerr << "Missing auth-tags in request:\n" << request_tree << "\n";
        assert(false);
        return Result(woinc::rpc::CONNECTION_STATUS::ERROR);
    }
};

void doit() {
    ConnectionMock connection;
    AuthorizeCommand cmd;

    cmd.request().password = "some password";

    assert_equals("Executing the command failed: " + cmd.error(),
                  cmd.execute(connection),
                  woinc::rpc::COMMAND_STATUS::OK);

    assert_equals("Flag not set", cmd.response().authorized, true);
}

}}

void test_positive_auth() {
    test::positive::doit();
}

/*----------------------------------------------------------------------------*/

namespace test { namespace negative_auth {

struct ConnectionMock : ConnectionMockStub {
    virtual ~ConnectionMock() = default;
    Result mock_do_rpc(Tree &request_tree, std::ostream &response) final override {
        Tree response_tree(create_response_tree());

        if (request_tree.root.has_child("auth1")) {
            response_tree.root["nonce"] = std::string("foobar");
            response << response_tree;
            return Result();
        }

        if (request_tree.root.has_child("auth2")) {
            // we know from test_positive_auth that the command works
            // but want to test the negative case so let's return 'unauthorized'
            response_tree.root["unauthorized"];
            response << response_tree;
            return Result(woinc::rpc::CONNECTION_STATUS::OK);
        }

        std::cerr << "Missing auth-tags in request:\n" << request_tree << "\n";
        assert(false);
        return Result(woinc::rpc::CONNECTION_STATUS::ERROR);
    }
};

void doit() {
    ConnectionMock connection;
    AuthorizeCommand cmd;

    cmd.request().password = "some password";

    assert_equals("Command returned wrong status",
                  cmd.execute(connection),
                  woinc::rpc::COMMAND_STATUS::UNAUTHORIZED);

    assert_equals("Flag set", cmd.response().authorized, false);
}

}}

void test_negative_auth() {
    test::negative_auth::doit();
}

/*----------------------------------------------------------------------------*/

namespace test { namespace client_error_auth1 {

struct ConnectionMock : ConnectionMockStub {
    virtual ~ConnectionMock() = default;
    Result mock_do_rpc(Tree &request_tree, std::ostream &response) final override {
        if (request_tree.root.has_child("auth1")) {
            Tree response_tree(create_response_tree());
            response_tree.root["error"] = "Something went wrong.";
            response << response_tree;
            return Result();
        }

        return Result(woinc::rpc::CONNECTION_STATUS::ERROR);
    }
};

void doit() {
    ConnectionMock connection;
    AuthorizeCommand cmd;

    cmd.request().password = "some password";

    assert_equals("Command returned wrong status",
                  cmd.execute(connection),
                  woinc::rpc::COMMAND_STATUS::CLIENT_ERROR);

    assert_equals("Flag set", cmd.response().authorized, false);
}

}}

void test_client_error_auth1() {
    test::client_error_auth1::doit();
}

/*----------------------------------------------------------------------------*/

namespace test { namespace client_error_auth2 {

struct ConnectionMock : ConnectionMockStub {
    virtual ~ConnectionMock() = default;
    Result mock_do_rpc(Tree &request_tree, std::ostream &response) final override {
        Tree response_tree(create_response_tree());

        if (request_tree.root.has_child("auth1")) {
            response_tree.root["nonce"] = std::string("foobar");
            response << response_tree;
            return Result();
        }

        if (request_tree.root.has_child("auth2")) {
            response_tree.root["error"] = "Something went wrong.";
            response << response_tree;
            return Result();
        }

        assert(false);
        return Result(woinc::rpc::CONNECTION_STATUS::ERROR);
    }
};

void doit() {
    ConnectionMock connection;
    AuthorizeCommand cmd;

    cmd.request().password = "some password";

    assert_equals("Command returned wrong status",
                  cmd.execute(connection),
                  woinc::rpc::COMMAND_STATUS::CLIENT_ERROR);

    assert_equals("Flag set", cmd.response().authorized, false);
}

}}

void test_client_error_auth2() {
    test::client_error_auth2::doit();
}
