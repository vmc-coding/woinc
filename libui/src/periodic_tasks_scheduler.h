/* libui/src/periodic_tasks_scheduler.h --
   Written and Copyright (C) 2018-2022 by vmc.

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

#ifndef WOINC_UI_PERIODIC_TASKS_SCHEDULER_H_
#define WOINC_UI_PERIODIC_TASKS_SCHEDULER_H_

#include <array>
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <string>

#include "configuration.h"
#include "handler_registry.h"
#include "host_controller.h"
#include "jobs.h"
#include "visibility.h"

namespace woinc { namespace ui {

class WOINCUI_LOCAL PeriodicTasksSchedulerContext {
    public:
        PeriodicTasksSchedulerContext(const Configuration &config, const HandlerRegistry &hander_registry);

        void add_host(std::string host, HostController &controller);
        void remove_host(const std::string &host);

        void reschedule_now(const std::string &host, PeriodicTask task);

        void trigger_shutdown();

    private:
        friend class PeriodicTasksScheduler;

        const HandlerRegistry &handler_registry_;

        const Configuration &configuration_;

        std::mutex mutex_;
        std::condition_variable condition_;

        bool shutdown_triggered_ = false;

        struct Task {
            Task(PeriodicTask t) : type(t) {}
            const PeriodicTask type;
            bool pending = false;
            std::chrono::steady_clock::time_point last_execution = std::chrono::steady_clock::time_point::min();
        };

        struct State {
            int messages_seqno = 0;
            int notices_seqno  = 0;
        };

        std::map<std::string, std::array<Task, 9>> tasks_;
        std::map<std::string, HostController &> host_controllers_;
        std::map<std::string, State> states_;
};

class WOINCUI_LOCAL PeriodicTasksScheduler : public PostExecutionHandler {
    public:
        PeriodicTasksScheduler(PeriodicTasksSchedulerContext &context);

        void operator()();

    public:
        void handle_post_execution(const std::string &host, Job *job) final;

    private:
        bool should_be_scheduled_(const PeriodicTasksSchedulerContext::Task &task,
                                  const Configuration::Intervals &intervals,
                                  const decltype(PeriodicTasksSchedulerContext::Task::last_execution) &now) const;
        void schedule_(const std::string &host, PeriodicTasksSchedulerContext::Task &task);

        PeriodicTasksSchedulerContext &context_;
};

}}

#endif
