#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <iostream>
#include <map>
#include <vector>

#include "connection.h"

class ConnectionInfo {
   public:
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    Connection *connection;

    ConnectionInfo(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port) {
        this->src_ip = src_ip;
        this->dst_ip = dst_ip;
        this->src_port = src_port;
        this->dst_port = dst_port;
    }

    bool operator==(const ConnectionInfo &c) const {
        return this->dst_ip == c.dst_ip && this->src_ip == c.src_ip && this->dst_port == c.dst_port && this->src_port == c.src_port;
    }
};

class Controller {
   public:
    struct device *dev;
    std::vector<ConnectionInfo> connection_list;
    std::vector<uint16_t> listened_ports;

    Controller();
    ~Controller();

    void packet_loop();
    void listen_port(uint16_t port);
    void write_to_connection(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, std::string &data);
    void add_connection(ConnectionInfo connection_info);
};

#endif /* CONTROLLER_H */