/* woinc/ui/handler.h --
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

#ifndef WOINC_UI_HANDLER_H_
#define WOINC_UI_HANDLER_H_

#include <string>

#include <woinc/types.h>
#include <woinc/ui/defs.h>

namespace woinc { namespace ui {

/*
 * Handles the life cycle of hosts. This handler must be implemented threadsafe.
 *
 * Controller::add_host()
 *  -> calls on_host_added() after adding the host to internal data structures
 *  -> may call on_host_connected() after establishing the tcp connection
 *    -> handler should trigger authentication to the host by calling Controller::auth_host(), may be called back by:
 *      -> on_host_authorized(): handler should trigger periodic tasks scheduling
 *      -> on_host_authorization_failed(): retry with another password or remove the host by calling Controller::async_remove_host
 *
 *  Each step may call on_host_error() and the handler should trigger
 *  removing the host by calling Controller::async_remove_host()
 */
struct HostHandler {
    virtual ~HostHandler() = default;

    virtual void on_host_added(const std::string &/*host*/) {};
    virtual void on_host_removed(const std::string &/*host*/) {};

    virtual void on_host_connected(const std::string &/*host*/) {};

    virtual void on_host_authorized(const std::string &/*host*/) {};
    virtual void on_host_authorization_failed(const std::string &/*host*/) {};

    virtual void on_host_error(const std::string &/*host*/, Error /*error*/) {};
};

/*
 * Handles the periodically updates of entities.
 *
 * This handler will not be called more than once at the same time
 * and not concurrently to HostHandler::on_host_removed().
 */
struct PeriodicTaskHandler {
    virtual ~PeriodicTaskHandler() = default;

    virtual void on_update(const std::string & /*host*/, const woinc::CCStatus &      /*cc_status*/) {};
    virtual void on_update(const std::string & /*host*/, const woinc::ClientState &   /*client_state*/) {};
    virtual void on_update(const std::string & /*host*/, const woinc::DiskUsage &     /*disk_usage*/) {};
    virtual void on_update(const std::string & /*host*/, const woinc::FileTransfers & /*file_transfers*/) {};
    virtual void on_update(const std::string & /*host*/, const woinc::Messages &      /*messages*/) {};
    virtual void on_update(const std::string & /*host*/, const woinc::Notices &       /*notices*/, bool /*refreshed*/) {};
    virtual void on_update(const std::string & /*host*/, const woinc::Projects &      /*projects*/) {};
    virtual void on_update(const std::string & /*host*/, const woinc::Statistics &    /*statistics*/) {};
    virtual void on_update(const std::string & /*host*/, const woinc::Tasks &         /*tasks*/) {};
};

}}

#endif
