/* ui/common/types_to_string.h --
   Written and Copyright (C) 2017-2020 by vmc.

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

const char *to_string(const woinc::ACTIVE_TASK_STATE);
const char *to_string(const woinc::MSG_INFO);
const char *to_string(const woinc::NETWORK_STATUS);
const char *to_string(const woinc::RESULT_CLIENT_STATE);
const char *to_string(const woinc::RPC_REASON);
const char *to_string(const woinc::RUN_MODE);
const char *to_string(const woinc::SCHEDULER_STATE);
const char *to_string(const woinc::SUSPEND_REASON);
const char *exit_code_to_string(int);

}}}

#endif
