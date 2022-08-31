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
#include <memory>
#include <stdexcept>

#ifndef NDEBUG
#include <iostream>
#endif

namespace woinc { namespace ui {

JobQueue::~JobQueue() {
    shutdown();
    // ensure no one is in a critical section
    std::unique_lock<std::mutex> lock(mutex_);
}

void JobQueue::push_front(std::unique_ptr<Job> job) {
    push_(std::move(job), true);
}

void JobQueue::push_back(std::unique_ptr<Job> job) {
    push_(std::move(job), false);
}

std::unique_ptr<Job> JobQueue::pop() {
    std::unique_ptr<Job> job;
    std::unique_lock<std::mutex> lock(mutex_);

    condition_.wait(lock, [this]() { return !jobs_.empty() || shutdown_; });

    if (!shutdown_) {
        assert(!jobs_.empty() && "Found empty job queue");

        job = std::move(jobs_.front());
        jobs_.pop_front();

        assert(job && "Received empty job from the queue");
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

void JobQueue::push_(std::unique_ptr<Job> job, bool front) {
    assert(job && "Can't insert empty job");

    bool pushed = true;

    {
        std::lock_guard<decltype(mutex_)> guard(mutex_);

        if (!shutdown_) {
            if (front)
                jobs_.push_front(std::move(job));
            else
                jobs_.push_back(std::move(job));
        } else {
            pushed = false;
        }
    }

    if (pushed)
        condition_.notify_one();
}

}}
