/**
 * main.cpp
 *
 * tc-p is a simple C++ TCP protocol implementation, to serve as an example to anyone who wants to learn it.
 *
 * Use with neccesary caution, this code has not been audited or tested for performance problems, security holes, memory leaks etc.
 *
 */

#include <netinet/in.h>

#include <bitset>
#include <iostream>
#include <thread>

#include "control/controller.h"

int main() {
    std::map<int, int> temp_map = {{1, 1}, {2, 2}, {3, 3}};

    auto temp_ptr = std::next(temp_map.begin(), 1);

    std::cout << (*std::next(temp_map.begin(), 1)).first << std::endl;
    std::cout << (*std::next(temp_map.begin(), 2)).first << std::endl;
    std::cout << (*std::next(temp_map.begin(), 3)).first << std::endl;
    std::cout << (*std::next(temp_map.begin(), 4)).first << std::endl;

    auto ctrl = Controller();
    ctrl.listen_port(9000);
    ctrl.listen_port(9500);

    // std::this_thread::sleep_for(std::chrono::seconds(10));

    // uint8_t src_ip_arr[] = {192, 168, 0, 1};
    // uint8_t dst_ip_arr[] = {192, 168, 0, 2};

    // uint32_t src_ip = ntohl(*(uint32_t *)src_ip_arr);
    // uint32_t dst_ip = ntohl(*(uint32_t *)dst_ip_arr);

    // ctrl.add_connection(ConnectionInfo(src_ip, dst_ip, 8500, 8600));

    /** TODO: consider running packet loop in another thread */
    ctrl.packet_loop();

    return 0;
}