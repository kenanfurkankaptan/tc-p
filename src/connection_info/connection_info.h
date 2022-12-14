#ifndef CONNECTION_INFO_H
#define CONNECTION_INFO_H

#include "../connection/connection.h"

class ConnectionInfo {
   public:
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    Connection *connection = nullptr;

    ConnectionInfo(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port);
    ~ConnectionInfo();

    bool check_if_connection_closed() const;
    Connection *create_new_connection();

    bool operator==(const ConnectionInfo &c) const {
        return this->dst_ip == c.dst_ip && this->src_ip == c.src_ip && this->dst_port == c.dst_port && this->src_port == c.src_port;
    }
};

#endif /* CONNECTION_INFO_H */
