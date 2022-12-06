#include "controller.h"

#include <string.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

ConnectionInfo::ConnectionInfo(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port) {
    this->src_ip = src_ip;
    this->dst_ip = dst_ip;
    this->src_port = src_port;
    this->dst_port = dst_port;

    this->connection = nullptr;
}

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
    // tuntap_down(dev);
    // tuntap_release(dev);
    // tuntap_destroy(dev);

    // delete dev;
    connection_list.clear();
    listened_ports.clear();
}

void Controller::listen_port(uint16_t port) {
    listened_ports.push_back(port);
    std::cout << "listenning port: " << port << std::endl;
}

void Controller::write_to_connection(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, std::string &data) {
    for (auto c : connection_list) {
        if (dst_ip == c.dst_ip && src_ip == c.src_ip && dst_port == c.dst_port && src_port == c.src_port) {
            c.connection->unacked.enqueue(reinterpret_cast<uint8_t *>(&data[0]), (int)data.length());
            c.connection->close(dev);
        }
    }
}

void Controller::add_connection(ConnectionInfo connection_info) {
    auto temp_ptr = new Connection();
    connection_info.connection = temp_ptr;
    this->connection_list.push_back(connection_info);

    temp_ptr->connect(dev, connection_info.src_ip, connection_info.dst_ip, connection_info.src_port, connection_info.dst_port);

    std::cout << "start connection on port: " << connection_info.dst_port << std::endl;
}

void Controller::packet_loop() {
    /** TODO: refactor in a better way */
    // wake up every 1 ms
    std::thread timer_loop([&]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            for (auto c : connection_list) {
                std::lock_guard<std::mutex> guard(c.connection->lockMutex);

                c.connection->on_tick(dev);
            }
        }
    });

    while (true) {
        char buff[1500];
        tuntap_read(dev, buff, 1500);

        membuf ip_buf(buff, buff + sizeof(buff));
        std::istream ip_packet(&ip_buf);
        Net::Ipv4Header ip = Net::Ipv4Header(ip_packet, true);

        if (ip.ip_version() != 4) {
            std::cout << "not ipv4 packet: " << std::bitset<8>(ip.ip_version()) << std::endl;
            continue;
        }

        if (ip.protocol != 6) {
            std::cout << "not tcp packet" << std::endl;
            continue;
        }

        membuf tcp_buf(buff + ip.get_size(), buff + sizeof(buff));
        std::istream tcp_packet(&tcp_buf);

        /** TODO: consider memmove */
        Net::TcpHeader tcp = Net::TcpHeader(tcp_packet, true);

        uint8_t *data = (uint8_t *)buff + ip.get_size() + tcp.get_header_len();
        int data_len = ip.payload_len - (ip.get_size() + tcp.get_header_len());

        ConnectionInfo temp_connection = ConnectionInfo(ip.source, ip.destination, tcp.source_port, tcp.destination_port);

        auto temp_idx = std::find(connection_list.begin(), connection_list.end(), temp_connection);
        if (temp_idx != connection_list.end()) {
            // connection is exist
            connection_list.at(temp_idx - connection_list.begin()).connection->on_packet(dev, ip, tcp, data, data_len);

            if (data_len && strncmp((const char *)data, "hello\n", data_len) == 0) {
                std::string temp55 = "Data received";
                this->write_to_connection(ip.source, ip.destination, tcp.source_port, tcp.destination_port, temp55);
            }

        } else {
            // connection is not exist
            if (std::find(listened_ports.begin(), listened_ports.end(), temp_connection.dst_port) != listened_ports.end()) {
                // port is accepted
                auto temp_ptr = new Connection();
                temp_connection.connection = temp_ptr;
                temp_ptr->on_packet(dev, ip, tcp, data, data_len);
                connection_list.push_back(temp_connection);
                std::cout << "packet on port: " << temp_connection.dst_port << std::endl;
            } else {
                // port is not accepted
                std::cout << "port: " << temp_connection.dst_port << " is not accepted" << std::endl;
            }
        }
    }
}
