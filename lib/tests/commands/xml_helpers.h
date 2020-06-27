/* tests/commands/xml_helpers.h --
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

#ifndef WOINC_TESTS_COMMANDS_XML_HELPERS_H_
#define WOINC_TESTS_COMMANDS_XML_HELPERS_H_

#include "../woinc_assert.h"

#include "../../src/xml.h"

#ifdef __GNUC__
#define WOINC_MAYBE_UNUSED __attribute__ ((unused))
#else
#define WOINC_MAYBE_UNUSED
#endif

namespace {

WOINC_MAYBE_UNUSED
woinc::xml::Node &need_node(woinc::xml::Node &node, const woinc::xml::Tag &tag) {
    assert_true("Node '" + tag + "' not found", node.has_child(tag));
    return node[tag];
}

WOINC_MAYBE_UNUSED
woinc::xml::Node &need_cmd_node(woinc::xml::Node &node, const woinc::xml::Tag &tag) {
    assert_equals("More than one command node found",
                  node.children.size(),
                  static_cast<decltype(node.children.size())>(1));
    return need_node(node, std::move(tag));
}

template<typename T>
WOINC_MAYBE_UNUSED
void need_node_value(woinc::xml::Node &node, const T &value) {
    assert_equals("Wrong '" + node.tag + "' value", node.content, std::to_string(value));
}

template<typename T>
WOINC_MAYBE_UNUSED
void need_node_with_value(woinc::xml::Node &parent, const woinc::xml::Tag &tag, const T &value) {
    need_node_value(need_node(parent, tag), value);
}

template<>
WOINC_MAYBE_UNUSED
void need_node_value(woinc::xml::Node &node, const std::string &value) {
    assert_equals("Wrong '" + node.tag + "' value", node.content, value);
}

}

#undef WOINC_MAYBE_UNUSED

#endif
