/* tests/commands/get_cc_status_cmd_test.cc --
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
    tests["01 - Positive test"]          = test::commands::test_positive;
    tests["02 - Not a BOINC response"]   = test::commands::test_not_boinc_response;
    tests["03 - Wrong cmd response"]     = test::commands::test_wrong_cmd_response;
}

namespace wrpc = woinc::rpc;

// ----------------------------------------------------------------

namespace test {

std::string get_request_tag() {
    return "get_cc_status";
}

std::string get_response_tag() {
    return "cc_status";
}

wrpc::Command *create_valid_command() {
    return new wrpc::GetCCStatusCommand;
}

void validate_request_cmd_node(woinc::xml::Node &cmd_node) {
    assert_true("Found too much nodes in request", cmd_node.children.empty());
}

void create_positive_response(std::ostream &response) {
    response << R"(
        <boinc_gui_rpc_reply>
            <cc_status>
                <network_status>1</network_status>
                <ams_password_error>0</ams_password_error>
                <manager_must_quit>0</manager_must_quit>

                <task_suspend_reason>0</task_suspend_reason>
                <task_mode>1</task_mode>
                <task_mode_perm>1</task_mode_perm>
                <task_mode_delay>13.000000</task_mode_delay>

                <gpu_suspend_reason>1</gpu_suspend_reason>
                <gpu_mode>2</gpu_mode>
                <gpu_mode_perm>2</gpu_mode_perm>
                <gpu_mode_delay>1.000000</gpu_mode_delay>

                <network_suspend_reason>2</network_suspend_reason>
                <network_mode>3</network_mode>
                <network_mode_perm>3</network_mode_perm>
                <network_mode_delay>0.100000</network_mode_delay>

                <disallow_attach>1</disallow_attach>
                <simple_gui_only>1</simple_gui_only>
                <max_event_log_lines>2000</max_event_log_lines>
            </cc_status>
        </boinc_gui_rpc_reply>)";
}

// TODO check all attributes
void validate_positive_response(wrpc::Command *cmd_in) {
    const auto cmd = dynamic_cast<wrpc::GetCCStatusCommand *>(cmd_in);
    const auto &cc_status = cmd->response().cc_status;

    assert_equals("Wrong network status", cc_status.network_status, woinc::NETWORK_STATUS::WANT_CONNECTION);

    assert_equals("Wrong CPU suspend reason", cc_status.cpu.suspend_reason, woinc::SUSPEND_REASON::NOT_SUSPENDED);
    assert_equals("Wrong CPU mode", cc_status.cpu.mode, woinc::RUN_MODE::ALWAYS);
    assert_equals("Wrong CPU perm mode", cc_status.cpu.perm_mode, woinc::RUN_MODE::ALWAYS);
    assert_equals("Wrong CPU delay", cc_status.cpu.delay, 13.0);

    assert_equals("Wrong GPU suspend reason", cc_status.gpu.suspend_reason, woinc::SUSPEND_REASON::BATTERIES);
    assert_equals("Wrong GPU mode", cc_status.gpu.mode, woinc::RUN_MODE::AUTO);
    assert_equals("Wrong GPU perm mode", cc_status.gpu.perm_mode, woinc::RUN_MODE::AUTO);
    assert_equals("Wrong GPU delay", cc_status.gpu.delay, 1.0);

    assert_equals("Wrong network suspend reason", cc_status.network.suspend_reason, woinc::SUSPEND_REASON::USER_ACTIVE);
    assert_equals("Wrong network mode", cc_status.network.mode, woinc::RUN_MODE::NEVER);
    assert_equals("Wrong network perm mode", cc_status.network.perm_mode, woinc::RUN_MODE::NEVER);
    assert_equals("Wrong network delay", cc_status.network.delay, 0.1);
}

}
