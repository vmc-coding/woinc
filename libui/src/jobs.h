/* libui/src/jobs.h --
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

#ifndef WOINC_UI_JOBS_H_
#define WOINC_UI_JOBS_H_

#include <future>
#include <memory>
#include <string>

#include <woinc/rpc_command.h>
#include <woinc/ui/defs.h>

#include "client.h"
#include "handler_registry.h"
#include "visibility.h"

namespace woinc { namespace ui {

struct WOINCUI_LOCAL Job;

struct WOINCUI_LOCAL PostExecutionHandler {
    virtual ~PostExecutionHandler() = default;
    virtual void handle_post_execution(const std::string &host, Job *) = 0;
};

struct WOINCUI_LOCAL Job {
    virtual ~Job() = default;

    virtual void execute(Client &client) = 0;

    void operator()(Client &client);

    void register_post_execution_handler(PostExecutionHandler *handler);

    private:
        PostExecutionHandler *post_handler_ = nullptr;
};

struct WOINCUI_LOCAL PeriodicJob : public Job {
    union Payload {
        bool active_only;
        int seqno;
    };

    PeriodicJob(PeriodicTask t, const HandlerRegistry &handler_registry, const Payload &payload = Payload());
    virtual ~PeriodicJob() = default;

    void execute(Client &client) final;

    const PeriodicTask task;
    const HandlerRegistry &handler_registry;

    Payload payload;
};

struct WOINCUI_LOCAL AuthorizationJob : public Job {
    AuthorizationJob(const std::string &password, const HandlerRegistry &handler_registry);
    virtual ~AuthorizationJob() = default;

    void execute(Client &client) final;

    private:
        const std::string password_;
        const HandlerRegistry &handler_registry_;
};

// wrap async commands that request data from the client; errors should be propagated through the future by the handler
template<typename RESULT>
struct WOINCUI_LOCAL AsyncJob : public Job {
    typedef std::promise<RESULT> Promise;
    typedef std::function<void(woinc::rpc::Command *cmd,
                               Promise &promise,
                               woinc::rpc::CommandStatus status)> ResultHandler;

    // the job takes the ownership of the command
    AsyncJob(woinc::rpc::Command *cmd, Promise promise, ResultHandler handler)
        : cmd_(cmd), promise_(std::move(promise)), handler_(std::move(handler)) {}
    virtual ~AsyncJob() = default;

    void execute(Client &client) final {
        handler_(cmd_.get(), promise_, client.execute(*cmd_));
    }

    private:
        std::unique_ptr<woinc::rpc::Command> cmd_;
        Promise promise_;
        ResultHandler handler_;
};

}}

#endif
