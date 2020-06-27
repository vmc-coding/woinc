/* lib/visibility.h --
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

#ifndef WOINC_VISIBILITY_H_
#define WOINC_VISIBILITY_H_

// For reference see https://gcc.gnu.org/wiki/Visibility

#if defined _WIN32 || defined __CYGWIN__
  #ifdef BUILDING_DLL
    #ifdef __GNUC__
      #define WOINC_API __attribute__ ((dllexport))
    #else
      #define WOINC_API __declspec(dllexport)
    #endif
  #else
    #ifdef __GNUC__
      #define WOINC_API __attribute__ ((dllimport))
    #else
      #define WOINC_API __declspec(dllimport)
    #endif
  #endif
  #define WOINC_LOCAL
#else
  #if __GNUC__ >= 4
    #define WOINC_API __attribute__ ((visibility ("default")))
    #define WOINC_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define WOINC_API
    #define WOINC_LOCAL
  #endif
#endif

#endif
