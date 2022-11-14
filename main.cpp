
#include <iostream>

#include "controller.h"

int main(int argc, char const *argv[]) {
    Controller ctrl = Controller();
    ctrl.listen_port(9000);
    ctrl.listen_port(8000);

    /** TODO: consider running pachet loop in another thread */
    ctrl.packet_loop();

    return 0;
}
