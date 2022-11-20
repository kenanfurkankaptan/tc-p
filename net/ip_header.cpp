#include "ip_header.h"

#include <netinet/in.h>

#include <array>
#include <bitset>

#include "../util/utility.h"

namespace Net {

// Ipv4Header::Ipv4Header(){};

Ipv4Header::Ipv4Header(std::istream &stream, bool ntoh) {
    this->ntoh = ntoh;

    stream.read((char *)&ver_ihl, sizeof(ver_ihl));
    stream.read((char *)&tos, sizeof(tos));
    stream.read((char *)&payload_len, sizeof(payload_len));
    stream.read((char *)&identification, sizeof(identification));
    stream.read((char *)&flags_fo, sizeof(flags_fo));
    stream.read((char *)&time_to_live, sizeof(time_to_live));
    stream.read((char *)&protocol, sizeof(protocol));
    stream.read((char *)&header_checksum, sizeof(header_checksum));
    stream.read((char *)&source, sizeof(source));
    stream.read((char *)&destination, sizeof(destination));

    // get options len by subtracting min header_len from total_header_len
    stream.read((char *)options, ((ver_ihl & 0x0F) * sizeof(uint32_t) - 20));

    if (ntoh) {
        payload_len = ntohs(payload_len);
        identification = ntohs(identification);
        flags_fo = ntohs(flags_fo);
        header_checksum = ntohs(header_checksum);
        source = ntohl(source);
        destination = ntohl(destination);
    }
}

Ipv4Header::Ipv4Header(uint8_t *data, bool ntoh) {
    this->ntoh = ntoh;

    ver_ihl = data[0];
    tos = data[1];
    payload_len = data[2] << 8 | (data[3] & 0xFF);
    identification = data[4] << 8 | (data[5] & 0xFF);
    flags_fo = data[6] << 8 | (data[7] & 0xFF);
    time_to_live = data[8];
    protocol = data[9];
    header_checksum = data[10] << 8 | (data[11] & 0xFF);
    source = data[12] << 24 | data[13] << 16 | data[14] << 8 | data[15];
    destination = data[16] << 24 | data[17] << 16 | data[18] << 8 | data[19];

    if (ntoh) {
        payload_len = ntohs(payload_len);
        identification = ntohs(identification);
        flags_fo = ntohs(flags_fo);
        header_checksum = ntohs(header_checksum);
        source = ntohl(source);
        destination = ntohl(destination);
    }
}

void Ipv4Header::write_to_buff(char *buff) {
    if (this->ntoh) {
        payload_len = ntohs(payload_len);
        identification = ntohs(identification);
        flags_fo = ntohs(flags_fo);
        header_checksum = ntohs(header_checksum);
        source = ntohl(source);
        destination = ntohl(destination);
    }

    membuf buff_stream(buff, buff + 20, true);
    std::ostream stream(&buff_stream);

    stream.write((char *)&ver_ihl, sizeof(ver_ihl));
    stream.write((char *)&tos, sizeof(tos));
    stream.write((char *)&payload_len, sizeof(payload_len));
    stream.write((char *)&identification, sizeof(identification));
    stream.write((char *)&flags_fo, sizeof(flags_fo));
    stream.write((char *)&time_to_live, sizeof(time_to_live));
    stream.write((char *)&protocol, sizeof(protocol));
    stream.write((char *)&header_checksum, sizeof(header_checksum));
    stream.write((char *)&source, sizeof(source));
    stream.write((char *)&destination, sizeof(destination));

    if (this->ntoh) {
        payload_len = ntohs(payload_len);
        identification = ntohs(identification);
        flags_fo = ntohs(flags_fo);
        header_checksum = ntohs(header_checksum);
        source = ntohl(source);
        destination = ntohl(destination);
    }
    return;
}

uint8_t Ipv4Header::ip_version() const {
    return (ver_ihl >> 4) & 0xF0;
}

uint16_t Ipv4Header::get_size() const {
    return (ver_ihl & 0x0F) * sizeof(uint32_t);
}

void Ipv4Header::set_size(uint8_t size) {
    if (size > 60) {
        std::cout << "error: ip header len > 60" << std::endl;
        this->ver_ihl = 0x0F;
        return;
    }
    this->ver_ihl ^= ((size / sizeof(uint32_t)) ^ this->ver_ihl) & (0x0F);
}

/**
 * The method compute_ip_checksum initialize the checksum field of IP header to zeros. Then calls a method compute_checksum.
 * The mothod compute_checksum accepts the computation data and computation length as two input parameters. It sum up all 16-bit
 * words, if there’s odd number of bytes, it adds a padding byte. After summing up all words, it folds the sum to 16 bits by
 * adding the carrier to the results. At last, it takes the one’s complement of sum and cast it to 16-bit unsigned short type.
 * https://gist.github.com/david-hoze/0c7021434796997a4ca42d7731a7073a
 */
uint16_t Ipv4Header::compute_ipv4_checksum() {
    this->header_checksum = 0;
    uint32_t sum = 0;

    // add ip header
    sum += ((ver_ihl << 8) & 0xFF00) | (tos & 0x00FF);
    sum += payload_len;
    sum += identification;
    sum += flags_fo;
    sum += ((time_to_live << 8) & 0xFF00) | (protocol & 0x00FF);
    // sum += header_checksum;           // it is  0
    sum += ((uint16_t *)&source)[0];  // get firs part
    sum += ((uint16_t *)&source)[1];  // and second part
    sum += ((uint16_t *)&destination)[0];
    sum += ((uint16_t *)&destination)[1];

    // add options
    uint8_t *addr = options;
    int count = this->get_size() - 20;
    while (count > 1) {
        sum += ntohs(*addr++);
        count -= 2;
    }
    // if any bytes left, pad the bytes and add
    if (count > 0) {
        sum += ((*addr) & htons(0xFF00));
    }
    // Fold sum to 16 bits: add carrier to result
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // one's complement
    return ((uint16_t)sum ^ 0xFFFF);
};

std::array<int8_t, 4> Ipv4Header::get_src_ip_array() {
    return std::array<int8_t, 4>{
        (int8_t)((source >> 24) & 0xFF),
        (int8_t)((source >> 16) & 0xFF),
        (int8_t)((source >> 8) & 0xFF),
        (int8_t)((source >> 0) & 0xFF),
    };
}

std::array<int8_t, 4> Ipv4Header::get_dst_ip_array() {
    return std::array<int8_t, 4>{
        (int8_t)((destination >> 24) & 0xFF),
        (int8_t)((destination >> 16) & 0xFF),
        (int8_t)((destination >> 8) & 0xFF),
        (int8_t)((destination >> 0) & 0xFF),
    };
}

}  // namespace Net