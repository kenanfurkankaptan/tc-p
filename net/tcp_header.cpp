#include "tcp_header.h"

#include <netinet/in.h>

#include <bitset>

#include "../util/utility.h"

namespace Net {

// TcpHeader::TcpHeader(){};

TcpHeader::TcpHeader(std::istream &stream, bool ntoh) {
    this->ntoh = ntoh;

    stream.read((char *)&source_port, sizeof(source_port));
    stream.read((char *)&destination_port, sizeof(destination_port));
    stream.read((char *)&sequence_number, sizeof(sequence_number));
    stream.read((char *)&acknowledgment_number, sizeof(acknowledgment_number));
    stream.read((char *)&data_offset_and_flags, sizeof(data_offset_and_flags));
    stream.read((char *)&window_size, sizeof(window_size));
    stream.read((char *)&checksum, sizeof(checksum));
    stream.read((char *)&urgent_pointer, sizeof(urgent_pointer));

    // get options len by subtracting min header_len from total_header_len
    stream.read((char *)options, (((ntohs(data_offset_and_flags) & 0xF000) >> 12) * sizeof(uint32_t)) - 20);

    if (ntoh) {
        source_port = ntohs(source_port);
        destination_port = ntohs(destination_port);
        sequence_number = ntohl(sequence_number);
        acknowledgment_number = ntohl(acknowledgment_number);
        data_offset_and_flags = ntohs(data_offset_and_flags);
        window_size = ntohs(window_size);
        checksum = ntohs(checksum);
        urgent_pointer = ntohs(urgent_pointer);
    }
}

TcpHeader::TcpHeader(uint8_t *data, bool ntoh) {
    this->ntoh = ntoh;

    source_port = data[1] << 8 | (data[2] & 0xFF);
    destination_port = data[3] << 8 | (data[4] & 0xFF);
    sequence_number = data[5] << 24 | data[6] << 16 | data[7] << 8 | data[8];
    acknowledgment_number = data[10] << 24 | data[11] << 16 | data[12] << 8 | data[13];
    data_offset_and_flags = data[13] << 8 | (data[14] & 0xFF);
    window_size = data[15] << 8 | (data[16] & 0xFF);
    checksum = data[17] << 8 | (data[18] & 0xFF);
    urgent_pointer = data[19] << 8 | (data[20] & 0xFF);

    if (ntoh) {
        source_port = ntohs(source_port);
        destination_port = ntohs(destination_port);
        sequence_number = ntohl(sequence_number);
        acknowledgment_number = ntohl(acknowledgment_number);
        data_offset_and_flags = ntohs(data_offset_and_flags);
        window_size = ntohs(window_size);
        checksum = ntohs(checksum);
        urgent_pointer = ntohs(urgent_pointer);
    }
}

void TcpHeader::write_to_buff(char *buff) {
    if (ntoh) {
        source_port = ntohs(source_port);
        destination_port = ntohs(destination_port);
        sequence_number = ntohl(sequence_number);
        acknowledgment_number = ntohl(acknowledgment_number);
        data_offset_and_flags = ntohs(data_offset_and_flags);
        window_size = ntohs(window_size);
        checksum = ntohs(checksum);
        urgent_pointer = ntohs(urgent_pointer);
    }

    membuf buff_stream(buff, buff + 20, true);
    std::ostream stream(&buff_stream);

    stream.write((char *)&source_port, sizeof(source_port));
    stream.write((char *)&destination_port, sizeof(destination_port));
    stream.write((char *)&sequence_number, sizeof(sequence_number));
    stream.write((char *)&acknowledgment_number, sizeof(acknowledgment_number));
    stream.write((char *)&data_offset_and_flags, sizeof(data_offset_and_flags));
    stream.write((char *)&window_size, sizeof(window_size));
    stream.write((char *)&checksum, sizeof(checksum));
    stream.write((char *)&urgent_pointer, sizeof(urgent_pointer));

    if (ntoh) {
        source_port = ntohs(source_port);
        destination_port = ntohs(destination_port);
        sequence_number = ntohl(sequence_number);
        acknowledgment_number = ntohl(acknowledgment_number);
        data_offset_and_flags = ntohs(data_offset_and_flags);
        window_size = ntohs(window_size);
        checksum = ntohs(checksum);
        urgent_pointer = ntohs(urgent_pointer);
    }

    return;
}

/**
 * https://gist.github.com/david-hoze/0c7021434796997a4ca42d7731a7073a
 */
uint16_t TcpHeader::compute_tcp_checksum(Net::Ipv4Header &ip_h, uint8_t *data, int data_len) {
    this->checksum = 0;
    uint32_t sum = 0;

    // add psudo ip header
    sum += ((uint16_t *)&ip_h.source)[0];  // get firs part
    sum += ((uint16_t *)&ip_h.source)[1];  // and second part
    sum += ((uint16_t *)&ip_h.destination)[0];
    sum += ((uint16_t *)&ip_h.destination)[1];
    sum += IPPROTO_TCP & 0x000F;                // add the protocol, it always will be tcp protocol
    sum += ip_h.payload_len - ip_h.get_size();  // tcp len, it includes data len

    // add tcp header
    sum += source_port;
    sum += destination_port;
    sum += ((uint16_t *)&sequence_number)[0];
    sum += ((uint16_t *)&sequence_number)[1];
    sum += ((uint16_t *)&acknowledgment_number)[0];
    sum += ((uint16_t *)&acknowledgment_number)[1];
    sum += data_offset_and_flags;
    sum += window_size;
    // sum += checksum;  // it should be 0, calculate it without checksum
    sum += urgent_pointer;

    // add options
    uint16_t *addr_options = (uint16_t *)options;
    int option_count = this->get_header_len() - 20;
    while (option_count > 1) {
        sum += ntohs(*addr_options++);
        option_count -= 2;
    }
    // if any bytes left, pad the bytes and add
    if (option_count > 0) {
        sum += ((*addr_options) & htons(0xFF00));
    }

    // add data
    uint16_t *addr_data = (uint16_t *)data;
    int data_count = data_len;
    while (data_count > 1) {
        sum += ntohs(*addr_data);
        addr_data++;
        data_count -= 2;
    }
    // if any bytes left, pad the bytes and add
    if (data_count > 0) {
        sum += (ntohs(*addr_data) & (0xFF00));
    }

    // Fold sum to 16 bits: add carrier to result
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    // one's complement
    return ((uint16_t)sum ^ 0xFFFF);
};

uint16_t TcpHeader::get_header_len() const {
    return ((data_offset_and_flags & 0xF000) >> 12) * sizeof(uint32_t);
}

void TcpHeader::set_header_len(uint16_t len) {
    if (len > 60) {
        std::cout << "error: tcp header len > 60" << std::endl;
        this->data_offset_and_flags = 0xF000;
        return;
    }
    this->data_offset_and_flags ^= (((len / sizeof(uint32_t)) << 12) ^ this->data_offset_and_flags) & (0xF000);
}

/* get flags */
/*
    A "const function", denoted with the keyword const after a function declaration,
    makes it a compiler error for this class function to change a member variable of the class.
    However, reading of a class variables is okay inside of the function,
    but writing inside of this function will generate a compiler error.
*/
bool TcpHeader::nonce() const {
    return (data_offset_and_flags >> 8) & 1U;
}
bool TcpHeader::cwr() const {
    return (data_offset_and_flags >> 7) & 1U;
}
bool TcpHeader::ech_echo() const {
    return (data_offset_and_flags >> 6) & 1U;
}
bool TcpHeader::urg() const {
    return (data_offset_and_flags >> 5) & 1U;
}
bool TcpHeader::ack() const {
    return (data_offset_and_flags >> 4) & 1U;
}
bool TcpHeader::psh() const {
    return (data_offset_and_flags >> 3) & 1U;
}
bool TcpHeader::rst() const {
    return (data_offset_and_flags >> 2) & 1U;
}
bool TcpHeader::syn() const {
    return (data_offset_and_flags >> 1) & 1U;
}
bool TcpHeader::fin() const {
    return (data_offset_and_flags >> 0) & 1U;
}

/* set flags */
void TcpHeader::nonce(bool flag) {
    this->data_offset_and_flags ^= (-flag ^ this->data_offset_and_flags) & (1UL << 8);
}
void TcpHeader::cwr(bool flag) {
    this->data_offset_and_flags ^= (-flag ^ this->data_offset_and_flags) & (1UL << 7);
}
void TcpHeader::ech_echo(bool flag) {
    this->data_offset_and_flags ^= (-flag ^ this->data_offset_and_flags) & (1UL << 6);
}
void TcpHeader::urg(bool flag) {
    this->data_offset_and_flags ^= (-flag ^ this->data_offset_and_flags) & (1UL << 5);
}
void TcpHeader::ack(bool flag) {
    this->data_offset_and_flags ^= (-flag ^ this->data_offset_and_flags) & (1UL << 4);
}
void TcpHeader::psh(bool flag) {
    this->data_offset_and_flags ^= (-flag ^ this->data_offset_and_flags) & (1UL << 3);
}
void TcpHeader::rst(bool flag) {
    this->data_offset_and_flags ^= (-flag ^ this->data_offset_and_flags) & (1UL << 2);
}
void TcpHeader::syn(bool flag) {
    this->data_offset_and_flags ^= (-flag ^ this->data_offset_and_flags) & (1UL << 1);
}
void TcpHeader::fin(bool flag) {
    this->data_offset_and_flags ^= (-flag ^ this->data_offset_and_flags) & (1UL << 0);
}

}  // namespace Net
