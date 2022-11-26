
#ifndef TCP_HEADER_H
#define TCP_HEADER_H

#include <iostream>

#include "../ip_header/ip_header.h"

namespace Net {

class TcpHeader {
   private:
    bool ntoh;

   public:
    uint16_t source_port;
    uint16_t destination_port;
    uint32_t sequence_number;
    uint32_t acknowledgment_number;
    uint16_t data_offset_and_flags;  // 4 bits data_offset, 3 bit reserve, 9 bit flags
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_pointer;

    uint8_t options[40];

    TcpHeader();
    TcpHeader(uint8_t *data, bool ntoh);
    TcpHeader(std::istream &stream, bool ntoh);

    void write_to_buff(char *buff);

    uint16_t compute_tcp_checksum(Net::Ipv4Header &ip_h, uint8_t *data, int data_len);

    // tcp_header_t load(std::istream &stream, bool ntoh);

    uint16_t get_header_len() const;
    void set_header_len(uint16_t len);

    /* get flags */
    bool nonce() const;
    bool cwr() const;
    bool ech_echo() const;
    bool urg() const;
    bool ack() const;
    bool psh() const;
    bool rst() const;
    bool syn() const;
    bool fin() const;

    /* set flags */
    void nonce(bool flag);
    void cwr(bool flag);
    void ech_echo(bool flag);
    void urg(bool flag);
    void ack(bool flag);
    void psh(bool flag);
    void rst(bool flag);
    void syn(bool flag);
    void fin(bool flag);
};
}  // namespace Net

#endif /* TCP_HEADER_H */
