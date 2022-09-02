/* lib/xml.cc --
   Written and Copyright (C) 2017-2022 by vmc.

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

#include "xml.h"

#include <algorithm>
#include <cassert>
#include <sstream>

// We use the contrib XML-Lib only for parsing the response, not for creating the request.
// Doing it this way we will have less to do when porting to other XML-Libs.
#include <pugixml.hpp>

namespace {

const std::string REQUEST_TAG__("boinc_gui_rpc_request");
const std::string RESPONSE_TAG__("boinc_gui_rpc_reply");

}

namespace woinc { namespace xml {

// --- Node impl

Node &Node::operator[](const Tag &t) {
    assert(!this->tag.empty());

    auto n = std::find_if(children.begin(), children.end(), [&](const Node &node) {
        return node.tag == t;
    });

    if (n == children.end()) {
        return add_child(t);
    } else {
        return *n;
    }
}

Node &Node::add_child(Tag t) {
    children.push_back(Node(std::move(t)));
    return children.back();
}

void Node::remove_childs(const Tag &t) {
    children.remove_if([&](const auto &node) { return node.tag == t; });
}

Nodes::const_iterator Node::find_child(const Tag &t) const {
    return find_child(children.begin(), t);
}

Nodes::const_iterator Node::find_child(const Nodes::const_iterator &begin, const Tag &t) const {
    return std::find_if(begin, children.cend(), [&](const Node &node){
        return node.tag == t;
    });
}

std::ostream &Node::print(std::ostream &out, size_t indention_level) const {
    if (tag.empty())
        return out;

    if (reset_indention_level)
        indention_level = 0;

    std::string indent(2 * indention_level, ' ');

    if (!children.empty()) {
        out << indent  << "<" << tag << ">" << content << "\n";
        for (const auto &child : children)
            child.print(out, indention_level + 1);
        out << indent << "</" << tag << ">\n";
    } else if (content.empty()) {
        // From https://boinc.berkeley.edu/trac/wiki/GuiRpcProtocol (Feb 17)
        // "Self-closing tags must not have a space before the slash,
        // or current client and server will not parse it correctly."
        out << indent << "<" << tag << "/>\n";
    } else {
        out << indent << "<" << tag << ">"
            << content
            << "</" << tag << ">\n";
    }
    return out;
}

// --- Tree impl

namespace {

void parse_node(const pugi::xml_node &pugi_node, Node &woinc_node) {
    for (const auto &pugi_child : pugi_node.children()) {
        if (pugi_child.type() == pugi::node_element) {
            Node next_woinc_node(pugi_child.name());
            parse_node(pugi_child, next_woinc_node);
            woinc_node.children.push_back(std::move(next_woinc_node));
        } else if (pugi_child.type() == pugi::node_pcdata) {
            woinc_node.content = pugi_child.value();
        } else if (pugi_child.type() == pugi::node_cdata) {
            woinc_node.content = pugi_child.value();
        }
    }
}

} // unnamed namespace


std::ostream &operator<<(std::ostream &out, const Node &node) {
    return node.print(out);
}

bool Tree::parse(std::istream &in, std::string &error_holder) {
    pugi::xml_document tree;

    auto parsing_status = tree.load(in);

    if (!parsing_status || !tree) {
        error_holder = parsing_status.description();
        return false;
    }

    bool found_root_element = false;

    for (const auto &child : tree.children()) {
        if (child.type() != pugi::node_element)
            continue;
        // broken xml with more than one root element
        if (found_root_element)
            return false;
        found_root_element = true;
        root.tag = child.name();
        parse_node(child, root);
    }

    return true;
}

std::string Tree::str() const {
    std::stringstream s;
    s << *this;
    return s.str();
}

std::ostream &operator<<(std::ostream &out, const Tree &tree) {
    return tree.print(out);
}

Tree create_boinc_request_tree() {
    return Tree(REQUEST_TAG__);
}

bool parse_boinc_response(Tree &tree, std::istream &in, std::string &error_holder) {
    return tree.parse(in, error_holder) && RESPONSE_TAG__ == tree.root.tag;
}

}}
