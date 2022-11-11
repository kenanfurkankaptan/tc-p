#include "connection.h"

// Connection::Connection(Connection &c) {
//     this->state = c.state;
//     this->send = c.send;
//     this->recv = c.recv;
//     this->ip = c.ip;
//     this->tcp = c.tcp;
//     this->incoming = c.incoming;
//     this->unacked = c.unacked;
//     this->closed = c.closed;
//     this->closed_at = c.closed_at;
// }

void Connection::accept(struct device *dev, Net::Ipv4Header &ip_h, Net ::TcpHeader &tcp_h) {
    if (!tcp_h.syn()) {
        // only expected SYN packet
        /** TODO: fix return value */
        std::cout << "not syn packet" << std::endl;
        return;
    }

    uint16_t iss = 0;
    uint16_t wnd = 1024;

    this->state = State::SynRcvd;
    this->send = SendSequenceSpace{iss, iss, wnd, false, 0, 0, iss};
    this->recv = RecvSequenceSpace{tcp_h.sequence_number + 1, tcp_h.window_size, false, tcp_h.sequence_number};
    this->ip = ([&]() {
        Net::Ipv4Header temp_ip = ip_h;
        temp_ip.destination = ip_h.source;
        temp_ip.source = ip_h.destination;
        return temp_ip;
    }());
    this->tcp = ([&]() {
        Net::TcpHeader temp_tcp = tcp_h;
        temp_tcp.destination_port = tcp_h.source_port;
        temp_tcp.source_port = tcp_h.destination_port;
        temp_tcp.acknowledgment_number = 0;
        temp_tcp.window_size = wnd;
        temp_tcp.set_header_len(20);
        return temp_tcp;
    }());
    this->incoming = Queue();
    this->unacked = Queue();
    this->closed = false;
    this->closed_at = -1;
    //         timers: Timers {
    //             send_times: Default::default(),
    //             srtt: time::Duration::from_secs(1 * 60).as_secs_f64(),
    //         },
    //     };

    // needs to start establishing connection
    this->tcp.syn(true);
    this->tcp.ack(true);
    this->write(dev, this->send.nxt, 0);

    return;
}

void Connection::on_packet(struct device *dev, Net::Ipv4Header &ip_h, Net ::TcpHeader &tcp_h, uint8_t *data, int data_len) {
    if (this->state == State::Listen) {
        this->accept(dev, ip_h, tcp_h);
        std::cout << "connection is accepted" << std::endl;
        this->state = State::SynRcvd;
        return;
    }

    // first check that sequence numbers are valid (RFC 793 S3.3)
    //
    // valid segment check okay if it acks at least one byte, which means that at least one of the following is true
    // RCV.NXT =< SEG.SEQ < RCV.NXT + RCV.WND
    // RCV.NXT =< SEG.SEQ + SEG.LEN-1 < RCV.NXT + RCV.WND
    //
    uint32_t seqn = tcp_h.sequence_number;  // sequence number
    int slen = data_len;

    if (tcp_h.fin()) {
        slen += 1;
    };
    if (tcp_h.syn()) {
        slen += 1;
    };
    uint32_t wend = this->recv.nxt + this->recv.wnd;  // window end
    bool okay = [&]() -> bool {
        if (slen == 0) {
            // zero-length segment has seperate rules for acceptance
            if (this->recv.wnd == 0) {
                if (seqn != this->recv.nxt) {
                    return false;
                } else {
                    return true;
                }
            } else if (!is_between_wrapped(this->recv.nxt - 1, seqn, wend)) {
                return false;
            } else {
                return true;
            }
        } else {
            if (this->recv.wnd == 0) {
                return false;
            } else if (!is_between_wrapped(this->recv.nxt - 1, seqn, wend) && !is_between_wrapped(this->recv.nxt - 1, seqn + slen - 1, wend)) {
                return false;
            } else {
                return true;
            }
        };
    }();

    if (!okay) {
        std::cout << "NOT OKAY" << std::endl;
        this->write(dev, this->send.nxt, 0);
        return this->availability();
    }

    if (!tcp_h.ack()) {
        if (tcp_h.syn()) {
            // got SYN part of initial handshake
            assert(data_len == 0);
            this->recv.nxt = seqn + 1;
        }
        return this->availability();
    }

    uint32_t ackn = tcp_h.acknowledgment_number;  // ack number
    /* STATES */
    if (this->state == State::SynRcvd) {
        if (is_between_wrapped(this->send.una - 1, ackn, this->send.nxt + 1)) {
            // must have ACKed our SYN, since we detected at least one ACKed byte
            // and we have only one byte (the SYN)
            this->state = State::Estab;
        } else {
            /** TODO: RST : <SEQ=SEH.ACK><CTL=RST> */
        }
    }
    if ((this->state == State::Estab) | State::FinWait1 | State::FinWait2) {
        if (is_between_wrapped(this->send.una, ackn, this->send.nxt + 1)) {
            if (!this->unacked.data.empty()) {
                int32_t data_start = [&]() {
                    if (this->send.una == this->send.iss) {
                        // send.una hasn't been updated yet with ACK for our SYN, so data starts just beyond it
                        return this->send.una + 1;
                    } else {
                        return this->send.una;
                    };
                }();

                int32_t acked_data_end = std::min(ackn - data_start, (uint32_t)this->unacked.data.size());

                // self.unacked.drain(..acked_data_end);
                // self.timers.send_times.retain(
                //     | &seq, sent | {
                //           if is_between_wrapped (self.send.una, seq, ackn) {
                //               self.timers.srtt =
                //                   0.8 * self.timers.srtt + (1.0 - 0.8) * sent.elapsed().as_secs_f64();
                //               false
                //           } else {
                //               true
                //           }
                //       });

                //----------------------------------------------------------------------------------
            }
            this->send.una = ackn;
        }
        /** TODO: if unacked empty and waiting flush, notify */
        /** TODO: update window */
    }

    // receive ack for out FIN
    if (this->state == State::FinWait1) {
        if (this->send.una == closed_at + 1) {
            // our FIN has been ACKed!
            this->state = State::FinWait2;
        }
    }
    if (this->state == State::Closing) {
        if (this->send.una == closed_at + 1) {
            // our FIN has been ACKed!
            this->state = State::TimeWait;
        }
    }
    if (this->state == State::LastAck) {
        if (this->send.una == closed_at + 1) {
            // our FIN has been ACKed!
            this->state = State::Closed;
        }
    }

    if (data_len > 0) {
        if ((this->state == State::Estab) | State::FinWait1 | State::FinWait2) {
            uint32_t unread_data_at = this->recv.nxt - seqn;

            if (unread_data_at > (uint32_t)data_len) {
                // we must have received a re-transmitted FIN that we ahve already seen
                // nxt points to beyond the fin, but the fin is not in data
                assert(unread_data_at == (uint32_t)(data_len + 1));
                unread_data_at = 0;
            }

            incoming.enqueue(data, data_len);

            std::cout << "incoming data" << std::endl;
            unsigned char *ptr = data;
            for (int i = 0; i < data_len; i++) {
                std::cout << ptr[i];
            }
            std::cout << std::endl;

            /*
            Once the TCP takes responsibility for the data it advances
            RCV.NXT over the data accepted, and adjusts RCV.WND as
            apporopriate to the current buffer availability. The total of
            RCV.NXT and RCV.WND should not be reduced.
            */
            this->recv.nxt = seqn + data_len;

            /* Send an acknowledgment of the form: <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> */
            /** TODO: maybe just tick to piggyback ack on data?? */
            // std::cout << seqn << std::endl;
            // std::cout << tcp_h.sequence_number << std::endl;
            // std::cout << data_len << std::endl;
            // std::cout << this->send.nxt << std::endl;
            this->write(dev, this->send.nxt, 0);
        }
    }

    if (tcp_h.fin()) {
        // we are done with connection
        switch (this->state) {
            case FinWait2:
                this->recv.nxt++;
                this->write(dev, this->send.nxt, 0);
                this->state = State::TimeWait;
                break;
            case FinWait1:
                this->recv.nxt++;
                this->write(dev, this->send.nxt, 0);
                this->state = State::Closing;
                break;
            case Estab:
                this->recv.nxt++;
                this->write(dev, this->send.nxt, 0);
                this->state = State::TimeWait;
                break;

            // we are not expecting FIN flag in other states
            default:
                break;
        }
    }
    return this->availability();
}

void Connection::write(struct device *dev, uint32_t seq, uint32_t limit) {
    const uint32_t buf_len = 1500;
    uint8_t buf[buf_len] = {};

    this->tcp.sequence_number = seq;
    this->tcp.acknowledgment_number = this->recv.nxt;

    uint32_t offset = seq - this->send.una;
    // we need two special case the two 'virtual' bytes SYN and FIN
    if (seq == closed_at + 1) {
        // trying to write following FIN
        offset = 0;
        limit = 0;
    }

    // let (mut h, mut t) = self.unacked.as_slices();
    // if h.len() >= offset {
    //     h = &h[offset..];
    // } else {
    //     let skipped = h.len();
    //     h = &[];
    //     t = &t[(offset - skipped)..];
    // }

    const uint32_t max_data = std::min(limit, (uint32_t)unacked.data.size());
    const uint32_t size = std::min((uint32_t)buf_len, this->tcp.get_header_len() + ip.size() + max_data);

    this->ip.payload_len = size - ip.size();

    uint8_t *unwritten = buf;
    // uint8_t unwritten[size];
    // std::copy(buf, buf + size, unwritten);

    this->ip.header_checksum = this->ip.compute_ipv4_checksum();
    this->ip.write_to_buff((char *)unwritten);
    uint32_t ip_header_ends_at = ip.size();

    // postpone writing the tcp header because we need the payload as one contiguous slice to calculate the tcp checksum
    this->tcp.set_header_len(20);
    uint32_t tcp_header_ends_at = ip_header_ends_at + this->tcp.get_header_len();

    uint32_t payload_bytes = ([&]() -> uint32_t {
        uint32_t written = 0;
        written += this->unacked.dequeue(unwritten + tcp_header_ends_at, max_data);
        return written;
    }());

    uint32_t payload_ends_at = tcp_header_ends_at + payload_bytes;

    this->tcp.checksum = this->tcp.compute_tcp_checksum(this->ip, unwritten + tcp_header_ends_at, payload_bytes);
    this->tcp.write_to_buff((char *)(unwritten) + ip_header_ends_at);

    std::cout << ip_header_ends_at << std::endl;
    std::cout << tcp_header_ends_at << std::endl;
    std::cout << payload_ends_at << std::endl;

    uint32_t next_seq = seq + payload_bytes;
    if (this->tcp.syn()) {
        next_seq++;
        this->tcp.syn(false);
    }
    if (this->tcp.fin()) {
        next_seq++;
        this->tcp.fin(false);
    }
    if (wrapping_lt(this->send.nxt, next_seq)) {
        this->send.nxt = next_seq;
    }

    /** TODO: implement timers */
    // self.timers.send_times.insert(seq, time::Instant::now());

    tuntap_write(dev, buf, payload_ends_at);
    return;
}

/** TODO: implement*/
void Connection::availability() {
    return;
}