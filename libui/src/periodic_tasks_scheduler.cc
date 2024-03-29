/* libui/src/periodic_tasks_scheduler.cc --
   Written and Copyright (C) 2018-2023 by vmc.

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

#include "periodic_tasks_scheduler.h"

#include <algorithm>
#include <cassert>
#include <mutex>
#include <thread>

#ifndef NDEBUG
#include <iostream>
#endif

using namespace std::chrono_literals;

namespace woinc { namespace ui {

// --- PeriodicTasksSchedulerContext ---

PeriodicTasksSchedulerContext::PeriodicTasksSchedulerContext(const Configuration &config,
                                                             const HandlerRegistry &handler_registry,
                                                             Scheduler scheduler)
    : configuration_(config), handler_registry_(handler_registry), scheduler_(std::move(scheduler)) {}

void PeriodicTasksSchedulerContext::add_host(std::string host) {
    auto tasks = std::array<Task, 9> {
        Task(PeriodicTask::GetCCStatus),
        Task(PeriodicTask::GetClientState),
        Task(PeriodicTask::GetDiskUsage),
        Task(PeriodicTask::GetFileTransfers),
        Task(PeriodicTask::GetMessages),
        Task(PeriodicTask::GetNotices),
        Task(PeriodicTask::GetProjectStatus),
        Task(PeriodicTask::GetStatistics),
        Task(PeriodicTask::GetTasks)
    };

    std::lock_guard<decltype(mutex_)> guard(mutex_);
    tasks_.emplace(host, std::move(tasks));
    states_.emplace(std::move(host), State());
}

void PeriodicTasksSchedulerContext::remove_host(const std::string &host) {
    std::lock_guard<decltype(mutex_)> guard(mutex_);
    tasks_.erase(host);
    states_.erase(host);
}

void PeriodicTasksSchedulerContext::reschedule_now(const std::string &host, PeriodicTask to_reschedule) {
    {
        std::lock_guard<decltype(mutex_)> guard(mutex_);
        tasks_.at(host).at(static_cast<size_t>(to_reschedule)).last_execution = std::chrono::steady_clock::time_point::min();
    }
    condition_.notify_one();
}

void PeriodicTasksSchedulerContext::trigger_shutdown() {
    {
        std::lock_guard<decltype(mutex_)> guard(mutex_);
        shutdown_triggered_ = true;
    }
    condition_.notify_all();
}

void PeriodicTasksSchedulerContext::handle_post_execution(const std::string &host, Job *j) {
    std::lock_guard<decltype(mutex_)> guard(mutex_);

    if (shutdown_triggered_)
        return;

    // we schedule and therefore register to periodic tasks only
    assert(dynamic_cast<PeriodicJob *>(j) != nullptr);

    PeriodicJob *job = static_cast<PeriodicJob *>(j);

    auto &tasks = tasks_.at(host);
    auto task = std::find_if(tasks.begin(), tasks.end(), [&](const auto &t) {
        return t.type == job->task;
    });

    if (task != tasks.end()) {
        task->last_execution = std::chrono::steady_clock::now();
        task->pending = false;

        if (job->task == PeriodicTask::GetMessages)
            states_.at(host).messages_seqno = job->payload.seqno;
        else if (job->task == PeriodicTask::GetNotices)
            states_.at(host).notices_seqno = job->payload.seqno;
    }
}


// --- PeriodicTasksScheduler ---

PeriodicTasksScheduler::PeriodicTasksScheduler(PeriodicTasksSchedulerContext &context)
    : context_(context)
{}

void PeriodicTasksScheduler::operator()() {
    const auto max_wake_up_time = 200ms;

    auto intervals{context_.configuration_.intervals()};
    auto wake_up_interval = std::min(*std::min_element(intervals.begin(), intervals.end()), max_wake_up_time);
    auto last_cache_update = std::chrono::steady_clock::now();

    while (true) {
        const auto now = std::chrono::steady_clock::now();

        // update interval cache once a second
        if (now - last_cache_update > 1s) {
            intervals = std::move(context_.configuration_.intervals());
            wake_up_interval = std::min(*std::min_element(intervals.begin(), intervals.end()), max_wake_up_time);
            last_cache_update = now;
        }

        std::unique_lock<decltype(context_.mutex_)> guard(context_.mutex_);

        if (!context_.shutdown_triggered_) {
            for (auto &host_tasks : context_.tasks_) {
                if (!context_.configuration_.schedule_periodic_tasks(host_tasks.first))
                    continue;
                for (auto &task : host_tasks.second)
                    if (!task.pending && now >= task.last_execution + intervals.at(static_cast<size_t>(task.type)))
                        schedule_(host_tasks.first, task);
            }
        }

        if (context_.condition_.wait_for(guard, wake_up_interval, [this]() { return context_.shutdown_triggered_; }))
            break;
    }
}

void PeriodicTasksScheduler::schedule_(const std::string &host, PeriodicTasksSchedulerContext::Task &task) {
    task.pending = true;

    PeriodicJob::Payload payload;

    if (task.type == PeriodicTask::GetMessages)
        payload.seqno = context_.states_.at(host).messages_seqno;
    else if (task.type == PeriodicTask::GetNotices)
        payload.seqno = context_.states_.at(host).notices_seqno;
    else if (task.type == PeriodicTask::GetTasks)
        payload.active_only = context_.configuration_.active_only_tasks(host);

    auto job = std::make_unique<PeriodicJob>(task.type, context_.handler_registry_, payload);
    job->register_post_execution_handler(&context_);

    context_.scheduler_(host, std::move(job));
}

}}
