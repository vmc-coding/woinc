/* ui/qt/utils.cc --
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

#include "qt/utils.h"

#include <ctime>
#include <cstdlib>

#include <QString>

namespace woinc { namespace ui { namespace qt {

std::pair<double, const char *> normalization_values(double size) {
    double factor;
    const char *unit;

    if (size >= 1024.*1024.*1024.*1024.) {
        factor = 1024.*1024.*1024.*1024.;
        unit = "TB";
    } else if (size >= 1024.*1024.*1024.) {
        factor = 1024.*1024.*1024.;
        unit = "GB";
    } else if (size >= 1024.*1024.) {
        factor = 1024.*1024.;
        unit = "MB";
    } else if (size >= 1024.) {
        factor = 1024.;
        unit = "KB";
    } else {
        factor = 1.;
        unit = "bytes";
    }

    return std::make_pair(factor, unit);
}

QString size_as_string(double size) {
    auto fu = normalization_values(size);

    if (size > 0) {
        if (fu.first > 1) {
            return QString::asprintf("%0.2f %s", size/fu.first, fu.second);
        } else {
            return QString::asprintf("%d %s", static_cast<int>(size), fu.second);
        }
    } else {
        return QString::fromUtf8("---");
    }
}


QString seconds_as_time_string(long seconds) {
    if (seconds <= 0)
        return QString::fromUtf8("---");
    auto dv = std::ldiv(seconds, 24L*3600L);
    auto days = dv.quot;
    dv = std::ldiv(dv.rem, 3600L);
    auto hours = dv.quot;
    dv = std::ldiv(dv.rem, 60L);
    if (days) {
        return QString::asprintf("%ldd %02ld:%02ld:%02ld", days, hours, dv.quot, dv.rem);
    } else {
        return QString::asprintf("%02ld:%02ld:%02ld", hours, dv.quot, dv.rem);
    }
}

QString time_t_as_string(time_t t) {
    char buf[128];
    auto ret = std::strftime(buf, sizeof(buf), "%c", std::localtime(&t));
    return QString(ret > 0 ? buf : "error");
}

}}}
