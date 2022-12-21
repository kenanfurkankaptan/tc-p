#include "connection.h"

// generates connection with incoming packets
void Connection::accept(struct device *dev, const Net::Ipv4Header &ip_h, const Net ::TcpHeader &tcp_h) {
    uint16_t iss = 0;
    uint16_t wnd = 1024;

    this->state = State::SynRcvd;
    this->previous_state = State::Listen;

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
        return temp_tcp;
    }());
    this->incoming = Queue();
    this->unacked = Queue();
    this->send_closed = false;
    this->send_closed_at = 0;
    this->connection_is_closed = false;

    // needs to start establishing connection
    this->tcp.syn(true);
    this->tcp.ack(true);
    this->write(dev, this->send.nxt, 0);

    return;
}

void Connection::connect(struct device *dev, uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port) {
    uint16_t iss = 0;
    uint16_t wnd = 1024;

    this->state = State::SynSent;
    this->previous_state = State::Listen;

    this->recv = RecvSequenceSpace();
    this->send = SendSequenceSpace{iss, iss, wnd, false, 0, 0, iss};
    this->ip = Net::Ipv4Header();
    this->ip.source = src_ip;
    this->ip.destination = dst_ip;

    this->tcp = Net::TcpHeader();
    this->tcp.destination_port = dst_port;
    this->tcp.source_port = src_port;
    this->tcp.acknowledgment_number = 0;
    this->tcp.window_size = wnd;

    this->incoming = Queue();
    this->unacked = Queue();
    this->send_closed = false;
    this->send_closed_at = 0;
    this->connection_is_closed = false;

    // needs to start establishing connection
    this->tcp.syn(true);
    this->write(dev, this->send.nxt, 0);

    return;
}

void Connection::delete_TCB() {
    this->state = State::Closed;
    this->previous_state = State::Closed;

    this->send = SendSequenceSpace{};
    this->recv = RecvSequenceSpace{};
    this->ip = Net::Ipv4Header();
    this->tcp = Net::TcpHeader();
    this->incoming = Queue();
    this->unacked = Queue();
    this->send_closed = false;
    this->send_closed_at = 0;
    this->connection_is_closed = false;

    std::cout << "TCB is deleted" << std::endl;

    return;
}

void Connection::close_send() {
    // do it once
    if (!this->send_closed) {
        this->send_closed = true;
        this->send_closed_at = this->send.una + (uint32_t)this->unacked.data.size() + 1;
        if (this->state == State::SynRcvd || this->state == State::Estab) this->change_state(State::FinWait1);
    }
}

void Connection::start_close_timer() {
    if (connection_is_closed)
        std::cout << "close timer restarted" << std::endl;
    else
        std::cout << "close timer started" << std::endl;

    connection_is_closed = true;
    connection_close_start_time = std::chrono::high_resolution_clock::now();
}

void Connection::check_close_timer() {
    // connection is still active no need to check
    if (!this->connection_is_closed) return;

    auto time_passed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - connection_close_start_time).count();

    // 2 * 1000 = 2 milisecond
    if (time_passed > (2 * 1000)) delete_TCB();
    return;
}

void Connection::on_packet(struct device *dev, const Net::Ipv4Header &ip_h, const Net ::TcpHeader &tcp_h, uint8_t *data, int data_len) {
    uint32_t seqn = tcp_h.sequence_number;            // sequence number
    uint32_t ackn = tcp_h.acknowledgment_number;      // ack number
    uint32_t wend = this->recv.nxt + this->recv.wnd;  // window end

    // The TCP header, also referred to as "segment header", and the payload, or data, or "segment data" make up the TCP segment, of varying size.
    int slen = data_len;
    if (tcp_h.fin()) {
        slen += 1;
    }
    if (tcp_h.syn()) {
        slen += 1;
    }

    /* STATES */
    if (this->state == State::Closed) {
        if (tcp_h.rst()) {
            return;
        } else {
            return send_rst(dev, ip_h, tcp_h, slen);
        }
    } else if (this->state == State::Listen) {
        // we only expect syn packet
        if (tcp_h.rst())
            return;
        else if (tcp_h.ack())
            return send_rst(dev, ip_h, tcp_h, slen);
        else if (tcp_h.syn())
            /** TODO: check security/compartment and SEG.PRC */
            return this->accept(dev, ip_h, tcp_h);
        else
            return;
    } else if (this->state == State::SynSent) {
        bool is_ack_acceptable = NULL;

        if (tcp_h.ack()) {
            /** If SEG.ACK =< ISS, or SEG.ACK > SND.NXT, send a reset */
            if (wrapping_lt(send.iss - 1, tcp_h.acknowledgment_number) || wrapping_lt(tcp_h.acknowledgment_number, send.iss))
                return send_rst(dev, ip_h, tcp_h, slen);

            /** If SND.UNA =< SEG.ACK =< SND.NXT then the ACK is acceptable. */
            is_ack_acceptable = is_between_wrapped(this->send.una - 1, ackn, this->send.nxt + 1);
        }
        if (tcp_h.rst()) {
            if (is_ack_acceptable) {
                std::cout << "error: connection reset " << std::endl;
                delete_TCB();
            } else {
                return;
            }
        }
        /** TODO: check security/compartment and SEG.PRC */

        if (is_ack_acceptable == false) return;
        /** This step should be reached only if the ACK is ok, or there is
        no ACK, and it the segment did not contain a RST.
        */
        if (tcp_h.syn()) {
            this->send.nxt = tcp_h.sequence_number + 1;
            this->recv.irs = tcp_h.sequence_number;
            this->send.una = is_ack_acceptable ? tcp_h.acknowledgment_number : this->send.una;
            /** TODO: any segments on the retransmission queue which
            are thereby acknowledged should be removed */

            /** If SND.UNA > ISS (our SYN has been ACKed) */
            if (this->send.una > this->send.iss) {
                return this->write(dev, this->send.nxt, 0);
            } else {
                this->change_state(State::SynRcvd);
                this->tcp.syn(true);
                return this->write(dev, this->send.iss, 0);
            }
        }
        if (!tcp.syn() && !tcp.rst()) return;

        return;
    }

    /**  otherwise
        SYN-RECEIVED STATE
        ESTABLISHED STATE
        FIN-WAIT-1 STATE
        FIN-WAIT-2 STATE
        CLOSE-WAIT STATE
        CLOSING STATE
        LAST-ACK STATE
        TIME-WAIT STATE
    */
    else {
        // first check that sequence numbers are valid (RFC 793 S3.3)
        //
        // valid segment check okay if it acks at least one byte, which means that at least one of the following is true
        // RCV.NXT =< SEG.SEQ < RCV.NXT + RCV.WND
        // RCV.NXT =< SEG.SEQ + SEG.LEN-1 < RCV.NXT + RCV.WND
        //
        bool okay = sequence_number_check(slen, seqn, wend);
        if (!okay) {
            std::cout << "SEQUENCE NUMBER IS NOT OKAY" << std::endl;

            if (tcp_h.rst()) return;
            return this->write(dev, this->send.nxt, 0);
        }

        if (tcp_h.rst()) {
            if (this->state == State::SynRcvd) {
                /*
                If this connection was initiated with a passive OPEN (i.e.,
                came from the LISTEN state), then return this connection to
                LISTEN state and return.
                */
                if (this->previous_state == State::Listen) {
                    this->change_state(State::Listen);
                    return;
                }
                /*
                If this connection was initiated with an active OPEN (i.e., came
                from SYN-SENT state) then the connection was refused, enter CLOSED
                state and return.
                */
                else if (this->previous_state == State::SynSent) {
                    delete_TCB();
                    return;
                }
                /*
                SynRcvd state only reachable from Listen or SynSent states
                other states are considered to be an error
                */
                else {
                    return;
                }
            } else if ((state == State::Estab) || (state == State::FinWait1) || (state == State::FinWait2) || (state == State::CloseWait)) {
                /** TODO: flush all queue */
                delete_TCB();
            } else if ((state == State::Closing) || (state == State::LastAck) || (state == State::TimeWait)) {
                delete_TCB();
            }
        }

        /** TODO: check security/compartment and SEG.PRC */

        if (tcp_h.syn()) {
            this->send_rst(dev, ip_h, tcp_h, slen);

            /** TODO: flush */
            return;
        }

        if (!tcp_h.ack()) {
            return;
        } else {
            if (this->state == State::SynRcvd) {
                if (is_between_wrapped(this->send.una - 1, ackn, this->send.nxt + 1)) {
                    // must have ACKed our SYN, since we detected at least one ACKed byte
                    // and we have only one byte (the SYN)
                    this->change_state(State::Estab);
                } else {
                    send_rst(dev, ip_h, tcp_h, slen);
                }
            }
            if ((state == State::Estab) || (state == State::FinWait1) || (state == State::FinWait2) || (state == State::CloseWait) ||
                (state == State::Closing)) {
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
                        this->unacked.remove(acked_data_end);

                        std::map<uint32_t, std::chrono::_V2::system_clock::time_point>::const_iterator itr;
                        auto map2 = this->timers.send_times;
                        for (itr = map2.cbegin(); itr != map2.cend();) {
                            itr = ([&]() {
                                auto seq = itr->first;
                                auto sent = itr->second;
                                if (is_between_wrapped(this->send.una, seq, ackn)) {
                                    auto elapsed =
                                        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - sent).count();
                                    /** SRTT = ( ALPHA * SRTT ) + ((1-ALPHA) * RTT)
                                     * Alpha is taken 0.8 */
                                    this->timers.srtt = ((int64_t)0.8 * this->timers.srtt + (int64_t)(1.0 - 0.8) * elapsed);
                                    return true;
                                } else {
                                    return false;
                                }
                            }())
                                      ? map2.erase(itr)
                                      : std::next(itr);
                        }
                    }

                    //----------------------------------------------------------------------------------

                    this->send.una = ackn;
                    /** TODO: update window */
                }

                if (this->state == State::FinWait1) {
                    // receive ack for out FIN
                    if (this->send.una == send_closed_at + 1) {
                        // our FIN has been ACKed!
                        std::cout << "Our FIN has been ACKed" << std::endl;
                        this->change_state(State::FinWait2);
                    }
                }
                if (this->state == State::FinWait2) {
                    /*
                    In addition to the processing for the ESTABLISHED state, if
                    the retransmission queue is empty, the user's CLOSE can be
                    acknowledged ("ok") but do not delete the TCB.
                    */
                    if (unacked.data.empty()) {
                        if (!tcp_h.fin()) std::cout << "the user's CLOSE is acknowledged" << std::endl;
                    }
                }
                if (this->state == State::Closing) {
                    if (this->send.una == send_closed_at + 1) {
                        // our FIN has been ACKed!
                        std::cout << "Our FIN has been ACKed" << std::endl;

                        this->change_state(State::TimeWait);
                        this->start_close_timer();
                    }
                }
            }
            // receive ack for out FIN
            if (this->state == State::LastAck) {
                if (this->send.una == send_closed_at + 1) {
                    // our FIN has been ACKed!
                    std::cout << "Our FIN has been ACKed" << std::endl;
                    delete_TCB();
                }
            }
            if (this->state == State::TimeWait) {
                this->start_close_timer();
                this->write(dev, this->send.nxt, 0);
            }
        }

        if (tcp_h.urg()) {
            /** TODO: implement later */
        }

        if (data_len > 0) {
            if ((state == State::Estab) || (state == State::FinWait1) || (state == State::FinWait2)) {
                uint32_t unread_data_at = this->recv.nxt - seqn;

                if (unread_data_at > (uint32_t)data_len) {
                    // we must have received a re-transmitted FIN that we ahve already seen
                    // nxt points to beyond the fin, but the fin is not in data
                    assert(unread_data_at == (uint32_t)(data_len + 1));
                    unread_data_at = 0;
                }

                /** TODO: refactor if needed **/
                incoming.enqueue(data, data_len);

                std::cout << "incoming data: ";
                const unsigned char *ptr = data;
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
                this->write(dev, this->send.nxt, 0);
            }
        }

        if (tcp_h.fin()) {
            std::cout << "FIN received" << std::endl;
            if ((state == State::Closed) || (state == State::Listen) || (state == State::SynSent)) return;

            ++this->recv.nxt;
            this->write(dev, this->send.nxt, 0);

            if ((this->state == State::SynRcvd) || (this->state == State::Estab))
                this->change_state(State::CloseWait);
            else if (this->state == State::FinWait1)
                this->change_state(State::TimeWait);
            else if (this->state == State::FinWait2) {
                this->change_state(State::TimeWait);
                start_close_timer();
            } else if (this->state == State::TimeWait)
                start_close_timer();
            else if ((this->state == State::CloseWait) || (this->state == State::Closing) || (this->state == State::LastAck))
                return;
            else
                std::cout << "error: FIN received in " << this->state << std::endl;
            return;
        }
    }
    return;
}

/** TODO: fix on_tick for sending syn-fin flags **/
void Connection::on_tick(struct device *dev) {
    if ((state == State::Closed) || (state == State::TimeWait) || (state == State::FinWait2)) {
        // we have shutdown our write side and the other side acked, no need to (re)transmit anything
        return;
    } else if (state == State::CloseWait) {
        this->close_send();
    }

    // nunacked_data and nunsent_data includes SYN and FIN flag
    uint32_t nunacked_data = this->send.nxt - this->send.una;
    uint32_t nunsent_data = nunacked_data > (uint32_t)this->unacked.data.size() ? 0 : (uint32_t)this->unacked.data.size() - nunacked_data;

    int64_t waited_for = 0;

    // key exists, there is a possibility of retransmission
    auto first_unacked_key = this->send.una;
    if (this->timers.send_times.count(first_unacked_key) == 1) {
        auto first_unacked_value = this->timers.send_times.at(this->send.una);
        waited_for = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - first_unacked_value).count();
    }

    /** RTO = min[UPPERBOUND,max[LOWERBOUND,(BETA*SRTT)]]
     * Beta is taken as 1.5
     * Upper limit is 1 min
     * Lower limit is 1 sec
     */
    bool should_retransmit = ([&]() {
        // nunacked data exist, check timers for retransmission
        if (nunacked_data > 0) {
            // 1 * 1000000 = 1 second
            // 60 * 1000000 = 60 second
            return ((waited_for > (1 * 1000000)) && (waited_for > (((int64_t)1.5) * this->timers.srtt))) || (waited_for > (60 * 1000000));
        }
        // nunacked data does not exist, we may need to send new data
        else {
            return false;
        }
    }());

    if (should_retransmit) {
        // we should retransmit!

        /** Resend syn packet */
        if (this->state == State::SynSent) {
            this->tcp.syn(true);
            this->write(dev, this->send.una, 0);
            return;
        }

        uint32_t send_limit = std::min((uint32_t)this->unacked.data.size(), (uint32_t)this->send.wnd);

        if (send_limit < this->send.wnd && this->send_closed) {
            std::cout << "Resend FIN" << std::endl;
            this->tcp.fin(true);
        } else if (send_limit == 0) {
            return;
        }

        this->write(dev, this->send.una, send_limit);
    } else {
        // we should send new data if have new data and space in the window
        if (nunsent_data == 0 && this->send_closed_at == 0) return;

        // sent FIN once, if transmitted before only retransmit
        if (this->send_closed_at == this->send.nxt) return;

        uint32_t allowed = this->send.wnd - nunacked_data;
        if (allowed == 0) return;

        uint32_t send_limit = std::min(nunsent_data, allowed);
        if (send_limit < allowed && this->send_closed) {
            this->tcp.fin(true);
        } else if (send_limit == 0) {
            return;
        }

        this->write(dev, this->send.nxt, send_limit);
    }
    return;
}

/**
 * TODO: fix flush for sending syn-fin flags
 */
void Connection::flush(struct device *dev) {
    uint32_t nunacked_data = (send_closed_at != 0 ? send_closed_at : this->send.nxt) - this->send.una;
    uint32_t nunsent_data = (uint32_t)this->unacked.data.size() - nunacked_data;

    // we should send new data if have new data and space in the window
    if (nunsent_data == 0 && this->send_closed_at != 0) {
        return;
    }

    uint32_t allowed = this->send.wnd - nunacked_data;
    if (allowed == 0) {
        return;
    }

    uint32_t send_data = std::min(nunacked_data + nunsent_data, allowed);
    if (send_data < allowed && this->send_closed && this->send_closed_at == 0) {
        this->tcp.fin(true);
        this->send_closed_at = this->send.una + (uint32_t)this->unacked.data.size();
    }

    if (send_data == 0) {
        return;
    };

    this->write(dev, this->send.una, send_data);
}

void Connection::write(struct device *dev, uint32_t seq, uint32_t limit) {
    const uint32_t buf_len = 1500;
    uint8_t buf[buf_len] = {};

    this->tcp.sequence_number = seq;
    this->tcp.acknowledgment_number = this->recv.nxt;

    uint32_t offset = seq - this->send.una;

    // we need two special case the two 'virtual' bytes SYN and FIN
    if (this->send_closed && (seq == this->send_closed_at + 1)) {
        // trying to write following FIN
        offset = 0;
        limit = 0;

        // std::cout << "AAAAAAAAAAAAAAAA" << std::endl;
        // std::cout << "this->tcp.syn(): " << this->tcp.syn() << std::endl;
        // std::cout << "this->tcp.ack(): " << this->tcp.ack() << std::endl;
        // std::cout << "this->tcp.fin(): " << this->tcp.fin() << std::endl;
        // std::cout << "AAAAAAAAAAAAAAAA" << std::endl;
    }

    this->ip.set_size(20);
    this->tcp.set_header_len(20);

    const uint32_t max_data = std::min(limit, (uint32_t)unacked.data.size());
    const uint32_t size = std::min(buf_len, (uint32_t)this->tcp.get_header_len() + (uint32_t)ip.get_size() + max_data);

    this->ip.payload_len = (uint16_t)size;

    uint8_t *unwritten = buf;

    this->ip.header_checksum = this->ip.compute_ipv4_checksum();
    this->ip.write_to_buff((char *)unwritten);
    uint32_t ip_header_ends_at = ip.get_size();

    // postpone writing the tcp header because we need the payload as one contiguous slice to calculate the tcp checksum
    uint32_t tcp_header_ends_at = ip_header_ends_at + this->tcp.get_header_len();

    uint32_t payload_bytes = ([&]() -> uint32_t {
        uint32_t written = 0;
        written += this->unacked.copy_from(unwritten + tcp_header_ends_at, max_data, offset);
        return written;
    }());

    uint32_t payload_ends_at = tcp_header_ends_at + payload_bytes;

    this->tcp.checksum = this->tcp.compute_tcp_checksum(this->ip, unwritten + tcp_header_ends_at, payload_bytes);
    this->tcp.write_to_buff((char *)(unwritten) + ip_header_ends_at);

    uint32_t next_seq = seq + payload_bytes;
    if (this->tcp.syn()) {
        ++next_seq;
        this->tcp.syn(false);
    }
    if (this->tcp.fin()) {
        ++next_seq;
        this->tcp.fin(false);
    }
    if (this->tcp.rst()) this->tcp.rst(false);

    if (wrapping_lt(this->send.nxt, next_seq)) {
        this->send.nxt = next_seq;
    }
    this->timers.send_times[seq] = std::chrono::high_resolution_clock::now();

    tuntap_write(dev, buf, payload_ends_at);
    return;
}

void Connection::send_rst(struct device *dev, const Net::Ipv4Header &ip_h, const Net::TcpHeader &tcp_h, int slen) {
    std::cout << "Sending Reset" << std::endl;

    this->tcp.rst(true);
    this->tcp.ack(false);

    if (this->state == State::Closed || !is_in_synchronized_state()) {
        this->ip.destination = ip_h.source;
        this->ip.source = ip_h.destination;
        this->tcp.destination_port = tcp_h.source_port;
        this->tcp.source_port = tcp_h.destination_port;
        this->tcp.window_size = 1024;

        // write <SEQ=SEG.ACK><CTL=RST>
        if (tcp_h.ack()) {
            this->tcp.sequence_number = tcp_h.acknowledgment_number;
        }
        // write <SEQ=0><ACK=SEG.SEQ+SEG.LEN><CTL=RST,ACK>
        else {
            this->tcp.sequence_number = 0;
            this->tcp.acknowledgment_number = tcp_h.sequence_number + slen;
            this->tcp.ack(true);
        }
        this->write(dev, this->tcp.sequence_number, 0);

        delete_TCB();

        return;
    } else {
        this->tcp.acknowledgment_number = this->send.nxt;
        this->write(dev, this->tcp.sequence_number, 0);
        this->tcp.ack(true);
        return;
    }

    return;
}

bool Connection::is_in_synchronized_state() const { return !((state == State::Listen) || (state == State::SynRcvd) || (state == State::SynSent)); }

// first check that sequence numbers are valid (RFC 793 S3.3)
//
// valid segment check okay if it acks at least one byte, which means that at least one of the following is true
// RCV.NXT =< SEG.SEQ < RCV.NXT + RCV.WND
// RCV.NXT =< SEG.SEQ + SEG.LEN-1 < RCV.NXT + RCV.WND
//
bool Connection::sequence_number_check(uint32_t slen, uint32_t seqn, uint32_t wend) const {
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
};

//-------------------------------------------------------

/** TODO: implement if needed */
void Connection::availability() const { return; }

bool Connection::is_rcv_closed() const {
    return (state == State::TimeWait) || (state == State::CloseWait) || (state == State::LastAck) || (state == State::Closed) || (state == State::Closing)
               ? true
               : false;
}

bool Connection::is_snd_closed() const {
    return (state == State::FinWait1) || (state == State::FinWait2) || (state == State::Closing) || (state == State::LastAck) || (state == State::Closed)
               ? true
               : false;
}

void Connection::send_data(std::string &data) {
    if ((this->state == State::Closed))
        std::cout << "error: connection does not exist" << std::endl;
    else if ((this->state == State::Listen))
        std::cout << "error: foreign socket unspecified (not implemented)" << std::endl;
    else if ((this->state == State::SynRcvd) || (this->state == State::SynSent))
        std::cout << "error: insufficient resources" << std::endl;
    else if ((this->state == State::Estab) || (this->state == State::CloseWait))
        this->unacked.enqueue(reinterpret_cast<uint8_t *>(&data[0]), (int)data.length());
    else if ((this->state == State::FinWait1) || (this->state == State::FinWait2) || (this->state == State::Closing) || (this->state == State::LastAck) ||
             (this->state == State::TimeWait))
        std::cout << "error: connection closing" << std::endl;
}
