#include "controller.h"

#include <algorithm>
#include <iostream>

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
}

void Controller::listen_port(uint16_t port) {
    listened_ports.push_back(port);
    std::cout << "listenning port: " << port << std::endl;
}

void Controller::packet_loop() {
    while (true) {
        char buff[1500];
        tuntap_read(dev, buff, 1500);

        membuf ip_buf(buff, buff + sizeof(buff));
        std::istream ip_packet(&ip_buf);
        Net::Ipv4Header ip = Net::Ipv4Header(ip_packet, true);

        if (ip.protocol != 6) {
            std::cout << "not tcp  packet" << std::endl;
            continue;
        }

        membuf tcp_buf(buff + ip.get_size(), buff + sizeof(buff));
        std::istream tcp_packet(&tcp_buf);

        /** TODO: consider memmove */
        // uint8_t buff2[1500];
        // std::memcpy(&buff2, &buff[ip.size()], sizeof(buff) - ip.size());
        Net::TcpHeader tcp = Net::TcpHeader(tcp_packet, true);

        uint8_t *data = (uint8_t *)buff + ip.get_size() + tcp.get_header_len();
        int data_len = ip.payload_len - (ip.get_size() + tcp.get_header_len());

        ConnectionInfo temp_connection = ConnectionInfo(ip.source, ip.destination, tcp.source_port, tcp.destination_port);

        auto temp_idx = std::find(connection_list.begin(), connection_list.end(), temp_connection);
        if (temp_idx != connection_list.end()) {
            // connection is exist
            connection_list.at(temp_idx - connection_list.begin()).connection->on_packet(dev, ip, tcp, data, data_len);
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

Controller::~Controller() {
    // tuntap_down(dev);
    // tuntap_release(dev);
    // tuntap_destroy(dev);

    delete dev;
    for (ConnectionInfo val : connection_list) {
        delete val.connection;
    }
}
