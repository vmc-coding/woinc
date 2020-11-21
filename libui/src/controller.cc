/* ui/controller/controller.cc --
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

#include <woinc/ui/controller.h>

#include <cassert>
#include <exception>
#include <map>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>

#ifndef NDEBUG
#include <iostream>
#endif

#include "configuration.h"
#include "handler_registry.h"
#include "host_controller.h"
#include "periodic_tasks_scheduler.h"

#define WOINC_LOCK_GUARD std::lock_guard<decltype(lock_)> guard(lock_)

namespace wrpc = woinc::rpc;

namespace {

void check_not_empty__(const std::string &str, const std::string &msg) {
    if (str.empty())
        throw std::invalid_argument(msg);
}

void check_not_empty_host_name__(const std::string &host) {
    check_not_empty__(host, "Missing host name");
}

}

namespace woinc { namespace ui {

// ---- Controller::Impl ----

class WOINCUI_LOCAL Controller::Impl {
    public:
        Impl();
        ~Impl();

        Impl(const Impl &) = delete;
        Impl &operator=(const Impl &) = delete;

        Impl(Impl &&) = default;
        Impl &operator=(Impl &&) = default;

        void shutdown();

        void register_handler(HostHandler *handler);
        void deregister_handler(HostHandler *handler);

        void register_handler(PeriodicTaskHandler *handler);
        void deregister_handler(PeriodicTaskHandler *handler);

        void add_host(std::string host,
                      std::string url,
                      std::uint16_t port);
        void authorize_host(std::string host,
                            std::string password);

        void remove_host(const std::string &host);
        void async_remove_host(std::string host);

        void periodic_task_interval(const PeriodicTask task, int interval);
        int periodic_task_interval(const PeriodicTask task) const;
        void schedule_periodic_tasks(const std::string &host, bool value);
        void reschedule_now(const std::string &host, PeriodicTask task);

        void active_only_tasks(const std::string &host, bool value);

        std::future<bool> file_transfer_op(const std::string &host, FILE_TRANSFER_OP op,
                                           const std::string &master_url, const std::string &filename);
        std::future<bool> project_op(const std::string &host, PROJECT_OP op, const std::string &master_url);
        std::future<bool> task_op(const std::string &host, TASK_OP op, const std::string &master_url, const std::string &task_name);

        std::future<GlobalPreferences> load_global_preferences(const std::string &host, GET_GLOBAL_PREFS_MODE mode);
        std::future<bool> save_global_preferences(const std::string &host, const GlobalPreferences &prefs, const GlobalPreferencesMask &mask);
        std::future<bool> read_global_prefs_override(const std::string &host);

        std::future<bool> run_mode(const std::string &host, RUN_MODE mode);
        std::future<bool> gpu_mode(const std::string &host, RUN_MODE mode);
        std::future<bool> network_mode(const std::string &host, RUN_MODE mode);

        std::future<AllProjectsList> all_projects_list(const std::string &host);

        std::future<bool> start_loading_project_config(const std::string &host, std::string master_url);
        std::future<ProjectConfig> poll_project_config(const std::string &host);

        std::future<bool> start_account_lookup(const std::string &host, std::string master_url,
                                               std::string email, std::string password);
        std::future<AccountOut> poll_account_lookup(const std::string &host);

    private: // helper methods which assume the controller is already locked
        // use a copy of the host string as it may be the key of the host controller map
        // which will be deleted in the erase call leading to a use after free access later on
        void remove_host_(std::string host);
        void async_remove_host_(std::string host);

        bool has_host_(const std::string &name) const;

        void schedule_now_(const std::string &host, Job *job, const char *func);

        void verify_not_shutdown_() const;
        void verify_known_host_(const std::string &host, const char *func) const;

        template<typename CMD,
                 typename RESULT,
                 typename GETTER = RESULT (*)(decltype(std::declval<CMD>().response()) &),
                 typename REQUEST = decltype(std::declval<CMD>().request())>
        std::future<RESULT> create_and_schedule_async_job_(const char *func,
                                                           const std::string &host,
                                                           GETTER getter,
                                                           std::string error_msg,
                                                           std::remove_reference_t<REQUEST> request = {}) {
            typedef std::promise<RESULT> Promise;
            Promise promise;
            auto future = promise.get_future();

            auto *job = new woinc::ui::AsyncJob<RESULT>(
                new CMD{request},
                std::move(promise),
                [=](woinc::rpc::Command *cmd, Promise &p, woinc::rpc::COMMAND_STATUS status) {
                    if (status == woinc::rpc::COMMAND_STATUS::OK)
                        p.set_value(getter(static_cast<CMD *>(cmd)->response()));
                    else
                        p.set_exception(std::make_exception_ptr(std::runtime_error{error_msg}));
                });

            schedule_now_(host, job, func);
            return future;
        }


    private:
        std::mutex lock_;

        bool shutdown_ = false;

        HandlerRegistry handler_registry_;

        Configuration configuration_;

        PeriodicTasksSchedulerContext periodic_tasks_scheduler_context_;
        std::thread periodic_tasks_scheduler_thread_;

        typedef std::map<std::string, std::unique_ptr<HostController>> HostControllers;
        HostControllers host_controllers_;
};

Controller::Impl::Impl() :
    periodic_tasks_scheduler_context_(configuration_, handler_registry_),
    periodic_tasks_scheduler_thread_(PeriodicTasksScheduler(periodic_tasks_scheduler_context_))
{}

Controller::Impl::~Impl() {
    shutdown();
}

void Controller::Impl::shutdown() {
    WOINC_LOCK_GUARD;

    // shutdown the controller

    shutdown_ = true;

    // shutdown the periodic tasks scheduler

    periodic_tasks_scheduler_context_.trigger_shutdown();

    if (periodic_tasks_scheduler_thread_.joinable())
        periodic_tasks_scheduler_thread_.join();

    // shutdown the host controllers

    while (!host_controllers_.empty())
        remove_host_(host_controllers_.cbegin()->first);
}

void Controller::Impl::register_handler(HostHandler *handler) {
    handler_registry_.register_handler(handler);
}

void Controller::Impl::deregister_handler(HostHandler *handler) {
    handler_registry_.deregister_handler(handler);
}

void Controller::Impl::register_handler(PeriodicTaskHandler *handler) {
    handler_registry_.register_handler(handler);
}

void Controller::Impl::deregister_handler(PeriodicTaskHandler *handler) {
    handler_registry_.deregister_handler(handler);
}

void Controller::Impl::add_host(std::string host,
                                std::string url,
                                std::uint16_t port) {
    check_not_empty_host_name__(host);
    check_not_empty__(url, "Missing url to host");

    HostController *host_controller = nullptr;

    {
        WOINC_LOCK_GUARD;

        verify_not_shutdown_();

        if (has_host_(host))
            throw std::invalid_argument("Host \"" + host + "\" already registered.");

        host_controller = new HostController(host);

        configuration_.add_host(host);
        host_controllers_.emplace(host, std::move(host_controller));
        // periodic tasks are not scheduled yet
        periodic_tasks_scheduler_context_.add_host(host, *host_controllers_.at(host));

        handler_registry_.for_host_handler([&](auto &handler) {
            handler.on_host_added(host);
        });
    }

    // connect asynchronously because the connect may block for a long time (see man 2 connect)
    std::thread([=]() {
        bool connected = host_controller->connect(url, port);
        handler_registry_.for_host_handler([&](HostHandler &handler) {
            if (connected)
                handler.on_host_connected(host);
            else
                handler.on_host_error(host, Error::CONNECTION_ERROR);
        });
    }).detach();
}

void Controller::Impl::authorize_host(std::string host,
                                      std::string password) {
    check_not_empty_host_name__(host);
    check_not_empty__(password, "Missing password");

    WOINC_LOCK_GUARD;

    verify_not_shutdown_();
    verify_known_host_(host, __func__);

    auto hc = host_controllers_.find(host);
    assert(hc != host_controllers_.end());
    hc->second->authorize(password, handler_registry_);
}

void Controller::Impl::remove_host(const std::string &host) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;
    verify_not_shutdown_();
    verify_known_host_(host, __func__);
    remove_host_(host);
}

void Controller::Impl::async_remove_host(std::string host) {
    check_not_empty_host_name__(host);

    std::thread([=]() { async_remove_host_(host); }).detach();
}

void Controller::Impl::periodic_task_interval(const PeriodicTask task, int interval) {
    configuration_.interval(task, interval);
}

int Controller::Impl::periodic_task_interval(const PeriodicTask task) const {
    return configuration_.interval(task);
}

void Controller::Impl::schedule_periodic_tasks(const std::string &host, bool value) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    verify_not_shutdown_();
    verify_known_host_(host, __func__);

    configuration_.schedule_periodic_tasks(host, value);
}

void Controller::Impl::reschedule_now(const std::string &host, PeriodicTask task) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    verify_not_shutdown_();
    verify_known_host_(host, __func__);

    periodic_tasks_scheduler_context_.reschedule_now(host, task);
}

void Controller::Impl::active_only_tasks(const std::string &host, bool value) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    verify_not_shutdown_();
    verify_known_host_(host, __func__);

    configuration_.active_only_tasks(host, value);
    periodic_tasks_scheduler_context_.reschedule_now(host, PeriodicTask::GET_TASKS);
}

std::future<bool> Controller::Impl::file_transfer_op(const std::string &host, FILE_TRANSFER_OP op,
                                                     const std::string &master_url, const std::string &filename) {
    check_not_empty_host_name__(host);
    check_not_empty__(master_url, "Missing master url");
    check_not_empty__(filename, "Missing filename");

    WOINC_LOCK_GUARD;

    auto future = create_and_schedule_async_job_<wrpc::FileTransferOpCommand, bool>(
        __func__,
        host,
        [](auto &r) { return r.success; },
        "Error while executing file transfer operation",
        {op, master_url, filename});

    periodic_tasks_scheduler_context_.reschedule_now(host, PeriodicTask::GET_FILE_TRANSFERS);

    return future;
}

std::future<bool> Controller::Impl::project_op(const std::string &host, PROJECT_OP op, const std::string &master_url) {
    check_not_empty_host_name__(host);
    check_not_empty__(master_url, "Missing master url");

    WOINC_LOCK_GUARD;

    auto future = create_and_schedule_async_job_<wrpc::ProjectOpCommand, bool>(
        __func__,
        host,
        [](auto &r) { return r.success; },
        "Error while executing project operation",
        {op, master_url});

    periodic_tasks_scheduler_context_.reschedule_now(host, PeriodicTask::GET_PROJECT_STATUS);

    return future;
}

std::future<bool> Controller::Impl::task_op(const std::string &host, TASK_OP op,
                               const std::string &master_url, const std::string &task_name) {
    check_not_empty_host_name__(host);
    check_not_empty__(master_url, "Missing master url");
    check_not_empty__(task_name, "Missing task name");

    WOINC_LOCK_GUARD;

    auto future = create_and_schedule_async_job_<rpc::TaskOpCommand, bool>(
        __func__,
        host,
        [](auto &r) { return r.success; },
        "Error while executing task operation",
        {op, master_url, task_name});

    periodic_tasks_scheduler_context_.reschedule_now(host, PeriodicTask::GET_TASKS);

    return future;
}

std::future<GlobalPreferences> Controller::Impl::load_global_preferences(const std::string &host, GET_GLOBAL_PREFS_MODE mode) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::GetGlobalPreferencesCommand, GlobalPreferences>(
        __func__,
        host,
        [](auto &r) { return r.preferences; },
        "Error while loading the preferences",
        {mode});
}

std::future<bool> Controller::Impl::save_global_preferences(const std::string &host, const GlobalPreferences &prefs, const GlobalPreferencesMask &mask) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::SetGlobalPreferencesCommand, bool>(
        __func__,
        host,
        [](auto &r) { return r.success; },
        "Error while setting the preferences",
        {prefs, mask});
}

std::future<bool> Controller::Impl::read_global_prefs_override(const std::string &host) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::ReadGlobalPreferencesOverrideCommand, bool>(
        __func__,
        host,
        [](auto &r) { return r.success; },
        "Error reading the preferences");
}

std::future<bool> Controller::Impl::run_mode(const std::string &host, RUN_MODE mode) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::SetRunModeCommand, bool>(
        __func__,
        host,
        [](auto &r) { return r.success; },
        "Error setting the run mode",
        mode);
}

std::future<bool> Controller::Impl::gpu_mode(const std::string &host, RUN_MODE mode) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::SetGpuModeCommand, bool>(
        __func__,
        host,
        [](auto &r) { return r.success; },
        "Error setting the gpu run mode",
        mode);
}

std::future<bool> Controller::Impl::network_mode(const std::string &host, RUN_MODE mode) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::SetNetworkModeCommand, bool>(
        __func__,
        host,
        [](auto &r) { return r.success; },
        "Error setting the network mode",
        mode);
}

std::future<AllProjectsList> Controller::Impl::all_projects_list(const std::string &host) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::GetAllProjectsListCommand, AllProjectsList>(
        __func__,
        host,
        [](auto &r) { return r.projects; },
        "Error getting the projects list");
}

std::future<bool> Controller::Impl::start_loading_project_config(const std::string &host, std::string master_url) {
    check_not_empty_host_name__(host);
    check_not_empty__(master_url, "Missing master url");

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::GetProjectConfigCommand, bool>(
        __func__,
        host,
        [](auto &r) { return r.success; },
        "Error loading the project config",
        {master_url});
}

std::future<ProjectConfig> Controller::Impl::poll_project_config(const std::string &host) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::GetProjectConfigPollCommand, ProjectConfig>(
        __func__,
        host,
        [](auto &r) { return r.project_config; },
        "Error polling the project config");
}

std::future<bool> Controller::Impl::start_account_lookup(const std::string &host, std::string master_url,
                                                         std::string email, std::string password) {
    check_not_empty_host_name__(host);
    check_not_empty__(master_url, "Missing master url");
    check_not_empty__(email, "Missing email");
    check_not_empty__(password, "Missing password");

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::LookupAccountCommand, bool>(
        __func__,
        host,
        [](auto &r) { return r.success; },
        "Error looking up the account info",
        {master_url, email, password});
}

std::future<AccountOut> Controller::Impl::poll_account_lookup(const std::string &host) {
    check_not_empty_host_name__(host);

    WOINC_LOCK_GUARD;

    return create_and_schedule_async_job_<wrpc::LookupAccountPollCommand, AccountOut>(
        __func__,
        host,
        [](auto &r) { return r.account_out; },
        "Error polling the account info");
}

void Controller::Impl::remove_host_(std::string host) {
    periodic_tasks_scheduler_context_.remove_host(host);
    host_controllers_.at(host)->shutdown();
    host_controllers_.erase(host);
    handler_registry_.for_host_handler([&](HostHandler &handler) { handler.on_host_removed(host); });
    configuration_.remove_host(host);
}

void Controller::Impl::async_remove_host_(std::string host) {
    WOINC_LOCK_GUARD;
    verify_not_shutdown_();
    remove_host_(host);
}

bool Controller::Impl::has_host_(const std::string &name) const {
    return host_controllers_.find(name) != host_controllers_.end();
}

#ifndef NDEBUG
void Controller::Impl::schedule_now_(const std::string &host, Job *job, const char *func) {
#else
void Controller::Impl::schedule_now_(const std::string &host, Job *job, const char *) {
#endif
    assert(job != nullptr);
    assert(func != nullptr);

    decltype(host_controllers_.find(host)) hc;

    try {
        verify_not_shutdown_();

        hc = host_controllers_.find(host);
        if (hc == host_controllers_.end()) {
#ifndef NDEBUG
            std::cerr << "Controller::" << func << " on non existing host \"" << host << "\" called\n";
#endif
            throw UnknownHostException{host};
        }
    } catch (...) {
        delete job;
        throw;
    }

    // not in the try-catch-block as the job queue takes ownership of the job
    hc->second->schedule_now(job);
}

void Controller::Impl::verify_not_shutdown_() const {
    if (shutdown_)
        throw ShutdownException();
}

#ifndef NDEBUG
void Controller::Impl::verify_known_host_(const std::string &host, const char *func) const {
#else
void Controller::Impl::verify_known_host_(const std::string &host, const char *) const {
#endif
    assert(func != nullptr);
    if (!has_host_(host)) {
#ifndef NDEBUG
        std::cerr << "Controller::" << func << " on non existing host \"" << host << "\" called\n";
#endif
        throw UnknownHostException{host};
    }
}


// ---- Controller ----

Controller::Controller()
    : impl_(new Impl)
{}

Controller::~Controller() {
    shutdown();
}

void Controller::shutdown() {
    impl_->shutdown();
}

void Controller::register_handler(HostHandler *handler) {
    impl_->register_handler(handler);
}

void Controller::deregister_handler(HostHandler *handler) {
    impl_->deregister_handler(handler);
}

void Controller::register_handler(PeriodicTaskHandler *handler) {
    impl_->register_handler(handler);
}

void Controller::deregister_handler(PeriodicTaskHandler *handler) {
    impl_->deregister_handler(handler);
}

void Controller::add_host(const std::string &host,
                          const std::string &url,
                          std::uint16_t port) {
    impl_->add_host(host, url, port);
}

void Controller::authorize_host(const std::string &host,
                                const std::string &password) {
    impl_->authorize_host(host, password);
}

void Controller::remove_host(const std::string &host) {
    impl_->remove_host(host);
}

void Controller::async_remove_host(std::string host) {
    impl_->async_remove_host(host);
}

void Controller::periodic_task_interval(const PeriodicTask task, int interval) {
    impl_->periodic_task_interval(task, interval);
}

int Controller::periodic_task_interval(const PeriodicTask task) const {
    return impl_->periodic_task_interval(task);
}

void Controller::schedule_periodic_tasks(const std::string &host, bool value) {
    impl_->schedule_periodic_tasks(host, value);
}

void Controller::reschedule_now(const std::string &host, PeriodicTask task) {
    impl_->reschedule_now(host, task);
}

void Controller::active_only_tasks(const std::string &host, bool value) {
    impl_->active_only_tasks(host, value);
}

std::future<bool> Controller::file_transfer_op(const std::string &host, FILE_TRANSFER_OP op,
                                               const std::string &master_url, const std::string &filename) {
    return impl_->file_transfer_op(host, op, master_url, filename);
}

std::future<bool> Controller::project_op(const std::string &host, PROJECT_OP op, const std::string &master_url) {
    return impl_->project_op(host, op, master_url);
}

std::future<bool> Controller::task_op(const std::string &host, TASK_OP op,
                         const std::string &master_url, const std::string &task_name) {
    return impl_->task_op(host, op, master_url, task_name);
}

std::future<GlobalPreferences> Controller::load_global_preferences(const std::string &host,
                                                                   GET_GLOBAL_PREFS_MODE mode) {
    return impl_->load_global_preferences(host, mode);
}

std::future<bool> Controller::save_global_preferences(const std::string &host,
                                                      const GlobalPreferences &prefs,
                                                      const GlobalPreferencesMask &mask) {
    return impl_->save_global_preferences(host, prefs, mask);
}

std::future<bool> Controller::read_global_prefs_override(const std::string &host) {
    return impl_->read_global_prefs_override(host);
}

std::future<bool> Controller::gpu_mode(const std::string &host, RUN_MODE mode) {
    return impl_->gpu_mode(host, mode);
}

std::future<bool> Controller::network_mode(const std::string &host, RUN_MODE mode) {
    return impl_->network_mode(host, mode);
}

std::future<bool> Controller::run_mode(const std::string &host, RUN_MODE mode) {
    return impl_->run_mode(host, mode);
}

std::future<AllProjectsList> Controller::all_projects_list(const std::string &host) {
    return impl_->all_projects_list(host);
}

std::future<bool> Controller::start_loading_project_config(const std::string &host, std::string master_url) {
    return impl_->start_loading_project_config(host, std::move(master_url));
}

std::future<ProjectConfig> Controller::poll_project_config(const std::string &host) {
    return impl_->poll_project_config(host);
}

std::future<bool> Controller::start_account_lookup(const std::string &host, std::string master_url,
                                                   std::string email, std::string password) {
    return impl_->start_account_lookup(host, std::move(master_url), std::move(email), std::move(password));
}

std::future<AccountOut> Controller::poll_account_lookup(const std::string &host) {
    return impl_->poll_account_lookup(host);
}

}}
