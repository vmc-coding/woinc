/* tests/commands/get_results_cmd_test.cc --
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
    return "get_results";
}

std::string get_response_tag() {
    return "results";
}

wrpc::Command *create_valid_command() {
    return new wrpc::GetResultsCommand;
}

void validate_request_cmd_node(woinc::xml::Node &cmd_node) {
    assert_equals("Expected node 'active_only'", cmd_node.children.size(), 1);

    auto &node = need_node(cmd_node, "active_only");
    assert_true("active_only does have wrong content: " + node.content,
                node.content == "0" || node.content == "1");
}

void create_positive_response(std::ostream &response) {
    response << R"(
        <boinc_gui_rpc_reply>
            <results>
                <result>
                    <name>name_1</name>
                    <wu_name>wu_name_1</wu_name>
                    <platform>some_platform</platform>
                    <plan_class>nci</plan_class>
                    <project_url>some_url</project_url>
                    <final_cpu_time>4.000000</final_cpu_time>
                    <final_elapsed_time>5.000000</final_elapsed_time>
                    <exit_status>196</exit_status>
                    <signal>2</signal>
                    <state>2</state>
                    <report_deadline>1234567891.000000</report_deadline>
                    <received_time>1234567890.000000</received_time>
                    <estimated_cpu_time_remaining>12345.678901</estimated_cpu_time_remaining>
                    <ready_to_report/>
                    <suspended_via_gui/>
                    <resources>32 CPUs</resources>
                    <got_server_ack/>
                    <completed_time>1.2</completed_time>
                    <project_suspended_via_gui/>
                    <report_immediately/>
                    <edf_scheduled/>
                    <coproc_missing/>
                    <scheduler_wait/>
                    <scheduler_wait_reason>reason</scheduler_wait_reason>
                    <network_wait/>
                    <version_num>123</version_num>
                    <active_task>
                        <active_task_state>1</active_task_state>
                        <app_version_num>234</app_version_num>
                        <scheduler_state>2</scheduler_state>
                        <checkpoint_cpu_time>1.000000</checkpoint_cpu_time>
                        <fraction_done>0.500000</fraction_done>
                        <current_cpu_time>2.000000</current_cpu_time>
                        <elapsed_time>3.000000</elapsed_time>
                        <swap_size>123456.000000</swap_size>
                        <working_set_size_smoothed>1234567.000000</working_set_size_smoothed>
                        <bytes_sent>1.200000</bytes_sent>
                        <bytes_received>2.300000</bytes_received>
                        <working_set_size>1234567.000000</working_set_size>
                        <slot>0</slot>
                        <pid>12345</pid>
                        <page_fault_rate>0.000000</page_fault_rate>
                        <progress_rate>0.000000</progress_rate>
                    </active_task>
                </result>
                <result>
                    <name>name_2</name>
                </result>
            </results>
        </boinc_gui_rpc_reply>)";
}

void validate_positive_response(wrpc::Command *cmd_in) {
    auto *cmd = dynamic_cast<wrpc::GetResultsCommand *>(cmd_in);

    const auto &tasks = cmd->response().tasks;
    assert_equals("Expected two tasks", tasks.size(), 2);

    auto task_iter = tasks.begin();

    {
        const auto& task = *task_iter++;

        assert_equals("Task 1 - wrong name", task.name, std::string("name_1"));
        assert_equals("Task 1 - wrong wu_name", task.wu_name, std::string("wu_name_1"));
        assert_equals("Task 1 - wrong project_url", task.project_url, std::string("some_url"));
        assert_equals("Task 1 - wrong final_cpu_time", task.final_cpu_time, 4.0);
        assert_equals("Task 1 - wrong exit_status", task.exit_status, woinc::TASK_EXIT_CODE::DISK_LIMIT_EXCEEDED);
        assert_equals("Task 1 - wrong state", task.state, woinc::RESULT_CLIENT_STATE::FILES_DOWNLOADED);
        assert_equals("Task 1 - wrong resport_deadline", task.report_deadline, static_cast<time_t>(1234567891.0));
        assert_equals("Task 1 - wrong estimated_cpu_time_remaining", task.estimated_cpu_time_remaining, 12345.678901);

        assert_true("Task 1 - wrong ready_to_report", task.ready_to_report);
        assert_true("Task 1 - wrong suspended_via_gui", task.suspended_via_gui);
        assert_equals("Task 1 - wrong resources", task.resources, std::string("32 CPUs"));

#ifdef WOINC_EXPOSE_FULL_STRUCTURES
        assert_true("Task 1 - wrong got_server_ack", task.got_server_ack);
        assert_equals("Task 1 - wrong completed_time", task.completed_time, 1.2);
        assert_true("Task 1 - wrong project_suspended_via_gui", task.project_suspended_via_gui);
        assert_true("Task 1 - wrong report_immediately", task.report_immediately);
        assert_true("Task 1 - wrong edf_scheduled", task.edf_scheduled);
        assert_true("Task 1 - wrong coproc_missing", task.coproc_missing);
        assert_true("Task 1 - wrong scheduler_wait", task.scheduler_wait);
        assert_equals("Task 1 - wrong scheduler_wait_reason", task.scheduler_wait_reason, std::string("reason"));
        assert_true("Task 1 - wrong network_wait", task.network_wait);
        assert_equals("Task 1 - wrong version num", task.version_num, 123);
#endif

        assert_true("Task 1 - not active", task.active_task.get() != nullptr);

        auto &active_task(*task.active_task);

        assert_equals("Task 1 - wrong active_task_state", active_task.active_task_state, woinc::ACTIVE_TASK_STATE::EXECUTING);
        assert_equals("Task 1 - wrong app_version_num", active_task.app_version_num, 234);
        assert_equals("Task 1 - wrong scheduler_state", active_task.scheduler_state, woinc::SCHEDULER_STATE::SCHEDULED);
        assert_equals("Task 1 - wrong checkpoint_cpu_time", active_task.checkpoint_cpu_time, 1.0);
        assert_equals("Task 1 - wrong fraction_done", active_task.fraction_done, 0.5);
        assert_equals("Task 1 - wrong current_cpu_time", active_task.current_cpu_time, 2.0);
        assert_equals("Task 1 - wrong swap_size", active_task.swap_size, 123456.0);
        assert_equals("Task 1 - wrong bytes_sent", active_task.bytes_sent, 1.2);
        assert_equals("Task 1 - wrong bytes_received", active_task.bytes_received, 2.3);
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
        assert_equals("Task 1 - wrong working_set_size", active_task.working_set_size, 1234567.0);
#endif
    }

    {
        const auto& task = *task_iter++;

        assert_equals("Task 2 - wrong name", task.name, std::string("name_2"));

        assert_false("Task 2 - wrong ready_to_report", task.ready_to_report);
        assert_false("Task 2 - wrong suspended_via_gui", task.suspended_via_gui);
#ifdef WOINC_EXPOSE_FULL_STRUCTURES
        assert_false("Task 2 - wrong got_server_ack", task.got_server_ack);
        assert_false("Task 2 - wrong project_suspended_via_gui", task.project_suspended_via_gui);
        assert_false("Task 2 - wrong report_immediately", task.report_immediately);
        assert_false("Task 2 - wrong edf_scheduled", task.edf_scheduled);
        assert_false("Task 2 - wrong coproc_missing", task.coproc_missing);
        assert_false("Task 2 - wrong scheduler_wait", task.scheduler_wait);
        assert_false("Task 2 - wrong network_wait", task.network_wait);
#endif

        assert_true("Task 2 - active", task.active_task.get() == nullptr);

    }
}

}
