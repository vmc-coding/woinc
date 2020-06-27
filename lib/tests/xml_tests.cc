/* tests/xml_tests.cc --
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

#include "test.h"
#include "woinc_assert.h"

#include "../src/xml.h"

static void test_node_empty();
static void test_node_with_tag();
static void test_node_with_content();
static void test_node_with_child();
static void test_node_with_children();
static void test_node_access_operator();
static void test_node_has_child();
static void test_node_content_content();
static void test_node_content_int();
static void test_node_hierarchy();

static void test_tree_shift_operator();
static void test_tree_parse_positive();
static void test_tree_parse_negative1();
static void test_tree_parse_negative2();
static void test_tree_parse_cdata();

static void test_create_boinc_request_tree();

static void test_parse_boinc_response_positive();
static void test_parse_boinc_response_negative();

void get_tests(Tests &tests) {
    tests["001 - Empty node"]                 = test_node_empty;
    tests["002 - Node with tag"]              = test_node_with_tag;
    tests["002 - Node with tag and content"]  = test_node_with_content;
    tests["003 - Node with child"]            = test_node_with_child;
    tests["004 - Node with children"]         = test_node_with_children;
    tests["005 - Node operator[]"]            = test_node_access_operator;
    tests["006 - Node - find child node"]     = test_node_has_child;
    tests["007 - Set node content - Content"] = test_node_content_content;
    tests["008 - Set node content - int"]     = test_node_content_int;
    tests["009 - Node with child hierarchy"]  = test_node_hierarchy;

    tests["100 - Print tree"]                 = test_tree_shift_operator;
    tests["101 - Parse tree - positive"]      = test_tree_parse_positive;
    tests["102 - Parse tree - negative 1"]    = test_tree_parse_negative1;
    tests["103 - Parse tree - negative 2"]    = test_tree_parse_negative2;
    tests["104 - Parse tree - with CDATA"]    = test_tree_parse_cdata;

    tests["200 - Create BOINC request tree"]  = test_create_boinc_request_tree;

    tests["300 - Parse response tree - positive"] = test_parse_boinc_response_positive;
    tests["301 - Parse response tree - negative"] = test_parse_boinc_response_negative;
}

// ----------------------------------------------------------------

// We know there are more than one way to represent the tested XML
// but we just expect this format from our own lib :)

namespace wxml = woinc::xml;

std::string create_result(wxml::Node &node) {
    std::ostringstream out;
    node.print(out);

    return out.str();
}

std::string create_result(wxml::Tree &tree) {
    std::ostringstream out;
    out << tree;

    return out.str();
}

// ------------------- Node tests ---------------------------------

void test_node_empty() {
    wxml::Node node("");
    std::string result(create_result(node));

    std::string wanted("");

    assert_equals("Expected empty response", result, wanted);
}

void test_node_with_tag() {
    wxml::Node node("dummy");
    std::string result(create_result(node));

    std::string wanted("<dummy/>\n");

    assert_equals("Expected single node with tag \"dummy\"", result, wanted);
}

void test_node_with_content() {
    wxml::Node node("dummy");
    node = "some content";
    std::string result(create_result(node));

    std::string wanted("<dummy>some content</dummy>\n");

    assert_equals("Expected node with \"some content\"", result, wanted);
}

void test_node_with_child() {
    wxml::Node parent("parent");
    parent.children.push_back(wxml::Node("child"));

    std::string result(create_result(parent));

    std::string wanted("<parent>\n"\
                       "  <child/>\n"\
                       "</parent>\n");

    assert_equals("Expected node \"parent\" with child \"child\"", result, wanted);
}

void test_node_with_children() {
    wxml::Node parent("parent");
    parent.children.push_back(wxml::Node("child1"));
    parent.children.push_back(wxml::Node("child2"));

    std::string result(create_result(parent));

    std::string wanted("<parent>\n"\
                       "  <child1/>\n"\
                       "  <child2/>\n"\
                       "</parent>\n");

    assert_equals("Expected node \"parent\" with children \"child1\" and \"child2\"", result, wanted);
}

void test_node_access_operator() {
    wxml::Node parent("parent");
    parent["child"];

    std::string result(create_result(parent));

    std::string wanted("<parent>\n"\
                       "  <child/>\n"\
                       "</parent>\n");

    assert_equals("Expected node \"parent\" with child \"child\"", result, wanted);
}

void test_node_has_child() {
    wxml::Node parent("parent");

    assert_false("Node \"child\" found although it doesn't exist", parent.has_child("child"));
    parent["child"];
    assert_true("Node \"child\" not found although we just created it", parent.has_child("child"));
}

void test_node_content_content() {
    wxml::Node node("dummy");
    node = "some content";

    std::string result(create_result(node));

    std::string wanted("<dummy>some content</dummy>\n");

    assert_equals("Expected node with \"some content\"", result, wanted);
}

void test_node_content_int() {
    wxml::Node node("dummy");
    node = 4711;

    std::string result(create_result(node));

    std::string wanted("<dummy>4711</dummy>\n");

    assert_equals("Expected node with content \"4711\"", result, wanted);
}

void test_node_hierarchy() {
    wxml::Node root("root");

    root["foo"]["bar"] = "foobar";
    root["baz"] = "blubb";
    root["foo"]["bar2"];
    root["someint"] = 12;
    root["whitespaces"] = "what ever";

    std::string result(create_result(root));

    std::string wanted("<root>\n"\
                       "  <foo>\n"\
                       "    <bar>foobar</bar>\n"\
                       "    <bar2/>\n"\
                       "  </foo>\n"\
                       "  <baz>blubb</baz>\n"\
                       "  <someint>12</someint>\n"\
                       "  <whitespaces>what ever</whitespaces>\n"\
                       "</root>\n");

    assert_equals("Got wrong result", result, wanted);
}

// ------------------- Tree tests ---------------------------------

void test_tree_shift_operator() {
    wxml::Tree tree("root");

    tree.root["foo"]["bar"] = "foobar";
    tree.root["baz"] = "blubb";
    tree.root["foo"]["bar2"];
    tree.root["someint"] = 12;
    tree.root["whitespaces"] = "what ever";

    std::string result(create_result(tree));

    std::string wanted("<root>\n"\
                       "  <foo>\n"\
                       "    <bar>foobar</bar>\n"\
                       "    <bar2/>\n"\
                       "  </foo>\n"\
                       "  <baz>blubb</baz>\n"\
                       "  <someint>12</someint>\n"\
                       "  <whitespaces>what ever</whitespaces>\n"\
                       "</root>\n");

    assert_equals("Got wrong result", result, wanted);
}

void test_tree_parse_positive() {
    std::string xmlstr("<root>\n"\
                       "  <foo>\n"\
                       "    <bar>foobar</bar>\n"\
                       "    <bar2/>\n"\
                       "  </foo>\n"\
                       "  <baz>blubb</baz>\n"\
                       "  <someint>12</someint>\n"\
                       "  <whitespaces>what ever</whitespaces>\n"\
                       "</root>\n");

    std::istringstream xml_stream(xmlstr);

    wxml::Tree tree;
    std::string error;
    assert_equals("Could not parse the xml", tree.parse(xml_stream, error), true);

    assert_equals("Wrong xml result", tree.root.tag, std::string("root"));

    assert_equals("Wrong xml result", tree.root.has_child("foo"), true);

    assert_equals("Wrong xml result", tree.root["foo"].has_child("bar"), true);
    assert_equals("Wrong xml result", tree.root["foo"]["bar"].content, std::string("foobar"));

    assert_equals("Wrong xml result", tree.root["foo"].has_child("bar2"), true);
    assert_equals("Wrong xml result", tree.root["foo"]["bar2"].content, std::string());

    assert_equals("Wrong xml result", tree.root.has_child("baz"), true);
    assert_equals("Wrong xml result", tree.root["baz"].content, std::string("blubb"));

    assert_equals("Wrong xml result", tree.root.has_child("someint"), true);
    assert_equals("Wrong xml result", tree.root["someint"].content, std::string("12"));

    assert_equals("Wrong xml result", tree.root.has_child("whitespaces"), true);
    assert_equals("Wrong xml result", tree.root["whitespaces"].content, std::string("what ever"));
}

void test_tree_parse_negative1() {
    std::string xmlstr("<root/><root/>");

    std::istringstream xml_stream(xmlstr);

    wxml::Tree tree;
    std::string error;
    assert_equals("Broken xml parsed", tree.parse(xml_stream, error), false);
}

void test_tree_parse_negative2() {
    std::string xmlstr("<root>");

    std::istringstream xml_stream(xmlstr);

    wxml::Tree tree;
    std::string error;
    assert_equals("Broken xml parsed", tree.parse(xml_stream, error), false);
    assert_not_empty("Broken xml parsed", error);
}

void test_tree_parse_cdata() {
    std::string xmlstr("<root><![CDATA[ Foobar ]]></root>");

    std::istringstream xml_stream(xmlstr);

    wxml::Tree tree;
    std::string error;
    assert_equals("Could not parse the xml", tree.parse(xml_stream, error), true);

    assert_equals("Wrong xml result", tree.root.content, std::string(" Foobar "));
}

// ----------------- create_boinc_request_tree() ------------------

void test_create_boinc_request_tree() {
    wxml::Tree tree(wxml::create_boinc_request_tree());

    std::string result(create_result(tree));

    std::string wanted("<boinc_gui_rpc_request/>\n");

    assert_equals("Wrong result", result, wanted);
}

// --------------- parse_boinc_response_positive() ----------------

void test_parse_boinc_response_positive() {
    std::string xmlstr("<boinc_gui_rpc_reply/>\n");
    std::istringstream xml_stream(xmlstr);

    wxml::Tree tree;
    std::string error;
    assert_equals("Could not parse the xml", wxml::parse_boinc_response(tree, xml_stream, error), true);
    assert_equals("Wrong root tag", tree.root.tag, std::string("boinc_gui_rpc_reply"));
}

void test_parse_boinc_response_negative() {
    std::string xmlstr("<not_boinc_gui_rpc_reply/>\n");
    std::istringstream xml_stream(xmlstr);

    wxml::Tree tree;
    std::string error;
    assert_equals("Parsed invalid response", wxml::parse_boinc_response(tree, xml_stream, error), false);
}
