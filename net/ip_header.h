#ifndef IP_HEADER_H
#define IP_HEADER_H

#include <array>
#include <iostream>

namespace Net {

class Ipv4Header {
   private:
    bool ntoh;

   public:
    uint8_t ver_ihl;  // 4 bits version and 4 bits internet protocol header length
    uint8_t tos;
    uint16_t payload_len;  // total ip packet size
    uint16_t identification;
    uint16_t flags_fo;  // 3 bits flags and 13 bits fragment-offset
    uint8_t time_to_live;
    uint8_t protocol;
    uint16_t header_checksum;  // header checksum
    uint32_t source;
    uint32_t destination;

    uint8_t options[40];

    Ipv4Header() = default;
    Ipv4Header(uint8_t *data, bool ntoh);
    Ipv4Header(std::istream &stream, bool ntoh);

    void write_to_buff(char *buff);

    uint8_t ip_version() const;
    uint16_t get_size() const;
    void set_size(uint8_t size);
    uint16_t compute_ipv4_checksum();

    std::array<int8_t, 4> get_src_ip_array();
    std::array<int8_t, 4> get_dst_ip_array();
};

}  // namespace Net

#endif /* IP_HEADER_H */
