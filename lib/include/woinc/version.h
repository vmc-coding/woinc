/* woinc/version.h --
   Written and Copyright (C) 2017 by vmc.

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

#ifndef WOINC_VERSION_H_
#define WOINC_VERSION_H_

namespace woinc {

//! \brief The major version of the woinc lib
int major_version();
//! \brief The minor version of the woinc lib
int minor_version();

// Used when exchanging the versions with the client.
// Typically these are the versions of boinc I'm using on my dev-box, the client ignores it anyway ..

int boinc_major_version();
int boinc_minor_version();
int boinc_release_version();

}

#endif
