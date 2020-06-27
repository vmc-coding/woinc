/* lib/xml.h --
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

#ifndef WOINC_XML_H_
#define WOINC_XML_H_

#include <iosfwd>
#include <list>
#include <string>
#include <utility>

#include "visibility.h"

// Very simple XML-wrapper which only supports the stuff we need.

namespace woinc { namespace xml WOINC_LOCAL {
    struct Node;

    typedef std::string Tag;
    typedef std::string Content;
    typedef std::list<Node> Nodes;

    struct Node {
        Tag tag;
        Content content;
        Nodes children;
        // when setting prefs BOINC writes the payload of the RPC request into the prefs file
        // without interpreting it (e.g. by parsing and reformating) which would be indented.
        // To prevent this, set the following flag to reset the indention level when generting the xml output.
        bool reset_indention_level = false;

        explicit Node(Tag t) : tag(std::move(t)) {}

        Node(Node &&) = default;
        Node &operator=(Node &&) = default;

        /*
         * Syntactic sugar to access a child node by a tag.
         * If it doesn't exist a node with this tag yet, it will be created.
         * Obviously this only accesses the first found node and should only be used
         * if the caller 'knows' that there is only one child with this tag.
         */
        Node &operator[](Tag tag);

        Node &add_child(Tag tag);

        bool has_child(const Tag &t) const {
            return found_child(find_child(t));
        }

        Nodes::const_iterator find_child(const Tag &tag) const;
        Nodes::const_iterator find_child(const Nodes::const_iterator &start, const Tag &tag) const;

        bool found_child(const Nodes::const_iterator &i) const {
            return i != children.end();
        }

        /*
         * Syntactic sugar to set the content of a node.
         */

        void operator=(Content c) {
            content = std::move(c);
        }

        void operator=(int i) {
            content = std::to_string(i);
        }

        void operator=(double d) {
            content = std::to_string(d);
        }

        std::ostream &print(std::ostream &out, size_t indention_level = 0) const;
    };

    struct Tree {
        Node root;

        explicit Tree(Tag tag = Tag()) : root(std::move(tag)) {}

        Tree(Tree &&) = default;
        Tree &operator=(Tree &&) = default;

        std::ostream &print(std::ostream &out) const {
            return root.print(out);
        }

        bool parse(std::istream &in, std::string &error_holder);

        std::string str() const;
    };

    std::ostream &operator<<(std::ostream &out, const Tree &tree);

    Tree create_boinc_request_tree();
    bool parse_boinc_response(Tree &tree, std::istream &in, std::string &error_holder);

}}

#endif
