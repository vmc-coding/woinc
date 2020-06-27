/* tests/commands/exchange_versions_cmd_test.cc --
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

#include "../test.h"

#include "generic_command_tests.h"

void get_tests(Tests &tests) {
    tests["01 - Positive test"]        = test::commands::test_positive;
    tests["02 - Not a BOINC response"] = test::commands::test_not_boinc_response;
    tests["03 - Wrong cmd response"]   = test::commands::test_wrong_cmd_response;
}

namespace wrpc = woinc::rpc;

// ----------------------------------------------------------------

namespace test {

std::string get_request_tag() {
    return "exchange_versions";
}

std::string get_response_tag() {
    return "server_version";
}

wrpc::Command *create_valid_command() {
    auto cmd = new wrpc::ExchangeVersionsCommand;
    cmd->request().version.major = 1;
    cmd->request().version.minor = 2;
    cmd->request().version.release = 3;
    return cmd;
}

void validate_request_cmd_node(woinc::xml::Node &cmd_node) {
    assert_equals("Found wrong number of nodes in request", cmd_node.children.size(), 3);

    need_node_with_value(cmd_node, "major", 1);
    need_node_with_value(cmd_node, "minor", 2);
    need_node_with_value(cmd_node, "release", 3);
}

void create_positive_response(std::ostream &response) {
    response << R"(
        <boinc_gui_rpc_reply>
          <server_version>
            <major>4</major>
            <minor>5</minor>
            <release>6</release>
          </server_version>
        </boinc_gui_rpc_reply>)";
}

void validate_positive_response(wrpc::Command *cmd_in) {
    const auto cmd = dynamic_cast<wrpc::ExchangeVersionsCommand *>(cmd_in);
    const auto &version = cmd->response().version;

    assert_equals("Major version missmatch", version.major, 4);
    assert_equals("Minor version missmatch", version.minor, 5);
    assert_equals("Release version missmatch", version.release, 6);
}

}
