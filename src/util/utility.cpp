#include "utility.h"

#include <iostream>

membuf::membuf(char* begin, char* end, bool write) {
    write ? this->setp(begin, end) : this->setg(begin, begin, end);
}

bool wrapping_lt(uint32_t lhs, uint32_t rhs) {
    // From RFC1323
    // TCP determines if a data segment is 'old' or new by testing
    // weather its sequence number is within 2**31 bytes of left edge
    // of the window, and if it is not disgarding data as old. To
    // insured that new data is never mistakenly considered old and
    // vice-versa, the left edge of the sender's window has to be at
    // most 2**31 away from the right edge of the receiver's window

    /*
    for native data types such as int8/int16/int32 wrapping is provided by the hardware
    */
    return (lhs - rhs) > ((uint32_t)1 << 31);
};

bool is_between_wrapped(uint32_t start, uint32_t x, uint32_t end) {
    return wrapping_lt(start, x) && wrapping_lt(x, end);
}
