/* libui/src/job_queue.h --
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

#ifndef WOINC_UI_JOB_QUEUE_H_
#define WOINC_UI_JOB_QUEUE_H_

#include <condition_variable>
#include <deque>
#include <mutex>

#include "jobs.h"
#include "visibility.h"

namespace woinc { namespace ui {

class WOINCUI_LOCAL JobQueue {
    public:
        JobQueue() = default;
        ~JobQueue();

        JobQueue(const JobQueue &) = delete;
        JobQueue(JobQueue &&) = delete;
        JobQueue &operator=(const JobQueue &) = delete;
        JobQueue &operator=(JobQueue &&) = delete;

        // The job queue takes ownership of the job
        void push_front(Job *job);
        void push_back(Job *job);

        // Returns the next job to run while blocking if there isn't any job in the queue.
        // If shutdown is triggered, a nullptr will be returned.
        // The caller takes ownership of the job
        Job *pop();

        void shutdown();

    private:
        void push_(Job *job, bool front);

    private:
        bool shutdown_ = false;

        std::mutex lock_;
        std::condition_variable condition_;

        typedef std::deque<Job *> Queue;
        Queue jobs_;
};

}}

#endif
