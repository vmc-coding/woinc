/* lib/rpc_parsing.h --
   Written and Copyright (C) 2017, 2018 by vmc.

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

#ifndef WOINC_RPC_PARSING_H_
#define WOINC_RPC_PARSING_H_

#include <woinc/types.h>

#include "visibility.h"
#include "xml.h"

namespace woinc { namespace rpc {

bool WOINC_LOCAL parse(const woinc::xml::Node &node, woinc::CCStatus &cc_status);
bool WOINC_LOCAL parse(const woinc::xml::Node &node, woinc::ClientState &client_state);
bool WOINC_LOCAL parse(const woinc::xml::Node &node, woinc::DiskUsage &disk_usage);
bool WOINC_LOCAL parse(const woinc::xml::Node &node, woinc::FileTransfer &file_transfer);
bool WOINC_LOCAL parse(const woinc::xml::Node &node, woinc::GlobalPreferences &global_preferences);
bool WOINC_LOCAL parse(const woinc::xml::Node &node, woinc::HostInfo &info);
bool WOINC_LOCAL parse(const woinc::xml::Node &node, woinc::Message &msg);
bool WOINC_LOCAL parse(const woinc::xml::Node &node, woinc::Notice &notice);
bool WOINC_LOCAL parse(const woinc::xml::Node &node, woinc::Project &project);
bool WOINC_LOCAL parse(const woinc::xml::Node &node, woinc::Statistics &statistics);
bool WOINC_LOCAL parse(const woinc::xml::Node &node, woinc::Task &task);
bool WOINC_LOCAL parse(const woinc::xml::Node &node, woinc::Version &version);
bool WOINC_LOCAL parse(const woinc::xml::Node &node, woinc::Workunit &workunit);

}}

#endif
