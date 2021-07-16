/* ui/common/types_to_string.h --
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

#ifndef WOINC_UI_COMMON_TYPES_TO_STRING_H_
#define WOINC_UI_COMMON_TYPES_TO_STRING_H_

#include <woinc/defs.h>

namespace woinc { namespace ui { namespace common {

const char *to_string(const woinc::ActiveTaskState);
const char *to_string(const woinc::MsgInfo);
const char *to_string(const woinc::NetworkStatus);
const char *to_string(const woinc::ResultClientState);
const char *to_string(const woinc::RpcReason);
const char *to_string(const woinc::RunMode);
const char *to_string(const woinc::SchedulerState);
const char *to_string(const woinc::SuspendReason);
const char *exit_code_to_string(int);

}}}

#endif
