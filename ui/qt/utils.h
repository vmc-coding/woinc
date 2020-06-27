/* ui/qt/utils.h --
   Written and Copyright (C) 2018-2020 by vmc.

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

#include <utility>

#include <QString>

namespace woinc { namespace ui { namespace qt {

// factor, unit
std::pair<double, const char *> normalization_values(double size);

QString size_as_string(double size);

QString seconds_as_time_string(long seconds);

QString time_t_as_string(time_t t);

}}}
