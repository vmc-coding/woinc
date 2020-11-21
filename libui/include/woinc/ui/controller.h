/* woinc/ui/controller.h --
   Written and Copyright (C) 2017-2020 by vmc.

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

#ifndef WOINC_UI_CONTROLLER_H_
#define WOINC_UI_CONTROLLER_H_

#include <cstdint>
#include <future>
#include <memory>
#include <string>

#include <woinc/ui/defs.h>
#include <woinc/ui/error.h>
#include <woinc/ui/handler.h>

namespace woinc { namespace ui {

class Controller {
    public:
        Controller();
        virtual ~Controller();

        Controller(const Controller &) = delete;
        Controller &operator=(const Controller &) = delete;

        Controller(Controller &&) = default;
        Controller &operator=(Controller &&) = default;

        virtual void shutdown();

    public: // handler

        virtual void register_handler(HostHandler *handler);
        virtual void deregister_handler(HostHandler *handler);

        virtual void register_handler(PeriodicTaskHandler *handler);
        virtual void deregister_handler(PeriodicTaskHandler *handler);

    public: // basic host handling

        // TODO rename to (dis)connect_host? is host the correct name or would we connect to clients instead?
        //      there may be more than one client at a given host ..
        // TODO rename to async_add_host?
        virtual void add_host(const std::string &host,
                              const std::string &url,
                              std::uint16_t port);

        virtual void authorize_host(const std::string &host,
                                    const std::string &password);

        // TODO only provide the async variant for public
        virtual void remove_host(const std::string &host);
        // use the async variant if you want to remove a host in one of the handlers
        virtual void async_remove_host(std::string host);

    public: // periodic tasks handling

        virtual void periodic_task_interval(PeriodicTask task, int seconds);
        virtual int periodic_task_interval(PeriodicTask task) const;

        virtual void schedule_periodic_tasks(const std::string &host, bool value);

        virtual void reschedule_now(const std::string &host, PeriodicTask task);

        virtual void active_only_tasks(const std::string &host, bool value);

    public: // commands to the client; all of those commands are async

        virtual std::future<bool> file_transfer_op(const std::string &host, FILE_TRANSFER_OP op,
                                                   const std::string &master_url, const std::string &filename);
        virtual std::future<bool> project_op(const std::string &host, PROJECT_OP op, const std::string &master_url);
        virtual std::future<bool> task_op(const std::string &host, TASK_OP op, const std::string &master_url, const std::string &task_name);

        virtual std::future<GlobalPreferences> load_global_preferences(const std::string &host,
                                                                       GET_GLOBAL_PREFS_MODE mode);
        virtual std::future<bool> save_global_preferences(const std::string &host,
                                                          const GlobalPreferences &prefs,
                                                          const GlobalPreferencesMask &mask);

        virtual std::future<bool> read_global_prefs_override(const std::string &host);

        virtual std::future<bool> run_mode(const std::string &host, RUN_MODE mode);
        virtual std::future<bool> gpu_mode(const std::string &host, RUN_MODE mode);
        virtual std::future<bool> network_mode(const std::string &host, RUN_MODE mode);

        virtual std::future<AllProjectsList> all_projects_list(const std::string &host);

        virtual std::future<bool> start_loading_project_config(const std::string &host, std::string master_url);
        // if it's still loading the resulting config.error_num will be -204, poll again after some delay;
        // yep, that's not a nice interface, we'll use std::variant when switching to std-c++17
        virtual std::future<ProjectConfig> poll_project_config(const std::string &host);

        virtual std::future<bool> start_account_lookup(const std::string &host, std::string master_url,
                                                       std::string email, std::string password);
        // if it's still loading the resulting config.error_num will be -204, poll again after some delay;
        virtual std::future<AccountOut> poll_account_lookup(const std::string &host);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
};

}}

#endif
