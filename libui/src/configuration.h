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
            std::chrono::seconds(1),    // GetCCStatus
            std::chrono::seconds(3600), // GetClientState
            std::chrono::seconds(60),   // GetDiskUsage
            std::chrono::seconds(1),    // GetFileTransfers
            std::chrono::seconds(1),    // GetMessages
            std::chrono::seconds(60),   // GetNotices
            std::chrono::seconds(1),    // GetProjectStatus
            std::chrono::seconds(60),   // GetStatistics
            std::chrono::seconds(1)     // GetTasks
        };

        struct HostConfiguration {
            bool schedule_periodic_tasks = false;
            bool active_only_tasks_ = false;
        };

        std::map<std::string, HostConfiguration> host_configurations_;
};

}}

#endif
