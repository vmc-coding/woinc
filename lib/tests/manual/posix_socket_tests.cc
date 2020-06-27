/* tests/manual/posix_socket_tests.cc --
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

#include "../test.h"
#include "../woinc_assert.h"

extern "C" {
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
} // extern "C"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <memory>

#include "../../src/socket.h"

static const char *HOST_V4 = "localhost";
static const char *HOST_V6 = "ip6-localhost";
static std::uint16_t PORT = 45620; // maybe we should provide the port by command line parameter
static std::size_t BUFFER_SIZE = 32*1024;

static void test_ipv4();
static void test_ipv6();
static void test_all_ipv4();
static void test_all_ipv6();

static void test_resolving_error();
static void test_connecting_error();
static void test_already_connected();
static void test_client_disconnected_before_request();
static void test_client_disconnected_before_response();

static void test_send_not_connected();
static void test_receive_not_connected();

void get_tests(Tests &tests) {
    tests["01 - Test IPv4"] = test_ipv4;
    tests["02 - Test IPv6"] = test_ipv6;
    tests["03 - Test All - IPv4"] = test_all_ipv4;
    tests["04 - Test All - IPv6"] = test_all_ipv6;

    tests["10 - Resolving error"] = test_resolving_error;
    tests["11 - Connecting error"] = test_connecting_error;
    tests["12 - Already connected"] = test_already_connected;
    tests["13 - Client disconnected before request"] = test_client_disconnected_before_request;
    tests["14 - Client disconnected before response"] = test_client_disconnected_before_response;

    tests["20 - Send not connected"] = test_send_not_connected;
    tests["21 - Receive not connected"] = test_receive_not_connected;
}

// helpers to create a listening socket which we could connect to in the tests

static int create_listening_socket(woinc::Socket::VERSION version, std::uint16_t port);
static void mock_client_response(int socket, const std::string &response);
static bool close_socket(int socket);

// the tests

void test_socket(woinc::Socket &socket, const char *host, int listening_socket) {
    assert_true("Could not connect to host", socket.connect(host, PORT));

    std::string request("whatever");
    assert_true("Could not send request to the client", socket.send(request.c_str(), request.size()));

    mock_client_response(listening_socket, "response 123");

    char buffer[BUFFER_SIZE];
    std::size_t read;
    assert_true("Did not receive response from the client", socket.receive(buffer, BUFFER_SIZE, read));
    std::string response(buffer, read);

    assert_true("Error while closing the listening socket", close_socket(listening_socket));

    socket.close();

    assert_equals("Ouch, got wrong response ..", response, std::string("response 123"));
}

void test_ipv4() {
    int listening_socket = create_listening_socket(woinc::Socket::VERSION::IPv4, PORT);

    std::unique_ptr<woinc::Socket> socket(woinc::Socket::create(woinc::Socket::VERSION::IPv4));
    test_socket(*socket, HOST_V4, listening_socket);
}

void test_ipv6() {
    int listening_socket = create_listening_socket(woinc::Socket::VERSION::IPv6, PORT);

    std::unique_ptr<woinc::Socket> socket(woinc::Socket::create(woinc::Socket::VERSION::IPv6));
    test_socket(*socket, HOST_V6, listening_socket);
}

void test_all_ipv4() {
    int listening_socket = create_listening_socket(woinc::Socket::VERSION::IPv4, PORT);

    std::unique_ptr<woinc::Socket> socket(woinc::Socket::create(woinc::Socket::VERSION::ALL));
    test_socket(*socket, HOST_V4, listening_socket);
}

void test_all_ipv6() {
    int listening_socket = create_listening_socket(woinc::Socket::VERSION::IPv6, PORT);

    std::unique_ptr<woinc::Socket> socket(woinc::Socket::create(woinc::Socket::VERSION::ALL));
    test_socket(*socket, HOST_V6, listening_socket);
}

void test_resolving_error() {
    std::unique_ptr<woinc::Socket> socket(woinc::Socket::create(woinc::Socket::VERSION::IPv4));
    auto result = socket->connect("345.231.423.12", PORT);
    assert_false("Ouch, could connect to some invalid host", result);
    assert_equals("Wrong status", result.status, woinc::Socket::STATUS::RESOLVING_ERROR);
    assert_false("Diagnostic message not set", result.error.empty());
}

void test_connecting_error() {
    std::unique_ptr<woinc::Socket> socket(woinc::Socket::create(woinc::Socket::VERSION::ALL));
    auto result = socket->connect(HOST_V4, PORT + 1);
    assert_false("Ouch, could connect to some invalid port", result);
    assert_equals("Wrong status", result.status, woinc::Socket::STATUS::SOCKET_ERROR);
    assert_false("Diagnostic message not set", result.error.empty());
}

void test_already_connected() {
    int listening_socket = create_listening_socket(woinc::Socket::VERSION::IPv4, PORT);

    std::unique_ptr<woinc::Socket> socket(woinc::Socket::create(woinc::Socket::VERSION::ALL));

    assert_true("Ouch, could not connect", socket->connect(HOST_V4, PORT));

    auto result = socket->connect(HOST_V4, PORT);
    close_socket(listening_socket);

    assert_false("Expected error because we're already connected", result);
    assert_equals("Wrong status", result.status, woinc::Socket::STATUS::ALREADY_CONNECTED);
}

void test_client_disconnected_before_request() {
    int listening_socket = create_listening_socket(woinc::Socket::VERSION::IPv4, PORT);

    std::unique_ptr<woinc::Socket> socket(woinc::Socket::create(woinc::Socket::VERSION::ALL));

    assert_true("Ouch, could not connect", socket->connect(HOST_V4, PORT));

    close_socket(listening_socket);

    std::string request("whatever");

    auto result = socket->send(request.c_str(), request.size());
    assert_false("Ouch, could send data to a closed socket", result);
    assert_equals("Wrong status", result.status, woinc::Socket::STATUS::SOCKET_ERROR);
}

void test_client_disconnected_before_response() {
    int listening_socket = create_listening_socket(woinc::Socket::VERSION::IPv4, PORT);

    std::unique_ptr<woinc::Socket> socket(woinc::Socket::create(woinc::Socket::VERSION::ALL));

    assert_true("Could not connect", socket->connect(HOST_V4, PORT));

    std::string request("whatever");
    assert_true("Could not send data to the socket", socket->send(request.c_str(), request.size()));

    close_socket(listening_socket);

    char buffer[BUFFER_SIZE];
    std::size_t read;

    auto result = socket->receive(buffer, BUFFER_SIZE, read);
    assert_false("Ouch, did receive response from a closed client", result);
    assert_equals("Wrong status", result.status, woinc::Socket::STATUS::SOCKET_ERROR);
}

void test_send_not_connected() {
    std::unique_ptr<woinc::Socket> socket(woinc::Socket::create(woinc::Socket::VERSION::ALL));

    std::stringstream request;
    request << "<boinc_gui_rpc_request><exchange_versions></boinc_gui_rpc_request>\003";

    auto result = socket->send(request.str().c_str(), request.str().size());
    assert_false("Could send although not connected", result);
    assert_equals("Wrong status", result.status, woinc::Socket::STATUS::NOT_CONNECTED);
}

void test_receive_not_connected() {
    std::unique_ptr<woinc::Socket> socket(woinc::Socket::create(woinc::Socket::VERSION::ALL));

    char buffer[BUFFER_SIZE];
    std::size_t read;

    auto result = socket->receive(buffer, BUFFER_SIZE, read);
    assert_false("Could receive although not connected", result);
    assert_equals("Wrong status", result.status, woinc::Socket::STATUS::NOT_CONNECTED);
}

// --------- helpers ----------

int create_listening_socket(woinc::Socket::VERSION version, std::uint16_t port) {
    assert(version == woinc::Socket::VERSION::IPv4 || version == woinc::Socket::VERSION::IPv6);

    int socket = ::socket(version == woinc::Socket::VERSION::IPv4 ? AF_INET : AF_INET6, SOCK_STREAM, 0);

    if (socket == -1) {
        std::cerr << "Could not create listening socket: " << strerror(errno) << "\n";
        exit(EXIT_FAILURE);
    }

    int one = 1;
    if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int)) == -1) {
        std::cerr << "Could not set options on the listening socket: " << strerror(errno) << "\n";
        exit(EXIT_FAILURE);
    }

    if (version == woinc::Socket::VERSION::IPv4) {
        sockaddr_in address4;

        std::memset(&address4, 0, sizeof(address4));
        address4.sin_family = AF_INET;
        address4.sin_addr.s_addr = INADDR_ANY;
        address4.sin_port = htons(port);

        if (::bind(socket, reinterpret_cast<sockaddr *>(&address4), sizeof(address4)) == -1) {
            std::cerr << "Could not bind the listening socket: " << strerror(errno) << "\n";
            exit(EXIT_FAILURE);
        }
    } else {
        sockaddr_in6 address6;

        std::memset(&address6, 0, sizeof(address6));
        address6.sin6_family = AF_INET6;
        address6.sin6_addr = IN6ADDR_LOOPBACK_INIT;
        address6.sin6_port = htons(port);

        if (::bind(socket, reinterpret_cast<sockaddr *>(&address6), sizeof(address6)) == -1) {
            std::cerr << "Could not bind the listening socket: " << strerror(errno) << "\n";
            exit(EXIT_FAILURE);
        }
    }

    if (::listen(socket, 1) == -1) {
        std::cerr << "Could not listen on the socket: " << strerror(errno) << "\n";
        exit(EXIT_FAILURE);
    }

    return socket;
}

void mock_client_response(int socket, const std::string &response) {
    char buffer[BUFFER_SIZE];

    int client_socket = ::accept(socket, nullptr, nullptr);

    if (client_socket == -1) {
        std::cerr << "Could not accept on the socket: " << strerror(errno) << "\n";
        exit(EXIT_FAILURE);
    }

    if (::recv(client_socket, buffer, BUFFER_SIZE, 0) == -1) {
        std::cerr << "Could not recv on the listening socket: " << strerror(errno) << "\n";
        exit(EXIT_FAILURE);
    }

    if (::send(client_socket, response.c_str(), response.size(), MSG_NOSIGNAL) == -1) {
        std::cerr << "Could not send on the listening socket: " << strerror(errno) << "\n";
        exit(EXIT_FAILURE);
    }

    close_socket(client_socket);
}

bool close_socket(int socket) {
    return ::close(socket) == 0;
}
