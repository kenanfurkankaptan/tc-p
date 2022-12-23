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
    auto ctrl = Controller();
    ctrl.listen_port(9000);
    ctrl.listen_port(9500);

    ctrl.packet_loop();

    // std::this_thread::sleep_for(std::chrono::seconds(10));

    // uint8_t src_ip_arr[] = {192, 168, 0, 3};
    // uint8_t dst_ip_arr[] = {192, 168, 0, 1};
    // uint32_t src_ip = ntohl(*(uint32_t *)src_ip_arr);
    // uint32_t dst_ip = ntohl(*(uint32_t *)dst_ip_arr);
    // ctrl.add_connection(new ConnectionInfo(src_ip, dst_ip, 8500, 8600));

    /** TODO: consider running packet loop in another thread */
    // ctrl.packet_loop();

    return 0;
}