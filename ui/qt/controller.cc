/* ui/qt/controller.cc --
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

#include "qt/controller.h"

#include <algorithm>
#include <cassert>
#include <exception>
#include <chrono>

#ifndef NDEBUG
#include <iostream>
#endif

#include <QTimer>

#include <woinc/ui/controller.h>

#include "qt/adapter.h"

#define WOINC_LOCK_GUARD std::lock_guard<decltype(mutex_)> guard(mutex_)

namespace {

enum {
    SUBSCRIPTION_POLLING_INTERVAL_MSEC = 100
};

using Controller = woinc::ui::qt::Controller;

class Subscription {
    public:
        Subscription(Controller::ErrorHandler error_handler, int timeout_msecs)
            : error_handler_(std::move(error_handler))
            , remaining_tries_(timeout_msecs / SUBSCRIPTION_POLLING_INTERVAL_MSEC)
        { assert(remaining_tries_ > 0); }
        virtual ~Subscription() = default;

        Subscription(const Subscription &) = delete;
        Subscription &operator=(const Subscription &) = delete;

        bool tick() {
            return status_() == std::future_status::ready || --remaining_tries_ == 0;
        }

        void finish() {
            if (remaining_tries_ == 0)
                fail_(QString());
            else
                finish_();
        }

    protected:
        void fail_(QString err) {
            error_handler_(std::move(err));
        }

        virtual std::future_status status_() = 0;
        virtual void finish_() = 0;

    private:
        Controller::ErrorHandler error_handler_;
        int remaining_tries_;
};

template<typename SUBJECT>
class SubscriptionImpl : public Subscription {
    public:
        SubscriptionImpl(Controller::Receiver<SUBJECT> receiver,
                         Controller::ErrorHandler error_handler,
                         std::future<SUBJECT> &&future,
                         int timeout_msecs = 30 * 1000)
            : Subscription(std::move(error_handler), timeout_msecs)
            , receiver_(std::move(receiver))
            , future_(std::move(future)) {}
        virtual ~SubscriptionImpl() = default;

    private:
        std::future_status status_() final {
            return future_.wait_for(std::chrono::seconds(0));
        }

        void finish_() final {
            try {
                receiver_(std::move(future_.get()));
            } catch (woinc::ui::ShutdownException &) {
                fail_(QString::fromUtf8("Not connected to host"));
            } catch (woinc::ui::UnknownHostException &e) {
                fail_(QString::fromUtf8("Not connected to host %1").arg(QString::fromStdString(e.host)));
            } catch (const std::exception &err) {
                fail_(QString::fromStdString(err.what()));
            }
        };

    private:
        Controller::Receiver<SUBJECT> receiver_;
        std::future<SUBJECT> future_;
};

}

namespace woinc { namespace ui { namespace qt {

// ----- Controller::Poller -----

class Controller::Poller {
    public:
        Poller() {
            timer_.setInterval(SUBSCRIPTION_POLLING_INTERVAL_MSEC);

            QObject::connect(&timer_, &QTimer::timeout, [=]() {
                // we have to collect the subscriptions to be finished,
                // because we don't know what the receiver will do and
                // thus we've to call it without locking the poller
                std::vector<std::unique_ptr<Subscription>> to_finish;
                {
                    WOINC_LOCK_GUARD;
                    for (auto iter = subscriptions_.begin(); iter != subscriptions_.end();) {
                        if ((*iter)->tick()) {
                            to_finish.push_back(std::move(*iter));
                            iter = subscriptions_.erase(iter);
                        } else {
                            ++ iter;
                        }
                    }
                    if (subscriptions_.empty())
                        timer_.stop();
                }
                for (auto &&iter : to_finish)
                    iter->finish();
            });
        }

        void add(Subscription *subscription) {
            WOINC_LOCK_GUARD;
            subscriptions_.push_back(std::unique_ptr<Subscription>(subscription));
            if (!timer_.isActive())
                timer_.start();
        }

        void stop() {
            timer_.stop();
        }

    private:
        std::mutex mutex_;
        QTimer timer_;
        std::vector<std::unique_ptr<Subscription>> subscriptions_;
};

// ----- Controller -----

Controller::Controller(QObject *parent)
    : QObject(parent)
    , ctrl_(new woinc::ui::Controller)
    , poller_(new Poller)
{}

Controller::~Controller() = default;

void Controller::register_handler(HostHandler *handler){
    ctrl_->register_handler(handler);
}

void Controller::deregister_handler(HostHandler *handler){
    ctrl_->deregister_handler(handler);
}

void Controller::register_handler(PeriodicTaskHandler *handler){
    ctrl_->register_handler(handler);
}

void Controller::deregister_handler(PeriodicTaskHandler *handler){
    ctrl_->deregister_handler(handler);
}

void Controller::load_global_prefs(const QString &host, GetGlobalPrefsMode mode,
                                   Receiver<GlobalPreferences> receiver, ErrorHandler error_handler) {
    poller_->add(new SubscriptionImpl<GlobalPreferences>(std::move(receiver),
                                                         std::move(error_handler),
                                                         ctrl_->load_global_preferences(host.toStdString(), mode)));
}

void Controller::save_global_prefs(const QString &host,
                                   const GlobalPreferences &prefs,
                                   const GlobalPreferencesMask &mask,
                                   Receiver<bool> receiver,
                                   ErrorHandler error_handler) {
    poller_->add(new SubscriptionImpl<bool>(std::move(receiver),
                                            std::move(error_handler),
                                            ctrl_->save_global_preferences(host.toStdString(), prefs, mask)));
}

void Controller::read_global_prefs(const QString &host, Receiver<bool> receiver, ErrorHandler error_handler) {
    poller_->add(new SubscriptionImpl<bool>(std::move(receiver),
                                            std::move(error_handler),
                                            ctrl_->read_global_prefs_override(host.toStdString())));
}

void Controller::load_cc_config(const QString &host, Receiver<CCConfig> receiver, ErrorHandler error_handler) {
    poller_->add(new SubscriptionImpl<CCConfig>(std::move(receiver),
                                                std::move(error_handler),
                                                ctrl_->cc_config(host.toStdString())));
}

void Controller::save_cc_config(const QString &host, const CCConfig &cc_config, Receiver<bool> receiver, ErrorHandler error_handler) {
    poller_->add(new SubscriptionImpl<bool>(std::move(receiver),
                                            std::move(error_handler),
                                            ctrl_->cc_config(host.toStdString(), cc_config)));
}

void Controller::read_config_files(const QString &host, Receiver<bool> receiver, ErrorHandler error_handler) {
    poller_->add(new SubscriptionImpl<bool>(std::move(receiver),
                                            std::move(error_handler),
                                            ctrl_->read_config_files(host.toStdString())));
}

void Controller::load_all_projects_list(const QString &host, Receiver<AllProjectsList> receiver, ErrorHandler error_handler) {
    poller_->add(new SubscriptionImpl<AllProjectsList>(std::move(receiver),
                                                       std::move(error_handler),
                                                       std::move(ctrl_->all_projects_list(host.toStdString()))));
}

void Controller::start_loading_project_config(const QString &host, const QString &master_url, Receiver<bool> receiver, ErrorHandler error_handler) {
    poller_->add(new SubscriptionImpl<bool>(std::move(receiver),
                                            std::move(error_handler),
                                            std::move(ctrl_->start_loading_project_config(host.toStdString(), master_url.toStdString()))));
}

void Controller::poll_project_config(const QString &host, Receiver<ProjectConfig> receiver, ErrorHandler error_handler) {
    poller_->add(new SubscriptionImpl<ProjectConfig>(std::move(receiver),
                                                     std::move(error_handler),
                                                     std::move(ctrl_->poll_project_config(host.toStdString()))));
}

void Controller::start_account_lookup(const QString &host, const QString &master_url,
                                      const QString &email, const QString &password,
                                      Receiver<bool> receiver, ErrorHandler error_handler) {
    poller_->add(new SubscriptionImpl<bool>(std::move(receiver),
                                            std::move(error_handler),
                                            std::move(ctrl_->start_account_lookup(host.toStdString(), master_url.toStdString(),
                                                                                  email.toStdString(), password.toStdString()))));
}

void Controller::poll_account_lookup(const QString &host, Receiver<AccountOut> receiver, ErrorHandler error_handler) {
    poller_->add(new SubscriptionImpl<AccountOut>(std::move(receiver),
                                                  std::move(error_handler),
                                                  std::move(ctrl_->poll_account_lookup(host.toStdString()))));
}

void Controller::attach_project(const QString &host, const QString &master_url, const QString &account_key,
                                Receiver<bool> receiver, ErrorHandler error_handler) {
    poller_->add(new SubscriptionImpl<bool>(std::move(receiver),
                                            std::move(error_handler),
                                            std::move(ctrl_->attach_project(host.toStdString(), master_url.toStdString(), account_key.toStdString()))));
}

void Controller::add_host(QString host, QString url, unsigned short port, QString password) {
    {
        WOINC_LOCK_GUARD;
        pending_logins_.push_back({host, password});
    }

    try {
        ctrl_->add_host(host.toStdString(), url.toStdString(), port);
    } catch (const std::exception &e) {
        emit error_occurred("Error", e.what());
    }
}

void Controller::trigger_ui_shutdown() {
    ctrl_->shutdown();
}

void Controller::trigger_client_shutdown(QString host) {
    ctrl_->schedule_periodic_tasks(host.toStdString(), false);
    auto result = ctrl_->quit(host.toStdString());
    // TODO don't block here
    if (result.get())
        ctrl_->shutdown();
    else
        ctrl_->schedule_periodic_tasks(host.toStdString(), true);
}

void Controller::do_active_only_tasks(QString host, bool value) {
    ctrl_->active_only_tasks(host.toStdString(), value);
}

void Controller::do_file_transfer_op(QString host, FileTransferOp op, QString project_url, QString filename) {
    ctrl_->file_transfer_op(host.toStdString(), op, project_url.toStdString(), filename.toStdString());
}

void Controller::do_project_op(QString host, QString project_url, ProjectOp op) {
    ctrl_->project_op(host.toStdString(), op, project_url.toStdString());
}

void Controller::do_task_op(QString host, QString project_url, QString name, TaskOp op) {
    ctrl_->task_op(host.toStdString(), op, project_url.toStdString(), name.toStdString());
}

void Controller::set_gpu_mode(QString host, RunMode mode) {
    ctrl_->gpu_mode(host.toStdString(), mode);
}

void Controller::set_network_mode(QString host, RunMode mode) {
    ctrl_->network_mode(host.toStdString(), mode);
}

void Controller::set_run_mode(QString host, RunMode mode) {
    ctrl_->run_mode(host.toStdString(), mode);
}

void Controller::schedule_disk_usage_update(QString host) {
    ctrl_->reschedule_now(host.toStdString(), PeriodicTask::GetDiskUsage);
}

void Controller::schedule_projects_update(QString host) {
    ctrl_->reschedule_now(host.toStdString(), PeriodicTask::GetProjectStatus);
}

void Controller::schedule_state_update(QString host) {
    ctrl_->reschedule_now(host.toStdString(), PeriodicTask::GetClientState);
}

void Controller::schedule_tasks_update(QString host) {
    ctrl_->reschedule_now(host.toStdString(), PeriodicTask::GetTasks);
}

void Controller::schedule_statistics_update(QString host) {
    ctrl_->reschedule_now(host.toStdString(), PeriodicTask::GetStatistics);
}

void Controller::run_cpu_benchmarks(QString host) {
    ctrl_->run_benchmarks(host.toStdString());
}

void Controller::retry_pending_transfers(QString host) {
    ctrl_->network_available(host.toStdString());
}

void Controller::connect(const HandlerAdapter *adapter) const {
#define WOINC_CONNECT(FROM, TO) QObject::connect(adapter, &HandlerAdapter::FROM, \
                                                 this, &Controller::TO, \
                                                 Qt::QueuedConnection)
    WOINC_CONNECT(connected, handle_host_connected);
    WOINC_CONNECT(authorized, handle_host_authorized);
    WOINC_CONNECT(authorization_failed, handle_host_authorization_failed);
    WOINC_CONNECT(error_occurred, handle_host_error);
#undef WOINC_CONNECT
}

void Controller::handle_host_connected(QString host) {
    WOINC_LOCK_GUARD;

    auto credentials = std::find_if(pending_logins_.begin(), pending_logins_.end(), [&](const auto &t) {
        return t.first == host;
    });

    bool auth = credentials != pending_logins_.end();
    assert(auth);

    if (auth)
        ctrl_->authorize_host(credentials->first.toStdString(), credentials->second.toStdString());
}

void Controller::handle_host_authorized(QString host) {
    {
        WOINC_LOCK_GUARD;
        pending_logins_.erase(std::remove_if(pending_logins_.begin(), pending_logins_.end(), [&](const auto &t) {
            return t.first == host;
        }), pending_logins_.end());
    }
    ctrl_->schedule_periodic_tasks(host.toStdString(), true);
}

void Controller::handle_host_authorization_failed(QString host) {
    {
        WOINC_LOCK_GUARD;
        pending_logins_.erase(std::remove_if(pending_logins_.begin(), pending_logins_.end(), [&](const auto &t) {
            return t.first == host;
        }), pending_logins_.end());
    }
    emit error_occurred(QString::fromUtf8("Authorization failed"),
                        QString("Authorization to host \"%1\" failed.").arg(host));
    ctrl_->async_remove_host(host.toStdString());
}

void Controller::handle_host_error(QString host, Error error) {
    QString msg;

    switch (error) {
        case Error::Disconnected:
        case Error::ConnectionError:
            msg = QString("Connection to host \"%1\" lost.").arg(host);
            break;
        case Error::Unauthorized:
            msg = QString("Not authorized to host \"%1\".").arg(host);
            break;
        case Error::ClientError:
        case Error::ParsingError:
            msg = QString("Communication error with host \"%1\".").arg(host);
            break;
        case Error::LogicError:
            msg = QString("Internal error handling host \"%1\".").arg(host);
            break;
    }

    emit error_occurred(QString::fromUtf8("An error occurred"),
                        msg /* + QString::fromUtf8(" The host will be removed from the list.") */);

    ctrl_->async_remove_host(host.toStdString());
}

}}}
