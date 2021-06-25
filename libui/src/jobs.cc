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

Error as_error__(wrpc::COMMAND_STATUS status) {
    switch (status) {
        case wrpc::COMMAND_STATUS::OK:               assert(false); return Error::LOGIC_ERROR;
        case wrpc::COMMAND_STATUS::DISCONNECTED:     return Error::DISCONNECTED;
        case wrpc::COMMAND_STATUS::UNAUTHORIZED:     return Error::UNAUTHORIZED;
        case wrpc::COMMAND_STATUS::CONNECTION_ERROR: return Error::CONNECTION_ERROR;
        case wrpc::COMMAND_STATUS::CLIENT_ERROR:     return Error::CLIENT_ERROR;
        case wrpc::COMMAND_STATUS::PARSING_ERROR:    return Error::PARSING_ERROR;
        case wrpc::COMMAND_STATUS::LOGIC_ERROR:      return Error::LOGIC_ERROR;
    }
    assert(false);
    return Error::LOGIC_ERROR;
}


template<typename CMD, typename GETTER>
void execute__(Client &client, const HandlerRegistry &handler_registry, CMD &&cmd, GETTER getter) {
    auto status = client.execute(cmd);
    if (status == wrpc::COMMAND_STATUS::OK) {
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
        case PeriodicTask::GET_CCSTATUS:
            execute__(client, handler_registry,
                      wrpc::GetCCStatusCommand(),
                      std::mem_fn(&wrpc::GetCCStatusResponse::cc_status));
            break;
        case PeriodicTask::GET_CLIENT_STATE:
            execute__(client, handler_registry,
                      wrpc::GetClientStateCommand(),
                      std::mem_fn(&wrpc::GetClientStateResponse::client_state));
            break;
        case PeriodicTask::GET_DISK_USAGE:
            execute__(client, handler_registry,
                      wrpc::GetDiskUsageCommand(),
                      std::mem_fn(&wrpc::GetDiskUsageResponse::disk_usage));
            break;
        case PeriodicTask::GET_FILE_TRANSFERS:
            execute__(client, handler_registry,
                      wrpc::GetFileTransfersCommand(),
                      std::mem_fn(&wrpc::GetFileTransfersResponse::file_transfers));
            break;
        case PeriodicTask::GET_MESSAGES:
            {
                wrpc::GetMessagesCommand cmd;
                cmd.request().seqno = payload.seqno;
                auto status = client.execute(cmd);
                if (status == wrpc::COMMAND_STATUS::OK) {
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
        case PeriodicTask::GET_NOTICES:
            {
                wrpc::GetNoticesCommand cmd;
                cmd.request().seqno = payload.seqno;
                auto status = client.execute(cmd);
                if (status == wrpc::COMMAND_STATUS::OK) {
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
        case PeriodicTask::GET_PROJECT_STATUS:
            execute__(client, handler_registry,
                      wrpc::GetProjectStatusCommand(),
                      std::mem_fn(&wrpc::GetProjectStatusResponse::projects));
            break;
        case PeriodicTask::GET_STATISTICS:
            execute__(client, handler_registry,
                      wrpc::GetStatisticsCommand(),
                      std::mem_fn(&wrpc::GetStatisticsResponse::statistics));
            break;
        case PeriodicTask::GET_TASKS:
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
        if (status == wrpc::COMMAND_STATUS::OK)
            handler.on_host_authorized(client.host());
        else if (status == wrpc::COMMAND_STATUS::UNAUTHORIZED)
            handler.on_host_authorization_failed(client.host());
        else
            handler.on_host_error(client.host(), as_error__(status));
    });
}

}}
