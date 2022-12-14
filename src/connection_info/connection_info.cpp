#include "connection_info.h"

ConnectionInfo::ConnectionInfo(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port)
    : src_ip{src_ip}, dst_ip{dst_ip}, src_port{src_port}, dst_port{dst_port} {}

ConnectionInfo::~ConnectionInfo() {
    if (connection != nullptr) delete connection;
}

bool ConnectionInfo::check_if_connection_closed() const { return (connection->state == State::Closed); }

Connection* ConnectionInfo::create_new_connection() {
    this->connection = new Connection();
    return this->connection;
}
