/* lib/md5.cc --
   Written and Copyright (C) 2017-2019 by vmc.

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

bool is_little_endian() {
    union { std::int_fast16_t i; char is_little; } endian = { 1 };
    return endian.is_little;
}

// transform between endianess
template<typename T>
constexpr void transform(T &t) {
    char *ptr = reinterpret_cast<char *>(&t);
    std::reverse(ptr, ptr + sizeof(T));
}

constexpr uint32_t F(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) | (~x & z);
}

constexpr uint32_t G(uint32_t x, uint32_t y, uint32_t z) {
    return (x & z) | (y & ~z);
}

constexpr uint32_t H(uint32_t x, uint32_t y, uint32_t z) {
    return x ^ y ^ z;
}

constexpr uint32_t I(uint32_t x, uint32_t y, uint32_t z) {
    return y ^ (x | ~z);
}

constexpr uint32_t ROTATE_LEFT(uint32_t x, size_t n) {
    return (x << n) | (x >> (32-n));
}

template<typename FUNC>
constexpr uint32_t OP(uint32_t a, uint32_t b, uint32_t c, uint32_t d,
                      uint32_t x, uint32_t s, uint32_t ac, FUNC func) {
    return ROTATE_LEFT(a + func(b, c, d) + x + ac, s) + b;
}

enum {
    S11 = 7,
    S12 = 12,
    S13 = 17,
    S14 = 22,
    S21 = 5,
    S22 = 9,
    S23 = 14,
    S24 = 20,
    S31 = 4,
    S32 = 11,
    S33 = 16,
    S34 = 23,
    S41 = 6,
    S42 = 10,
    S43 = 15,
    S44 = 21
};

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
    if (!is_little_endian())
        transform(tmp);
    buffer.insert(buffer.end(), reinterpret_cast<char *>(&tmp), reinterpret_cast<char *>(&tmp + 1));

    // Step 3: init md buffer (see RFC)
    uint32_t A = 0x67452301;
    uint32_t B = 0xEFCDAB89;
    uint32_t C = 0x98BADCFE;
    uint32_t D = 0x10325476;

    // for each block update the digest
    // nearly all of this code in the loop is from the RFC
    for (size_t block = 0; block < pad_bytes; block += 64) {
        std::array<uint32_t, 16> x;
        uint32_t a = A;
        uint32_t b = B;
        uint32_t c = C;
        uint32_t d = D;

        // copy next block into 16 32bit words
        std::copy(&buffer[block], &buffer[block + 64], reinterpret_cast<char*>(x.begin()));

        // if we're big endian we have to convert them
        if (!is_little_endian())
            std::for_each(x.begin(), x.end(), [](uint32_t &v) { transform(v); });

        // Round 1
        a = OP(a, b, c, d, x[ 0], S11, 0xd76aa478, F);
        d = OP(d, a, b, c, x[ 1], S12, 0xe8c7b756, F);
        c = OP(c, d, a, b, x[ 2], S13, 0x242070db, F);
        b = OP(b, c, d, a, x[ 3], S14, 0xc1bdceee, F);
        a = OP(a, b, c, d, x[ 4], S11, 0xf57c0faf, F);
        d = OP(d, a, b, c, x[ 5], S12, 0x4787c62a, F);
        c = OP(c, d, a, b, x[ 6], S13, 0xa8304613, F);
        b = OP(b, c, d, a, x[ 7], S14, 0xfd469501, F);
        a = OP(a, b, c, d, x[ 8], S11, 0x698098d8, F);
        d = OP(d, a, b, c, x[ 9], S12, 0x8b44f7af, F);
        c = OP(c, d, a, b, x[10], S13, 0xffff5bb1, F);
        b = OP(b, c, d, a, x[11], S14, 0x895cd7be, F);
        a = OP(a, b, c, d, x[12], S11, 0x6b901122, F);
        d = OP(d, a, b, c, x[13], S12, 0xfd987193, F);
        c = OP(c, d, a, b, x[14], S13, 0xa679438e, F);
        b = OP(b, c, d, a, x[15], S14, 0x49b40821, F);

        // Round 2
        a = OP(a, b, c, d, x[ 1], S21, 0xf61e2562, G);
        d = OP(d, a, b, c, x[ 6], S22, 0xc040b340, G);
        c = OP(c, d, a, b, x[11], S23, 0x265e5a51, G);
        b = OP(b, c, d, a, x[ 0], S24, 0xe9b6c7aa, G);
        a = OP(a, b, c, d, x[ 5], S21, 0xd62f105d, G);
        d = OP(d, a, b, c, x[10], S22,  0x2441453, G);
        c = OP(c, d, a, b, x[15], S23, 0xd8a1e681, G);
        b = OP(b, c, d, a, x[ 4], S24, 0xe7d3fbc8, G);
        a = OP(a, b, c, d, x[ 9], S21, 0x21e1cde6, G);
        d = OP(d, a, b, c, x[14], S22, 0xc33707d6, G);
        c = OP(c, d, a, b, x[ 3], S23, 0xf4d50d87, G);
        b = OP(b, c, d, a, x[ 8], S24, 0x455a14ed, G);
        a = OP(a, b, c, d, x[13], S21, 0xa9e3e905, G);
        d = OP(d, a, b, c, x[ 2], S22, 0xfcefa3f8, G);
        c = OP(c, d, a, b, x[ 7], S23, 0x676f02d9, G);
        b = OP(b, c, d, a, x[12], S24, 0x8d2a4c8a, G);

        // Round 3
        a = OP(a, b, c, d, x[ 5], S31, 0xfffa3942, H);
        d = OP(d, a, b, c, x[ 8], S32, 0x8771f681, H);
        c = OP(c, d, a, b, x[11], S33, 0x6d9d6122, H);
        b = OP(b, c, d, a, x[14], S34, 0xfde5380c, H);
        a = OP(a, b, c, d, x[ 1], S31, 0xa4beea44, H);
        d = OP(d, a, b, c, x[ 4], S32, 0x4bdecfa9, H);
        c = OP(c, d, a, b, x[ 7], S33, 0xf6bb4b60, H);
        b = OP(b, c, d, a, x[10], S34, 0xbebfbc70, H);
        a = OP(a, b, c, d, x[13], S31, 0x289b7ec6, H);
        d = OP(d, a, b, c, x[ 0], S32, 0xeaa127fa, H);
        c = OP(c, d, a, b, x[ 3], S33, 0xd4ef3085, H);
        b = OP(b, c, d, a, x[ 6], S34,  0x4881d05, H);
        a = OP(a, b, c, d, x[ 9], S31, 0xd9d4d039, H);
        d = OP(d, a, b, c, x[12], S32, 0xe6db99e5, H);
        c = OP(c, d, a, b, x[15], S33, 0x1fa27cf8, H);
        b = OP(b, c, d, a, x[ 2], S34, 0xc4ac5665, H);

        // Round 4
        a = OP(a, b, c, d, x[ 0], S41, 0xf4292244, I);
        d = OP(d, a, b, c, x[ 7], S42, 0x432aff97, I);
        c = OP(c, d, a, b, x[14], S43, 0xab9423a7, I);
        b = OP(b, c, d, a, x[ 5], S44, 0xfc93a039, I);
        a = OP(a, b, c, d, x[12], S41, 0x655b59c3, I);
        d = OP(d, a, b, c, x[ 3], S42, 0x8f0ccc92, I);
        c = OP(c, d, a, b, x[10], S43, 0xffeff47d, I);
        b = OP(b, c, d, a, x[ 1], S44, 0x85845dd1, I);
        a = OP(a, b, c, d, x[ 8], S41, 0x6fa87e4f, I);
        d = OP(d, a, b, c, x[15], S42, 0xfe2ce6e0, I);
        c = OP(c, d, a, b, x[ 6], S43, 0xa3014314, I);
        b = OP(b, c, d, a, x[13], S44, 0x4e0811a1, I);
        a = OP(a, b, c, d, x[ 4], S41, 0xf7537e82, I);
        d = OP(d, a, b, c, x[11], S42, 0xbd3af235, I);
        c = OP(c, d, a, b, x[ 2], S43, 0x2ad7d2bb, I);
        b = OP(b, c, d, a, x[ 9], S44, 0xeb86d391, I);

        A += a;
        B += b;
        C += c;
        D += d;
    }

    // build output as hex string

    static_assert(sizeof(A) == sizeof(B) && sizeof(B) == sizeof(C) && sizeof(C) == sizeof(D),
                  "Types need to be of equal size");

    std::array<char, 4 * sizeof(A)> md_value;

    if (!is_little_endian()){
        transform(A);
        transform(B);
        transform(C);
        transform(D);
    }

    std::copy(reinterpret_cast<char *>(&A), reinterpret_cast<char *>(&A + 1), &md_value[0 * sizeof(A)]);
    std::copy(reinterpret_cast<char *>(&B), reinterpret_cast<char *>(&B + 1), &md_value[1 * sizeof(A)]);
    std::copy(reinterpret_cast<char *>(&C), reinterpret_cast<char *>(&C + 1), &md_value[2 * sizeof(A)]);
    std::copy(reinterpret_cast<char *>(&D), reinterpret_cast<char *>(&D + 1), &md_value[3 * sizeof(A)]);

    std::ostringstream os;

    for (size_t i = 0; i < md_value.size(); i++)
        os << std::setw(2) << std::setfill('0') << std::hex << (md_value[i] & 0xff);

    return os.str();
}

}
