#include "connection_info.h"

ConnectionInfo::ConnectionInfo(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, const std::map<std::string, std::string>& response_table)
    : src_ip{src_ip}, dst_ip{dst_ip}, src_port{src_port}, dst_port{dst_port} {
    if (!response_table.empty()) this->response_table = response_table;
}

ConnectionInfo::~ConnectionInfo() = default;

bool ConnectionInfo::check_if_connection_closed() const { return connection->is_connection_closed(); }

std::unique_ptr<Connection>& ConnectionInfo::create_new_connection() {
    this->connection = std::make_unique<Connection>();
    return this->connection;
}
