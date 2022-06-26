/* libui/src/configuration.h --
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

#ifndef WOINC_UI_CONFIGURATION_H_
#define WOINC_UI_CONFIGURATION_H_

#include <array>
#include <chrono>
#include <map>
#include <mutex>
#include <string>

#include <woinc/ui/defs.h>

#include "visibility.h"

namespace woinc { namespace ui {

class WOINCUI_LOCAL Configuration {
    public:
        typedef std::array<std::chrono::milliseconds, 9> Intervals;

    public:
        void interval(PeriodicTask task, std::chrono::milliseconds ms);
        std::chrono::milliseconds interval(PeriodicTask task) const;

        Intervals intervals() const;

        void active_only_tasks(const std::string &host, bool value);
        bool active_only_tasks(const std::string &host) const;

        void schedule_periodic_tasks(const std::string &host, bool value);
        bool schedule_periodic_tasks(const std::string &host) const;

    private:
        friend class Controller;
        void add_host(std::string host);
        void remove_host(const std::string &host);

    private:
        mutable std::mutex mutex_;

        Intervals intervals_ = {
            std::chrono::milliseconds(1 * 1000),    // GetCCStatus
            std::chrono::milliseconds(3600 * 1000), // GetClientState
            std::chrono::milliseconds(60 * 1000),   // GetDiskUsage
            std::chrono::milliseconds(1 * 1000),    // GetFileTransfers
            std::chrono::milliseconds(1 * 1000),    // GetMessages
            std::chrono::milliseconds(60 * 1000),   // GetNotices
            std::chrono::milliseconds(1 * 1000),    // GetProjectStatus
            std::chrono::milliseconds(60 * 1000),   // GetStatistics
            std::chrono::milliseconds(1 * 1000)     // GetTasks
        };

        struct HostConfiguration {
            bool schedule_periodic_tasks = false;
            bool active_only_tasks_ = false;
        };

        std::map<std::string, HostConfiguration> host_configurations_;
};

}}

#endif
