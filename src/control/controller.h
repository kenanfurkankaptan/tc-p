#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <iostream>
#include <map>
#include <memory>
#include <vector>

#include "../connection/connection.h"
#include "../connection_info/connection_info.h"

class Controller {
   public:
    struct device *dev;

    std::vector<std::unique_ptr<ConnectionInfo>> connection_list;
    std::vector<uint16_t> listened_ports;

    std::mutex connection_list_mutex;

    explicit Controller(const std::string ip);
    ~Controller();

    void packet_loop();
    void listen_port(uint16_t port);
    void write_to_connection(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, std::string &data) const;
    void add_connection(const ConnectionInfo *connection_info);
};

#endif /* CONTROLLER_H */