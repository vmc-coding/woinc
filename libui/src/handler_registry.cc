/* libui/src/handler_registry.cc --
   Written and Copyright (C) 2019-2022 by vmc.

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

#include "handler_registry.h"

#include <algorithm>

namespace woinc { namespace ui {

#define WOINC_LOCK_GUARD std::lock_guard<decltype(mutex_)> guard(mutex_)

void HandlerRegistry::register_handler(HostHandler *handler) {
    WOINC_LOCK_GUARD;
    host_handler_.push_back(handler);
}

void HandlerRegistry::deregister_handler(HostHandler *handler) {
    WOINC_LOCK_GUARD;
    host_handler_.erase(std::remove(host_handler_.begin(),
                                    host_handler_.end(),
                                    handler),
                        host_handler_.end());
}

void HandlerRegistry::register_handler(PeriodicTaskHandler *handler) {
    WOINC_LOCK_GUARD;
    periodic_task_handler_.push_back(handler);
}

void HandlerRegistry::deregister_handler(PeriodicTaskHandler *handler) {
    WOINC_LOCK_GUARD;
    periodic_task_handler_.erase(std::remove(periodic_task_handler_.begin(),
                                             periodic_task_handler_.end(),
                                             handler),
                                 periodic_task_handler_.end());
}

void HandlerRegistry::for_host_handler(std::function<void(HostHandler &handler)> f) const {
    WOINC_LOCK_GUARD;
    for (auto &&handler : host_handler_)
        f(*handler);
}

void HandlerRegistry::for_periodic_task_handler(std::function<void(PeriodicTaskHandler &handler)> f) const {
    WOINC_LOCK_GUARD;
    for (auto &&handler : periodic_task_handler_)
        f(*handler);
}

}}
