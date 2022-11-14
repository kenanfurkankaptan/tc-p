#include <bitset>
#include <iostream>

#include "connection.h"
#include "ip_header.h"
#include "tcp_header.h"

std::string toIpAddrString(int32_t ip) {
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;
    printf("%d.%d.%d.%d\n", bytes[3], bytes[2], bytes[1], bytes[0]);

    return "";
}

void test_ip_header(Net::Ipv4Header &ip) {
    std::cout << "ver_ihl " << std::bitset<8>(ip.ver_ihl) << std::endl;
    std::cout << "tos " << std::bitset<8>(ip.tos) << std::endl;
    std::cout << "payload_len " << std::bitset<16>(ip.payload_len) << std::endl;
    std::cout << "identification " << std::bitset<16>(ip.identification) << std::endl;
    std::cout << "flags_fo " << std::bitset<16>(ip.flags_fo) << std::endl;
    std::cout << "time_to_live " << std::bitset<8>(ip.time_to_live) << std::endl;
    std::cout << "protocol " << std::bitset<8>(ip.protocol) << std::endl;
    std::cout << "header_checksum " << std::bitset<16>(ip.header_checksum) << std::endl;
    std::cout << "source " << std::bitset<32>(ip.source) << std::endl;
    std::cout << "destination " << std::bitset<32>(ip.destination) << std::endl;
}

void test_ip_options(Net::Ipv4Header &ip) {
    for (int i = 0; i < ip.get_size() - 20; i++) {
        std::cout << std::bitset<8>(ip.options[i]) << std::endl;
    }
}

//-------------------------------------------------------------------------------------------

void test_tcp_flags(Net::TcpHeader &tcp) {
    std::cout << "data_offset_and_flags: " << std::bitset<16>(tcp.data_offset_and_flags) << std::endl;

    std::cout << "header len: " << tcp.get_header_len() << std::endl;
    std::cout << "nnc: " << tcp.nonce() << std::endl;
    std::cout << "cwr: " << tcp.cwr() << std::endl;
    std::cout << "ecn: " << tcp.ech_echo() << std::endl;
    std::cout << "urg: " << tcp.urg() << std::endl;
    std::cout << "ack: " << tcp.ack() << std::endl;
    std::cout << "psh: " << tcp.psh() << std::endl;
    std::cout << "rst: " << tcp.rst() << std::endl;
    std::cout << "syn: " << tcp.syn() << std::endl;
    std::cout << "fin: " << tcp.fin() << std::endl;
}

void test_tcp_options(Net::TcpHeader &tcp) {
    for (int i = 0; i < tcp.get_header_len() - 20; i++) {
        std::cout << std::bitset<8>(tcp.options[i]) << std::endl;
    }
}

//-------------------------------------------------------------------------------------------

void test_connection(Connection &connection) {
    std::cout << "src ip: " << connection.ip.source << std::endl;
    std::cout << "src port: " << connection.tcp.source_port << std::endl;
    std::cout << "dst ip: " << connection.ip.destination << std::endl;
    std::cout << "dst port: " << connection.tcp.destination_port << std::endl;
    std::cout << "state: " << connection.state << std::endl;
}
