/* libui/src/jobs.cc --
   Written and Copyright (C) 2017-2021 by vmc.

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

#include "jobs.h"

#include <cassert>
#include <functional>

namespace wrpc = woinc::rpc;

namespace {

using namespace woinc::ui;

Error as_error__(wrpc::CommandStatus status) {
    switch (status) {
        case wrpc::CommandStatus::Ok:              assert(false); return Error::LogicError;
        case wrpc::CommandStatus::Disconnected:    return Error::Disconnected;
        case wrpc::CommandStatus::Unauthorized:    return Error::Unauthorized;
        case wrpc::CommandStatus::ConnectionError: return Error::ConnectionError;
        case wrpc::CommandStatus::ClientError:     return Error::ClientError;
        case wrpc::CommandStatus::ParsingError:    return Error::ParsingError;
        case wrpc::CommandStatus::LogicError:      return Error::LogicError;
    }
    assert(false);
    return Error::LogicError;
}


template<typename CMD, typename GETTER>
void execute__(Client &client, const HandlerRegistry &handler_registry, CMD &&cmd, GETTER getter) {
    auto status = client.execute(cmd);
    if (status == wrpc::CommandStatus::Ok) {
        handler_registry.for_periodic_task_handler([&](auto &handler) {
            handler.on_update(client.host(), getter(cmd.response()));
        });
    } else {
        handler_registry.for_host_handler([&](auto &handler) {
            handler.on_host_error(client.host(), as_error__(status));
        });
    }
}

}

namespace woinc { namespace ui {

// ---- Job ----

void Job::operator()(Client &client) {
    execute(client);

    if (post_handler_)
        post_handler_->handle_post_execution(client.host(), this);
}

void Job::register_post_execution_handler(PostExecutionHandler *handler) {
    post_handler_ = handler;
}

// ---- PeriodicJob ----

PeriodicJob::PeriodicJob(PeriodicTask t, const HandlerRegistry &hr, const Payload &p)
    : task(t), handler_registry(hr), payload(p)
{}

void PeriodicJob::execute(Client &client) {
    switch (task) {
        case PeriodicTask::GetCCStatus:
            execute__(client, handler_registry,
                      wrpc::GetCCStatusCommand(),
                      std::mem_fn(&wrpc::GetCCStatusResponse::cc_status));
            break;
        case PeriodicTask::GetClientState:
            execute__(client, handler_registry,
                      wrpc::GetClientStateCommand(),
                      std::mem_fn(&wrpc::GetClientStateResponse::client_state));
            break;
        case PeriodicTask::GetDiskUsage:
            execute__(client, handler_registry,
                      wrpc::GetDiskUsageCommand(),
                      std::mem_fn(&wrpc::GetDiskUsageResponse::disk_usage));
            break;
        case PeriodicTask::GetFileTransfers:
            execute__(client, handler_registry,
                      wrpc::GetFileTransfersCommand(),
                      std::mem_fn(&wrpc::GetFileTransfersResponse::file_transfers));
            break;
        case PeriodicTask::GetMessages:
            {
                wrpc::GetMessagesCommand cmd;
                cmd.request().seqno = payload.seqno;
                auto status = client.execute(cmd);
                if (status == wrpc::CommandStatus::Ok) {
                    if (!cmd.response().messages.empty()) {
                        payload.seqno = cmd.response().messages.back().seqno;
                        handler_registry.for_periodic_task_handler([&](auto &handler) {
                            handler.on_update(client.host(), cmd.response().messages);
                        });
                    }
                } else {
                    handler_registry.for_host_handler([&](auto &handler) {
                        handler.on_host_error(client.host(), as_error__(status));
                    });
                }
            }
            break;
        case PeriodicTask::GetNotices:
            {
                wrpc::GetNoticesCommand cmd;
                cmd.request().seqno = payload.seqno;
                auto status = client.execute(cmd);
                if (status == wrpc::CommandStatus::Ok) {
                    if (!cmd.response().notices.empty()) {
                        payload.seqno = cmd.response().notices.back().seqno;
                        handler_registry.for_periodic_task_handler([&](auto &handler) {
                            handler.on_update(client.host(), cmd.response().notices, cmd.response().refreshed);
                        });
                    }
                } else {
                    handler_registry.for_host_handler([&](auto &handler) {
                        handler.on_host_error(client.host(), as_error__(status));
                    });
                }
            }
            break;
        case PeriodicTask::GetProjectStatus:
            execute__(client, handler_registry,
                      wrpc::GetProjectStatusCommand(),
                      std::mem_fn(&wrpc::GetProjectStatusResponse::projects));
            break;
        case PeriodicTask::GetStatistics:
            execute__(client, handler_registry,
                      wrpc::GetStatisticsCommand(),
                      std::mem_fn(&wrpc::GetStatisticsResponse::statistics));
            break;
        case PeriodicTask::GetTasks:
            {
#if 0
                static int foo = 0;
                if (++foo == 5) {
                    handler_registry.for_periodic_task_handler([&](auto &handler) {
                        handler.on_host_error(client.host(), Error::CLIENT_ERROR);
                    });
                    break;
                }
#endif
                wrpc::GetResultsCommand cmd;
                cmd.request().active_only = payload.active_only;
                execute__(client, handler_registry,
                          std::move(cmd), std::mem_fn(&wrpc::GetResultsResponse::tasks));
            }
            break;
    }
}

// ---- AuthorizationJob ----

AuthorizationJob::AuthorizationJob(const std::string &password, const HandlerRegistry &handler_registry)
    : password_(password), handler_registry_(handler_registry)
{}

void AuthorizationJob::execute(Client &client) {
    woinc::rpc::AuthorizeCommand cmd;
    cmd.request().password = password_;

    auto status = client.execute(cmd);
    handler_registry_.for_host_handler([&](auto &handler) {
        if (status == wrpc::CommandStatus::Ok)
            handler.on_host_authorized(client.host());
        else if (status == wrpc::CommandStatus::Unauthorized)
            handler.on_host_authorization_failed(client.host());
        else
            handler.on_host_error(client.host(), as_error__(status));
    });
}

}}
