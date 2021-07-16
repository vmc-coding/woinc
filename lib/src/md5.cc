/* lib/md5.cc --
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

/* Implemented according to the RFC 1321 from April 1992.

   We use strings as input and output here (not blocks) as we only
   have to hash nonce+password when authorizing against client. */

#include "md5.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <sstream>
#include <vector>
#include <iomanip>

using std::uint32_t;
using std::size_t;

namespace {

bool is_little_endian__() {
    union { std::int_fast16_t i; char is_little; } endian = { 1 };
    return endian.is_little;
}

// transform between endianess
template<typename T>
constexpr void transform__(T &t) noexcept {
    char *ptr = reinterpret_cast<char *>(&t);
    std::reverse(ptr, ptr + sizeof(T));
}

constexpr uint32_t F__(uint32_t x, uint32_t y, uint32_t z) noexcept {
    return (x & y) | (~x & z);
}

constexpr uint32_t G__(uint32_t x, uint32_t y, uint32_t z) noexcept {
    return (x & z) | (y & ~z);
}

constexpr uint32_t H__(uint32_t x, uint32_t y, uint32_t z) noexcept {
    return x ^ y ^ z;
}

constexpr uint32_t I__(uint32_t x, uint32_t y, uint32_t z) noexcept {
    return y ^ (x | ~z);
}

constexpr uint32_t ROTATE_LEFT__(uint32_t x, size_t n) noexcept {
    return (x << n) | (x >> (32-n));
}

template<typename Func>
constexpr uint32_t OP__(uint32_t a, uint32_t b, uint32_t c, uint32_t d,
                        uint32_t x, uint32_t s, uint32_t ac, Func func) noexcept {
    return ROTATE_LEFT__(a + func(b, c, d) + x + ac, s) + b;
}

constexpr uint32_t S11__ = 7;
constexpr uint32_t S12__ = 12;
constexpr uint32_t S13__ = 17;
constexpr uint32_t S14__ = 22;
constexpr uint32_t S21__ = 5;
constexpr uint32_t S22__ = 9;
constexpr uint32_t S23__ = 14;
constexpr uint32_t S24__ = 20;
constexpr uint32_t S31__ = 4;
constexpr uint32_t S32__ = 11;
constexpr uint32_t S33__ = 16;
constexpr uint32_t S34__ = 23;
constexpr uint32_t S41__ = 6;
constexpr uint32_t S42__ = 10;
constexpr uint32_t S43__ = 15;
constexpr uint32_t S44__ = 21;

}

namespace woinc {

std::string md5(const std::string &in_str) {
    size_t bytes = in_str.size();

    // +1 for the first padding 0x80 byte
    // +8 for the number of bits of the message as 64bit int
    size_t pad_bytes = bytes + 9;

    // pad to pad_bytes % 64 := 0
    if (pad_bytes % 64 != 0)
        pad_bytes += 64 - pad_bytes % 64;

    assert(pad_bytes % 64 == 0);
    assert(pad_bytes >= bytes + 9);

    // Step 0: our working buffer, filled with the input string
    std::vector<char> buffer;
    buffer.reserve(pad_bytes);
    std::copy(in_str.begin(), in_str.end(), std::back_inserter(buffer));

    // Step 1: padding (add a '1' bit and fill with '0' bits)
    buffer.push_back(static_cast<char>(0x80));
    std::fill_n(std::back_inserter(buffer), pad_bytes - 8 - (bytes + 1), 0x00);

    // Step 2: appending length in bits of input msg as 64 bit little endian
    std::uint64_t tmp = bytes * 8;
    if (!is_little_endian__())
        transform__(tmp);
    buffer.insert(buffer.end(), reinterpret_cast<char *>(&tmp), reinterpret_cast<char *>(&tmp + 1));

    // Step 3: init md buffer (see RFC)
    uint32_t buf_a = 0x67452301;
    uint32_t buf_b = 0xEFCDAB89;
    uint32_t buf_c = 0x98BADCFE;
    uint32_t buf_d = 0x10325476;

    // for each block update the digest
    // nearly all of this code in the loop is from the RFC
    for (size_t block = 0; block < pad_bytes; block += 64) {
        std::array<uint32_t, 16> x;
        uint32_t a = buf_a;
        uint32_t b = buf_b;
        uint32_t c = buf_c;
        uint32_t d = buf_d;

        // copy next block into 16 32bit words
        std::copy(&buffer[block], &buffer[block + 64], reinterpret_cast<char*>(x.begin()));

        // if we're big endian we have to convert them
        if (!is_little_endian__())
            std::for_each(x.begin(), x.end(), [](uint32_t &v) { transform__(v); });

        // Round 1
        a = OP__(a, b, c, d, x[ 0], S11__, 0xd76aa478, F__);
        d = OP__(d, a, b, c, x[ 1], S12__, 0xe8c7b756, F__);
        c = OP__(c, d, a, b, x[ 2], S13__, 0x242070db, F__);
        b = OP__(b, c, d, a, x[ 3], S14__, 0xc1bdceee, F__);
        a = OP__(a, b, c, d, x[ 4], S11__, 0xf57c0faf, F__);
        d = OP__(d, a, b, c, x[ 5], S12__, 0x4787c62a, F__);
        c = OP__(c, d, a, b, x[ 6], S13__, 0xa8304613, F__);
        b = OP__(b, c, d, a, x[ 7], S14__, 0xfd469501, F__);
        a = OP__(a, b, c, d, x[ 8], S11__, 0x698098d8, F__);
        d = OP__(d, a, b, c, x[ 9], S12__, 0x8b44f7af, F__);
        c = OP__(c, d, a, b, x[10], S13__, 0xffff5bb1, F__);
        b = OP__(b, c, d, a, x[11], S14__, 0x895cd7be, F__);
        a = OP__(a, b, c, d, x[12], S11__, 0x6b901122, F__);
        d = OP__(d, a, b, c, x[13], S12__, 0xfd987193, F__);
        c = OP__(c, d, a, b, x[14], S13__, 0xa679438e, F__);
        b = OP__(b, c, d, a, x[15], S14__, 0x49b40821, F__);

        // Round 2
        a = OP__(a, b, c, d, x[ 1], S21__, 0xf61e2562, G__);
        d = OP__(d, a, b, c, x[ 6], S22__, 0xc040b340, G__);
        c = OP__(c, d, a, b, x[11], S23__, 0x265e5a51, G__);
        b = OP__(b, c, d, a, x[ 0], S24__, 0xe9b6c7aa, G__);
        a = OP__(a, b, c, d, x[ 5], S21__, 0xd62f105d, G__);
        d = OP__(d, a, b, c, x[10], S22__,  0x2441453, G__);
        c = OP__(c, d, a, b, x[15], S23__, 0xd8a1e681, G__);
        b = OP__(b, c, d, a, x[ 4], S24__, 0xe7d3fbc8, G__);
        a = OP__(a, b, c, d, x[ 9], S21__, 0x21e1cde6, G__);
        d = OP__(d, a, b, c, x[14], S22__, 0xc33707d6, G__);
        c = OP__(c, d, a, b, x[ 3], S23__, 0xf4d50d87, G__);
        b = OP__(b, c, d, a, x[ 8], S24__, 0x455a14ed, G__);
        a = OP__(a, b, c, d, x[13], S21__, 0xa9e3e905, G__);
        d = OP__(d, a, b, c, x[ 2], S22__, 0xfcefa3f8, G__);
        c = OP__(c, d, a, b, x[ 7], S23__, 0x676f02d9, G__);
        b = OP__(b, c, d, a, x[12], S24__, 0x8d2a4c8a, G__);

        // Round 3
        a = OP__(a, b, c, d, x[ 5], S31__, 0xfffa3942, H__);
        d = OP__(d, a, b, c, x[ 8], S32__, 0x8771f681, H__);
        c = OP__(c, d, a, b, x[11], S33__, 0x6d9d6122, H__);
        b = OP__(b, c, d, a, x[14], S34__, 0xfde5380c, H__);
        a = OP__(a, b, c, d, x[ 1], S31__, 0xa4beea44, H__);
        d = OP__(d, a, b, c, x[ 4], S32__, 0x4bdecfa9, H__);
        c = OP__(c, d, a, b, x[ 7], S33__, 0xf6bb4b60, H__);
        b = OP__(b, c, d, a, x[10], S34__, 0xbebfbc70, H__);
        a = OP__(a, b, c, d, x[13], S31__, 0x289b7ec6, H__);
        d = OP__(d, a, b, c, x[ 0], S32__, 0xeaa127fa, H__);
        c = OP__(c, d, a, b, x[ 3], S33__, 0xd4ef3085, H__);
        b = OP__(b, c, d, a, x[ 6], S34__,  0x4881d05, H__);
        a = OP__(a, b, c, d, x[ 9], S31__, 0xd9d4d039, H__);
        d = OP__(d, a, b, c, x[12], S32__, 0xe6db99e5, H__);
        c = OP__(c, d, a, b, x[15], S33__, 0x1fa27cf8, H__);
        b = OP__(b, c, d, a, x[ 2], S34__, 0xc4ac5665, H__);

        // Round 4
        a = OP__(a, b, c, d, x[ 0], S41__, 0xf4292244, I__);
        d = OP__(d, a, b, c, x[ 7], S42__, 0x432aff97, I__);
        c = OP__(c, d, a, b, x[14], S43__, 0xab9423a7, I__);
        b = OP__(b, c, d, a, x[ 5], S44__, 0xfc93a039, I__);
        a = OP__(a, b, c, d, x[12], S41__, 0x655b59c3, I__);
        d = OP__(d, a, b, c, x[ 3], S42__, 0x8f0ccc92, I__);
        c = OP__(c, d, a, b, x[10], S43__, 0xffeff47d, I__);
        b = OP__(b, c, d, a, x[ 1], S44__, 0x85845dd1, I__);
        a = OP__(a, b, c, d, x[ 8], S41__, 0x6fa87e4f, I__);
        d = OP__(d, a, b, c, x[15], S42__, 0xfe2ce6e0, I__);
        c = OP__(c, d, a, b, x[ 6], S43__, 0xa3014314, I__);
        b = OP__(b, c, d, a, x[13], S44__, 0x4e0811a1, I__);
        a = OP__(a, b, c, d, x[ 4], S41__, 0xf7537e82, I__);
        d = OP__(d, a, b, c, x[11], S42__, 0xbd3af235, I__);
        c = OP__(c, d, a, b, x[ 2], S43__, 0x2ad7d2bb, I__);
        b = OP__(b, c, d, a, x[ 9], S44__, 0xeb86d391, I__);

        buf_a += a;
        buf_b += b;
        buf_c += c;
        buf_d += d;
    }

    // build output as hex string

    static_assert(sizeof(buf_a) == sizeof(buf_b) && sizeof(buf_b) == sizeof(buf_c) && sizeof(buf_c) == sizeof(buf_d),
                  "Types need to be of equal size");

    std::array<char, 4 * sizeof(buf_a)> md_value;

    if (!is_little_endian__()) {
        transform__(buf_a);
        transform__(buf_b);
        transform__(buf_c);
        transform__(buf_d);
    }

    std::copy(reinterpret_cast<char *>(&buf_a), reinterpret_cast<char *>(&buf_a + 1), &md_value[0 * sizeof(buf_a)]);
    std::copy(reinterpret_cast<char *>(&buf_b), reinterpret_cast<char *>(&buf_b + 1), &md_value[1 * sizeof(buf_a)]);
    std::copy(reinterpret_cast<char *>(&buf_c), reinterpret_cast<char *>(&buf_c + 1), &md_value[2 * sizeof(buf_a)]);
    std::copy(reinterpret_cast<char *>(&buf_d), reinterpret_cast<char *>(&buf_d + 1), &md_value[3 * sizeof(buf_a)]);

    std::ostringstream os;

    for (size_t i = 0; i < md_value.size(); ++i)
        os << std::setw(2) << std::setfill('0') << std::hex << (md_value[i] & 0xff);

    return os.str();
}

}
