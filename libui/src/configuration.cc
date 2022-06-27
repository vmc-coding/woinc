/* libui/src/configuration.cc --
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

#include "configuration.h"

#include <algorithm>
#include <cassert>

#define WOINC_CONFIGURATION_LOCK_GUARD std::lock_guard<decltype(mutex_)> guard(mutex_)

namespace woinc { namespace ui {

void Configuration::interval(PeriodicTask task, Interval duration) {
    WOINC_CONFIGURATION_LOCK_GUARD;
    intervals_[static_cast<size_t>(task)] = duration;
}

Configuration::Interval Configuration::interval(PeriodicTask task) const {
    WOINC_CONFIGURATION_LOCK_GUARD;
    return intervals_.at(static_cast<size_t>(task));
}

Configuration::Intervals Configuration::intervals() const {
    WOINC_CONFIGURATION_LOCK_GUARD;
    return intervals_;
}

void Configuration::active_only_tasks(const std::string &host, bool value) {
    WOINC_CONFIGURATION_LOCK_GUARD;
    assert(host_configurations_.find(host) != host_configurations_.end());
    host_configurations_.at(host).active_only_tasks_ = value;
}

bool Configuration::active_only_tasks(const std::string &host) const {
    WOINC_CONFIGURATION_LOCK_GUARD;
    assert(host_configurations_.find(host) != host_configurations_.end());
    return host_configurations_.at(host).active_only_tasks_;
}

void Configuration::schedule_periodic_tasks(const std::string &host, bool value) {
    WOINC_CONFIGURATION_LOCK_GUARD;
    assert(host_configurations_.find(host) != host_configurations_.end());
    host_configurations_.at(host).schedule_periodic_tasks = value;
}

bool Configuration::schedule_periodic_tasks(const std::string &host) const {
    WOINC_CONFIGURATION_LOCK_GUARD;
    assert(host_configurations_.find(host) != host_configurations_.end());
    return host_configurations_.at(host).schedule_periodic_tasks;
}

void Configuration::add_host(std::string host) {
    WOINC_CONFIGURATION_LOCK_GUARD;
    assert(host_configurations_.find(host) == host_configurations_.end());
    host_configurations_.emplace(std::move(host), HostConfiguration());
}

void Configuration::remove_host(const std::string &host) {
    WOINC_CONFIGURATION_LOCK_GUARD;
    assert(host_configurations_.find(host) != host_configurations_.end());
    host_configurations_.erase(host);
}

}}
