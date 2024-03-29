
#ifndef CONNECTION_H
#define CONNECTION_H

#include <assert.h>

#include <algorithm>
#include <array>
#include <bitset>
#include <iostream>
#include <map>
#include <queue>
#include <vector>

#include "../net/ip_header/ip_header.h"
#include "../net/tcp_header/tcp_header.h"
#include "../util/queue/queue.h"
#include "../util/utility.h"
#include "tuntap++.hh"
#include "tuntap.h"

enum State {
    Listen,
    SynSent,
    SynRcvd,
    Estab,
    FinWait1,
    FinWait2,
    CloseWait,
    LastAck,
    Closing,
    TimeWait,
    Closed,
};

/// State of the Send Sequence Space (RFC 793 S3.2 Figure4)
///
/// ```
///
///      1         2          3          4
/// ----------|----------|----------|----------
///        SND.UNA    SND.NXT    SND.UNA
///                             + SND.WND
///
/// 1 - old sequence numbers which have been acknowledged
/// 2 - sequence numbers of unacknowledged data
/// 3 - sequence numbers allowed for new data transmission
/// 4 - future sequence numbers which are not yet allowed
/// ```
struct SendSequenceSpace {
    // send unacknowledged
    uint32_t una;
    // send next
    uint32_t nxt;
    // send window
    uint16_t wnd;
    // send urgent pointer
    bool up;
    // segment sequence number used for last window update
    uint64_t wl1;
    // segment acknowledgment number used for last window update
    uint64_t wl2;
    // initial send sequence number
    uint32_t iss;
};

/// State of the Receive Sequence Space (RFC 793 S3.2 Figure5)
///
/// ```
///
///      1          2          3
/// ----------|----------|----------
///        RCV.NXT    RCV.NXT
///                  + RCV.WND
///
/// 1 - old sequence numbers which have been acknowledged
/// 2 - sequence numbers allowed for new reception
/// 3 - future sequence numbers which are not yet allowed
/// ```
struct RecvSequenceSpace {
    // receive next
    uint32_t nxt;
    // receive window
    uint16_t wnd;
    //  receive urgent pointer
    bool up;
    // initial receive sequence number
    uint32_t irs;
};

struct Timer {
    int64_t srtt;  // Smoothed Round Trip Time
    std::map<uint32_t, std::chrono::_V2::system_clock::time_point> send_times;
};

/** TODO: Don't mix public and private data members. getters that returns reference could be added to class */ 
class Connection {
   public:
    /*
    A copy or move assignment operator cannot be automatically generated
    by the compiler for classes with data members that are arrays
    (which are not copy- or move-assignable) so you must define your own.
    */
    Connection() = default;

    void connect(struct device *dev, uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port);
    void on_packet(struct device *dev, const Net::Ipv4Header &ip_h, const Net::TcpHeader &tcp_h, uint8_t *data, int data_len);
    void on_tick(struct device *dev);
    void send_data(std::string &data);
    void close_send();
    void check_close_timer();

    inline bool is_connection_closed() {
        std::scoped_lock<std::mutex> lock(lockMutex);
        return this->connection_closed;
    }


   private:
    std::mutex lockMutex;

    State state = State::Listen;
    State previous_state = State::Closed;

    Net::Ipv4Header ip;
    Net::TcpHeader tcp;


    inline void change_state(State new_state) {
        this->previous_state = this->state;
        this->state = new_state;
    }

    SendSequenceSpace send;
    RecvSequenceSpace recv;

    Timer timers;
    Queue incoming;
    Queue unacked;  // unacked contains both sent and unsent data

    bool is_send_closed;
    uint32_t send_closed_at = 0;  // == 0 means it is not set

    bool connection_closed;
    std::chrono::_V2::system_clock::time_point connection_close_start_time;



    void accept(struct device *dev, const Net::Ipv4Header &ip_h, const Net::TcpHeader &tcp_h);
    void delete_TCB();
    
    void start_close_timer();
    
    void flush(struct device *dev);

    void write(struct device *dev, uint32_t seq, uint32_t limit);
    void send_rst(struct device *dev, const Net::Ipv4Header &ip_h, const Net::TcpHeader &tcp_h, int slen);

    bool sequence_number_check(uint32_t slen, uint32_t seqn, uint32_t wend) const;

    bool is_in_synchronized_state() const;
    bool is_rcv_closed() const;
    bool is_snd_closed() const;

};

#endif /* CONNECTION_H */
