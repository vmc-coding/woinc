/* libui/src/host_controller.h --
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

#ifndef WOINC_UI_HOST_H_
#define WOINC_UI_HOST_H_

#include <memory>
#include <string>
#include <thread>

#include "client.h"
#include "handler_registry.h"
#include "job_queue.h"
#include "visibility.h"

namespace woinc { namespace ui {

// The host controller is not threadsafe! As this is a lib intern class
// and the only user is the controller, we ensure thread safety there.
class WOINCUI_LOCAL HostController {
    public:
        HostController(std::string name);
        virtual ~HostController();

        HostController(HostController &) = delete;
        HostController &operator=(const HostController &) = delete;

        HostController(HostController &&) = default;
        HostController &operator=(HostController &&) = default;

    public: // called by the controller, error checking and thread safety are done there
        bool connect(const std::string &url, std::uint16_t port);
        void authorize(const std::string &password, const HandlerRegistry &handler_registry);
        void disconnect();

        void shutdown();

    public:
        void schedule_now(std::unique_ptr<Job> job);
        void schedule(std::unique_ptr<Job> job);

    private:
        const std::string host_name_;

        Client client_;
        JobQueue job_queue_;
        std::thread worker_thread_;
};

}}

#endif
