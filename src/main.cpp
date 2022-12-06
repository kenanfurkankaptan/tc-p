#include <netinet/in.h>

#include <bitset>
#include <iostream>
#include <thread>

#include "control/controller.h"

int main() {
    Controller ctrl = Controller();
    ctrl.listen_port(9000);
    ctrl.listen_port(8000);

    // std::this_thread::sleep_for(std::chrono::seconds(15));

    // uint8_t src_ip_arr[] = {192, 168, 0, 1};
    // uint8_t dst_ip_arr[] = {192, 168, 0, 2};

    // uint32_t src_ip = ntohl(*(uint32_t *)src_ip_arr);
    // uint32_t dst_ip = ntohl(*(uint32_t *)dst_ip_arr);

    // ctrl.add_connection(ConnectionInfo(src_ip, dst_ip, 8500, 8600));

    /** TODO: consider running packet loop in another thread */
    ctrl.packet_loop();

    return 0;
}
