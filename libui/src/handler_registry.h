/* libui/src/handler_registry.h --
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

#ifndef WOINC_UI_HANDLER_REGISTRY_H_
#define WOINC_UI_HANDLER_REGISTRY_H_

#include <functional>
#include <mutex>
#include <vector>

#include <woinc/ui/handler.h>

#include "visibility.h"

namespace woinc { namespace ui {

class WOINCUI_LOCAL HandlerRegistry {
    public:
        void register_handler(HostHandler *handler);
        void deregister_handler(HostHandler *handler);

        void register_handler(PeriodicTaskHandler *handler);
        void deregister_handler(PeriodicTaskHandler *handler);

    public:
        void for_host_handler(std::function<void(HostHandler &handler)> f) const;
        void for_periodic_task_handler(std::function<void(PeriodicTaskHandler &handler)> f) const;

    private:
        mutable std::mutex mutex_;

        std::vector<HostHandler *> host_handler_;
        std::vector<PeriodicTaskHandler *> periodic_task_handler_;
};

}}

#endif
