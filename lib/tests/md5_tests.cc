/* tests/md5_tests.cc --
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

#include "test.h"
#include "woinc_assert.h"

#include "../src/md5.h"

// Tests are from RFC 1321

static void test01();
static void test02();
static void test03();
static void test04();
static void test05();
static void test06();
static void test07();

void get_tests(Tests &tests) {
    tests["Test01"] = test01;
    tests["Test02"] = test02;
    tests["Test03"] = test03;
    tests["Test04"] = test04;
    tests["Test05"] = test05;
    tests["Test06"] = test06;
    tests["Test07"] = test07;
}

void test01() {
    assert_equals("", woinc::md5(""), std::string("d41d8cd98f00b204e9800998ecf8427e"));
}

void test02() {
    assert_equals("", woinc::md5("a"), std::string("0cc175b9c0f1b6a831c399e269772661"));
}

void test03() {
    assert_equals("", woinc::md5("abc"), std::string("900150983cd24fb0d6963f7d28e17f72"));
}

void test04() {
    assert_equals("", woinc::md5("message digest"),
                  std::string("f96b697d7cb7938d525a2f31aaf161d0"));
}

void test05() {
    assert_equals("", woinc::md5("abcdefghijklmnopqrstuvwxyz"),
                  std::string("c3fcd3d76192e4007dfb496cca67e13b"));
}

void test06() {
    assert_equals("", woinc::md5("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"),
                  std::string("d174ab98d277d9f5a5611c2c9f419d9f"));
}

void test07() {
    assert_equals("", woinc::md5("1234567890123456789012345678901234567890123456789012345678901" \
                                 "2345678901234567890"),
                  std::string("57edf4a22be3c955ac49da2e2107b67a"));
}
