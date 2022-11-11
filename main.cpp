#include <array>
#include <bitset>
#include <cstring>
#include <iostream>
#include <istream>
#include <string>

#include "connection.h"
#include "ip_header.h"
#include "tcp_header.h"
#include "test.cpp"
#include "tuntap++.hh"
#include "tuntap.h"
#include "utility.h"

std::string toIpAddrString(int32_t ip) {
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;
    printf("%d.%d.%d.%d\n", bytes[3], bytes[2], bytes[1], bytes[0]);

    return "";
}

int main(int argc, char const *argv[]) {
    char buff[1500];

    struct device *dev;
    dev = tuntap_init();
    if (tuntap_start(dev, TUNTAP_MODE_TUNNEL, TUNTAP_ID_ANY) == -1) {
        return 1;
    }
    if (tuntap_set_ip(dev, "192.168.0.1", 24) == -1) {
        return 1;
    }
    if (tuntap_up(dev) == -1) {
        return 1;
    }

    Connection connection;

    while (true) {
        tuntap_read(dev, buff, 1500);

        membuf ip_buf(buff, buff + sizeof(buff));
        std::istream ip_packet(&ip_buf);
        Net::Ipv4Header ip = Net::Ipv4Header(ip_packet, true);

        if (ip.protocol != 6) {
            std::cout << "not tcp  packet" << std::endl;
            continue;
        }

        membuf tcp_buf(buff + ip.size(), buff + sizeof(buff));
        std::istream tcp_packet(&tcp_buf);

        /** TODO: consider memmove */
        // uint8_t buff2[1500];
        // std::memcpy(&buff2, &buff[ip.size()], sizeof(buff) - ip.size());
        Net::TcpHeader tcp = Net::TcpHeader(tcp_packet, true);

        uint8_t *data = (uint8_t *)buff + ip.size() + tcp.get_header_len();
        int data_len = ip.payload_len - (ip.size() + tcp.get_header_len());
        connection.on_packet(dev, ip, tcp, data, data_len);

        // test_connection(connection);

        // std::cout << "-----------------------------------------------------------------------------------------------" << std::endl;
    }

    tuntap_destroy(dev);
    return 0;
}
