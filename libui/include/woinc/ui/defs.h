/* woinc/ui/defs.h --
   Written and Copyright (C) 2018-2021 by vmc.

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

#ifndef WOINC_UI_DEFS_H_
#define WOINC_UI_DEFS_H_

namespace woinc { namespace ui {

enum class Error {
    Disconnected,
    Unauthorized,
    ConnectionError,
    ClientError,
    ParsingError,
    LogicError
};

enum class PeriodicTask {
    GetCCStatus,
    GetClientState,
    GetDiskUsage,
    GetFileTransfers,
    GetMessages,
    GetNotices,
    GetProjectStatus,
    GetStatistics,
    GetTasks
};

}}

#endif
