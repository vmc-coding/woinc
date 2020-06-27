/* libui/src/host_controller.cc --
   Written and Copyright (C) 2018-2019 by vmc.

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

#include "host_controller.h"

#include <cassert>
#include <thread>

namespace {

using namespace woinc::ui;

struct Worker {
    Worker(Client &client, JobQueue &job_queue)
        : client_(client),
        job_queue_(job_queue)
    {}

    void operator()() {
        Job *job;
        do {
            if ((job = job_queue_.pop()) != nullptr) {
                (*job)(client_);
                delete job;
            }
        } while (job != nullptr);
    }

    private:
        Client &client_;
        JobQueue &job_queue_;
};

}

namespace woinc { namespace ui {

HostController::HostController(const std::string &name) : host_name_(name) {}

HostController::~HostController() {
    shutdown();
}

bool HostController::connect(const std::string &url, std::uint16_t port) {
    if (!client_.connect(url, port))
        return false;

    worker_thread_ = std::thread(Worker(client_, job_queue_));
    return true;
}

void HostController::authorize(const std::string &password, const HandlerRegistry &handler_registry) {
    schedule(new AuthorizationJob(password, handler_registry));
}

void HostController::disconnect() {
    client_.disconnect();
}

void HostController::shutdown() {
    job_queue_.shutdown();
    if (worker_thread_.joinable())
        worker_thread_.join();
    disconnect();
}

void HostController::schedule_now(Job *job) {
    job_queue_.push_front(job);
}

void HostController::schedule(Job *job) {
    job_queue_.push_back(job);
}

}}
