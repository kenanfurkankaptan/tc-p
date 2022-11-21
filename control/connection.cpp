#include "connection.h"

// generates connection with incoming packets
void Connection::accept(struct device *dev, Net::Ipv4Header &ip_h, Net ::TcpHeader &tcp_h) {
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
        return temp_tcp;
    }());
    this->incoming = Queue();
    this->unacked = Queue();
    this->closed = false;
    this->closed_at = 0;
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
    uint32_t seqn = tcp_h.sequence_number;            // sequence number
    uint32_t ackn = tcp_h.acknowledgment_number;      // ack number
    uint32_t wend = this->recv.nxt + this->recv.wnd;  // window end

    int slen = data_len;
    if (tcp_h.fin()) {
        slen += 1;
    };
    if (tcp_h.syn()) {
        slen += 1;
    };

    /* STATES */
    if (this->state == State::Closed) {
        if (tcp_h.rst()) {
            return;
        } else {
            return send_rst(dev);
        }
    } else if (this->state == State::Listen) {
        // we only expect syn packet
        if (tcp_h.rst())
            return;
        else if (tcp_h.ack())
            return send_rst(dev);

        else if (tcp_h.syn())
            /** TODO: check security/compartment and SEG.PRC */
            return this->accept(dev, ip_h, tcp_h);
        else
            return;
    } else if (this->state == State::SynSent) {
        bool is_ack_acceptable = NULL;

        if (tcp_h.ack()) {
            /** If SEG.ACK =< ISS, or SEG.ACK > SND.NXT, send a reset */
            if ((tcp_h.acknowledgment_number <= send.iss) || (tcp_h.acknowledgment_number > send.nxt)) return send_rst(dev);

            /** If SND.UNA =< SEG.ACK =< SND.NXT then the ACK is acceptable. */
            is_ack_acceptable = is_between_wrapped(this->send.una - 1, ackn, this->send.nxt + 1);
        }
        if (tcp_h.rst()) {
            if (is_ack_acceptable) {
                std::cout << "error: connection reset " << std::endl;
                /** TODO: handle closed state */
                this->state = State::Closed;
            } else {
                return;
            }
        }
        /** TODO: check security/compartment and SEG.PRC */

        if (is_ack_acceptable != (true || NULL)) return;
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
                this->tcp.syn(true);
                return this->write(dev, this->send.iss, 0);
            }
        }
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
                /** TODO: remove retransmission queue */
                /*
                If this connection was initiated with a passive OPEN (i.e.,
                came from the LISTEN state), then return this connection to
                LISTEN state and return.
                If this connection was initiated with an active OPEN (i.e., came
                from SYN-SENT state) then the connection was refused, enter CLOSED
                state and return.
                */
                this->state = State::Listen;

            } else if ((state == State::Estab) || (state == State::FinWait1) || (state == State::FinWait2) || (state == State::CloseWait)) {
                /** TODO: flush all queue */
                this->state = State::Closed;
            } else if ((state == State::Closing) || (state == State::LastAck) || (state == State::TimeWait)) {
                this->state = State::Closed;
            }
        }

        /** TODO: check security/compartment and SEG.PRC */

        if (tcp_h.syn()) {
            this->send_rst(dev);

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
                    this->state = State::Estab;
                } else {
                    /** TODO: RST : <SEQ=SEH.ACK><CTL=RST> */
                }
            }
            if ((state == State::Estab) || (state == State::FinWait1) || (state == State::FinWait2) || (state == State::CloseWait) || (state == State::Closing)) {
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
                                    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - sent).count();
                                    this->timers.srtt = 0.8 * this->timers.srtt + (1.0 - 0.8) * elapsed;
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

                    // std::cout << "this->send.nxt: " << this->send.nxt << std::endl;
                    // std::cout << "ackn: " << ackn << std::endl;

                    this->send.una = ackn;
                    /** TODO: if unacked empty and waiting flush, notify */
                    /** TODO: update window */
                }

                if (this->state == State::FinWait1) {
                    // receive ack for out FIN
                    if (this->send.una == closed_at + 1) {
                        // our FIN has been ACKed!
                        this->state = State::FinWait2;
                    }
                }
                if (this->state == State::FinWait2) {
                    /** TODO: ??? */
                    /*
                    In addition to the processing for the ESTABLISHED state, if
                    the retransmission queue is empty, the user's CLOSE can be
                    acknowledged ("ok") but do not delete the TCB.
                    */
                }
                if (this->state == State::Closing) {
                    if (this->send.una == closed_at + 1) {
                        // our FIN has been ACKed!
                        this->state = State::TimeWait;
                    }
                }
            }
            // receive ack for out FIN
            if (this->state == State::LastAck) {
                if (this->send.una == closed_at + 1) {
                    // our FIN has been ACKed!
                    this->state = State::Closed;
                }
            }
            if (this->state == State::TimeWait) {
                /** TODO: ?? */
            }
        }

        if (tcp_h.urg()) {
            /** TODO: implement */
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
                this->write(dev, this->send.nxt, 0);
            }
        }

        if (tcp_h.fin()) {
            if ((state == State::Closed) || (state == State::Listen) || (state == State::SynSent)) return;

            ++this->recv.nxt;
            this->write(dev, this->send.nxt, 0);

            if ((this->state == State::SynRcvd) || (this->state == State::Estab))
                this->state = State::CloseWait;
            else if (this->state == (State::FinWait1))
                this->state = State::TimeWait;
            else if (this->state == (State::FinWait2))
                this->state = State::TimeWait;
            else if (this->state == (State::CloseWait))
                this->state = State::CloseWait;

            return;
        }
    }
    return;
}

void Connection::on_tick(struct device *dev) {
    if ((state == State::Closed) || (state == State::TimeWait) || (state == State::FinWait2)) {
        // we have shutdown our write side and the other side acked, no need to (re)transmit anything
        return;
    }

    uint32_t nunacked_data = (closed_at != 0 ? closed_at : this->send.nxt) - this->send.una;
    uint32_t nunsent_data = this->unacked.data.size() - nunacked_data;

    // std::cout << "this->send.nxt: " << this->send.nxt << std::endl;
    // std::cout << "this->send.una: " << this->send.una << std::endl;

    // std::cout << "nunacked_data: " << nunacked_data << std::endl;
    // std::cout << "nunsent_data: " << nunsent_data << std::endl;

    /** TODO: TEST */
    auto temp66 = *(std::next(this->timers.send_times.begin(), this->send.una));
    uint64_t waited_for = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - temp66.second).count();
    bool should_retransmit = ([&]() {
        return (waited_for > (uint64_t)std::chrono::microseconds(1000000).count()) && (waited_for * (std::chrono::system_clock::period::num / std::chrono::system_clock::period::den) > 1.5 * this->timers.srtt);
    }());

    if (should_retransmit) {
        // we should retransmit!
        uint32_t send = std::min((uint32_t)this->unacked.data.size(), (uint32_t)this->send.wnd);

        if (send < this->send.wnd && this->closed) {
            // can we include the FIN?
            this->tcp.fin(true);
            this->closed_at = this->send.una + this->unacked.data.size();
        }

        if (send == 0) {
            return;
        };

        this->write(dev, this->send.una, send);
    } else {
        // we should send new data if have new data and space in the window
        if (nunsent_data == 0 && this->closed_at != 0) {
            return;
        }

        uint32_t allowed = this->send.wnd - nunacked_data;
        if (allowed == 0) {
            return;
        }

        uint32_t send = std::min(nunsent_data, allowed);
        if (send < allowed && this->closed && this->closed_at == 0) {
            this->tcp.fin(true);
            this->closed_at = this->send.una + this->unacked.data.size();
        }

        if (send == 0) {
            return;
        };

        // std::cout << "this->send.nxt" << std::endl;
        // std::cout << this->send.nxt << std::endl;
        // std::cout << "this->send.una" << std::endl;
        // std::cout << this->send.una << std::endl;

        // std::cout << "this->unacked.data.size()" << std::endl;
        // std::cout << this->unacked.data.size() << std::endl;

        // std::cout << "nunacked_data" << std::endl;
        // std::cout << nunacked_data << std::endl;
        // std::cout << "nunsent_data" << std::endl;
        // std::cout << nunsent_data << std::endl;

        // std::cout << "send" << std::endl;
        // std::cout << send << std::endl;
        // std::cout << "allowed" << std::endl;
        // std::cout << allowed << std::endl;

        this->write(dev, this->send.nxt, send);
    }

    // if FIN, enter FIN-WAIT-1
    return;
}

void Connection::write(struct device *dev, uint32_t seq, uint32_t limit) {
    const uint32_t buf_len = 1500;
    uint8_t buf[buf_len] = {};

    this->tcp.sequence_number = seq;
    this->tcp.acknowledgment_number = this->recv.nxt;

    uint32_t offset = seq - this->send.una;

    // we need two special case the two 'virtual' bytes SYN and FIN
    if (this->closed && (seq == this->closed_at + 1)) {
        // trying to write following FIN
        offset = 0;
        limit = 0;
    }

    this->ip.set_size(20);
    this->tcp.set_header_len(20);

    const uint32_t max_data = std::min(limit, (uint32_t)unacked.data.size());
    const uint32_t size = std::min((uint32_t)buf_len, this->tcp.get_header_len() + ip.get_size() + max_data);

    this->ip.payload_len = size;

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
    if (wrapping_lt(this->send.nxt, next_seq)) {
        this->send.nxt = next_seq;
    }
    this->timers.send_times.insert({seq, std::chrono::high_resolution_clock::now()});

    tuntap_write(dev, buf, payload_ends_at);
    return;
}

void Connection::send_rst(struct device *dev) {
    std::cout << "Send Reset" << std::endl;

    this->tcp.rst(true);
    // TODO: fix sequence numbers here
    // If the incoming segment has an ACK field, the reset takes its
    // sequence number from the ACK field of the segment, otherwise the
    // reset has sequence number zero and the ACK field is set to the sum
    // of the sequence number and segment length of the incoming segment.
    // The connection remains in the same state.
    //
    // TODO: handle synchronized RST
    // 3.  If the connection is in a synchronized state (ESTABLISHED,
    // FIN-WAIT-1, FIN-WAIT-2, CLOSE-WAIT, CLOSING, LAST-ACK, TIME-WAIT),
    // any unacceptable segment (out of window sequence number or
    // unacceptible acknowledgment number) must elicit only an empty
    // acknowledgment segment containing the current send-sequence number
    // and an acknowledgment indicating the next sequence number expected
    // to be received, and the connection remains in the same state.
    this->tcp.sequence_number = 0;
    this->tcp.acknowledgment_number = 0;
    this->write(dev, this->send.nxt, 0);
    return;
}

bool Connection::is_in_synchronized_state() {
    return (state != State::SynRcvd) || (state != State::SynSent);
}

// first check that sequence numbers are valid (RFC 793 S3.3)
//
// valid segment check okay if it acks at least one byte, which means that at least one of the following is true
// RCV.NXT =< SEG.SEQ < RCV.NXT + RCV.WND
// RCV.NXT =< SEG.SEQ + SEG.LEN-1 < RCV.NXT + RCV.WND
//
bool Connection::sequence_number_check(int slen, int seqn, int wend) {
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
void Connection::availability() {
    return;
}

bool Connection::is_rcv_closed() {
    return (state == State::TimeWait) || (state == State::CloseWait) || (state == State::LastAck) || (state == State::Closed) || (state == State::Closing) ? true : false;
}

bool Connection::is_snd_closed() {
    return (state == State::FinWait1) || (state == State::FinWait2) || (state == State::Closing) || (state == State::LastAck) || (state == State::Closed) ? true : false;
}
