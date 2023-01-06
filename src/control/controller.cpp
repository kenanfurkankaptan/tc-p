#include "controller.h"

#include <string.h>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

Controller::Controller() {
    struct device *dev;

    dev = tuntap_init();
    this->dev = dev;

    if (tuntap_start(dev, TUNTAP_MODE_TUNNEL, TUNTAP_ID_ANY) == -1) {
        return;
    }
    if (tuntap_set_ip(dev, "192.168.0.1", 24) == -1) {
        return;
    }
    if (tuntap_up(dev) == -1) {
        return;
    }

    connection_list = {};
    listened_ports = {};
}

/** TODO: fix destroy */
Controller::~Controller() {
    for (auto p : connection_list) {
        delete p;
    }
    connection_list.clear();
    listened_ports.clear();

    tuntap_down(dev);
    tuntap_destroy(dev);
}

void Controller::listen_port(uint16_t port) {
    listened_ports.push_back(port);
    std::cout << "listenning port: " << port << std::endl;
}

void Controller::write_to_connection(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, std::string &data) {
    for (auto c : connection_list) {
        if (*c == ConnectionInfo(src_ip, dst_ip, src_port, dst_port)) {
            /** TODO: it added for tests, remove it later */
            if (data == "exit\n") {
                c->connection->close_send();
            } else {
                c->connection->send_data(data);
            }
            return;
        }
    }
    std::cout << "connection not found!!" << std::endl;
}

// add_connection() accepts ip's as host byte order.
// use ntohl to convert them network byte order to host byte order.
void Controller::add_connection(ConnectionInfo *connection_info) {
    auto temp_connection = new ConnectionInfo(connection_info->src_ip, connection_info->dst_ip, connection_info->src_port, connection_info->dst_port);
    connection_list.push_back(temp_connection);
    connection_list.back()->create_new_connection()->connect(dev, connection_info->src_ip, connection_info->dst_ip, connection_info->src_port,
                                                             connection_info->dst_port);

    std::cout << "start connection on port: " << connection_info->dst_port << std::endl;
}

void Controller::packet_loop() {
    /** TODO: refactor in a better way */
    // wake up every 1 ms
    std::jthread timer_loop([&]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            // TODO: mutex locks whole loop unnecessarily find better way
            std::lock_guard connection_list_guard(this->connection_list_mutex);
            for (int idx = 0; auto c : connection_list) {
                if (c->connection == nullptr) return;
                c->connection->on_tick(dev);
                c->connection->check_close_timer();

                // remove connection if it is in closed state
                if (c->check_if_connection_closed()) {
                    delete c;
                    connection_list.erase(connection_list.begin() + idx);
                    std::cout << "connection removed from the list " << std::endl;
                }
                ++idx;
            }
        }
    });

    while (true) {
        // create and reset buffer
        char buff[1500];
        memset(buff, 0, 1500);

        // blocks until tuntap read
        tuntap_read(dev, buff, 1500);

        membuf ip_buf(buff, buff + sizeof(buff));
        std::istream ip_packet(&ip_buf);
        auto ip = Net::Ipv4Header(ip_packet, true);

        if (ip.ip_version() != 4) {
            std::cout << "packet dropped: not ipv4 packet " << std::endl;
            continue;
        }

        if (ip.protocol != 6) {
            std::cout << "packet dropped: not tcp segment" << std::endl;
            continue;
        }

        membuf tcp_buf(buff + ip.get_size(), buff + sizeof(buff));
        std::istream tcp_packet(&tcp_buf);

        auto tcp = Net::TcpHeader(tcp_packet, true);

        uint8_t *data = (uint8_t *)buff + ip.get_size() + tcp.get_header_len();
        int data_len = ip.payload_len - (ip.get_size() + tcp.get_header_len());

        // TODO: mutex locks whole loop unnecessarily find better way
        std::lock_guard connection_list_guard(this->connection_list_mutex);

        // check if same connection established before
        auto index_iterator = std::ranges::find_if(connection_list.begin(), connection_list.end(), [&](ConnectionInfo const *c) {
            return c->src_ip == ip.source && c->dst_ip == ip.destination && c->src_port == tcp.source_port && c->dst_port == tcp.destination_port;
        });
        if (index_iterator != connection_list.end()) {
            // connection is exist
            long int index = index_iterator - connection_list.begin();
            auto index_connection = connection_list.at(index);

            index_connection->connection->on_packet(dev, ip, tcp, data, data_len);

            if (data_len > 0) {
                auto data_str = std::string((char *)data);

                // if data ends with newline, remove it
                // in response table newlines are not included
                if (data_str.back() == '\n') data_str.pop_back();
                if (index_connection->response_table.contains(data_str))
                    this->write_to_connection(ip.source, ip.destination, tcp.source_port, tcp.destination_port, index_connection->response_table[data_str]);
            }

        } else {
            // connection is not exist
            if (std::ranges::find(listened_ports.begin(), listened_ports.end(), tcp.destination_port) != listened_ports.end()) {
                // port is accepted, create new connection
                auto temp_connection = new ConnectionInfo(ip.source, ip.destination, tcp.source_port, tcp.destination_port);

                connection_list.push_back(temp_connection);
                connection_list.back()->create_new_connection()->on_packet(dev, ip, tcp, data, data_len);

                std::cout << "new packet on port: " << temp_connection->dst_port << std::endl;
            } else {
                // port is not accepted
                std::cout << "port: " << tcp.destination_port << " is not accepted" << std::endl;
            }
        }
    }
}
