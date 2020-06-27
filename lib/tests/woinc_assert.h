/* tests/woinc_assert.h --
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

#ifndef WOINC_ASSERT_H_
#define WOINC_ASSERT_H_

#include <cstdlib>
#include <sstream>
#include <string>
#include <type_traits>

void assert_contains(const std::string &where, const std::string &what) {
    if (where.find(what) == where.npos)
        throw std::runtime_error("String \"" + what + "\" not found in:\n" + where + "\n");
}

void assert_empty(const std::string &msg, const std::string &what) {
    if (!what.empty())
        throw std::runtime_error(msg + ". got: " + what + "\n");
}

void assert_not_empty(const std::string &msg, const std::string &what) {
    if (what.empty())
        throw std::runtime_error(msg + ". got: " + what + "\n");
}

template<typename T, std::enable_if_t<!std::is_enum<T>::value, int> = 0>
void assert_equals(const std::string &msg, const T &actual, const T &wanted) {
    if (wanted != actual) {
        std::ostringstream os;
        os << "Assertion error: ";
        if (!msg.empty())
            os << msg << ": ";
        os << "wanted: " << wanted << " got: " << actual << std::endl;
        throw std::runtime_error(os.str());
    }
}

template<typename T, std::enable_if_t<std::is_enum<T>::value, int> = 0>
void assert_equals(const std::string &msg, const T &actual, const T &wanted) {
    typedef typename std::underlying_type<T>::type utype;
    assert_equals(msg, static_cast<utype>(actual), static_cast<utype>(wanted));
}

void assert_equals(const std::string &msg, const std::size_t &actual, const int &wanted) {
    assert_equals(msg, actual, static_cast<std::size_t>(wanted));
}

void assert_true(const std::string &msg, const bool b) {
    if (!b) {
        throw std::runtime_error(msg);
    }
}

void assert_false(const std::string &msg, const bool b) {
    assert_true(msg, !b);
}

#endif
