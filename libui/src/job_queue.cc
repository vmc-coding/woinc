/* libui/src/job_queue.cc --
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

#include "job_queue.h"

#include <cassert>
#include <stdexcept>

namespace woinc { namespace ui {

JobQueue::~JobQueue() {
    shutdown();

    std::lock_guard<decltype(mutex_)> guard(mutex_);
    while (!jobs_.empty()) {
        delete jobs_.front();
        jobs_.pop_front();
    }
}

void JobQueue::push_front(Job *job) {
    push_(job, true);
}

void JobQueue::push_back(Job *job) {
    push_(job, false);
}

Job *JobQueue::pop() {
    Job *job = nullptr;
    std::unique_lock<std::mutex> lock(mutex_);

    condition_.wait(lock, [this]() { return !jobs_.empty() || shutdown_; });

    if (!shutdown_) {
        assert(!jobs_.empty());

        job = jobs_.front();
        jobs_.pop_front();

        assert(job);
    }

    return job;
}

void JobQueue::shutdown() {
    {
        std::lock_guard<decltype(mutex_)> guard(mutex_);
        shutdown_ = true;
    }

    condition_.notify_all();
}

void JobQueue::push_(Job *job, bool front) {
    if (job == nullptr)
        throw std::invalid_argument("Received nullptr instead of a job");

    bool pushed = true;

    {
        std::lock_guard<decltype(mutex_)> guard(mutex_);

        if (!shutdown_) {
            if (front)
                jobs_.push_front(job);
            else
                jobs_.push_back(job);
        } else {
            pushed = false;
        }
    }

    if (pushed)
        condition_.notify_one();
    else // the queue takes ownership but as the shutdown is triggered, we simply delete the job
        delete job;
}

}}
