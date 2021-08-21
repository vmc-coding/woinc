/* libui/profiling/run_periodic_tasks.cc --
   Written and Copyright (C) 2021 by vmc.

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

#include <iostream>
#include <string>
#include <thread>

#include <woinc/ui/controller.h>

using namespace woinc::ui;
using namespace std::chrono_literals;

void adjust_interval(Controller &controller, PeriodicTask task) {
    controller.periodic_task_interval(task, controller.periodic_task_interval(task) / 100);
}

int main() {
    std::string host("localhost");
    Controller controller;

    controller.add_host(host, host);

    adjust_interval(controller, PeriodicTask::GetCCStatus);
    adjust_interval(controller, PeriodicTask::GetClientState);
    adjust_interval(controller, PeriodicTask::GetDiskUsage);
    adjust_interval(controller, PeriodicTask::GetFileTransfers);
    adjust_interval(controller, PeriodicTask::GetMessages);
    adjust_interval(controller, PeriodicTask::GetNotices);
    adjust_interval(controller, PeriodicTask::GetProjectStatus);
    adjust_interval(controller, PeriodicTask::GetStatistics);
    adjust_interval(controller, PeriodicTask::GetTasks);

    controller.schedule_periodic_tasks(host, true);

    std::this_thread::sleep_for(10s);

    return 0;
}
