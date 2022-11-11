
#ifndef CONNECTION_H
#define CONNECTION_H

#include <assert.h>

#include <algorithm>
#include <array>
#include <bitset>
#include <iostream>
#include <queue>
#include <vector>

#include "ip_header.h"
#include "queue.h"
#include "tcp_header.h"
#include "tuntap++.hh"
#include "tuntap.h"
#include "utility.h"

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

class Connection {
   public:
    State state = Listen;
    SendSequenceSpace send;
    RecvSequenceSpace recv;
    Net::Ipv4Header ip;
    Net::TcpHeader tcp;
    // Timer timers;        /** TODO: implement timers **/
    Queue incoming;
    Queue unacked;  // unacked contains both sent and unsent data
    bool closed;
    uint32_t closed_at;

    /*
    A copy or move assignment operator cannot be automatically generated
    by the compiler for classes with data members that are arrays
    (which are not copy- or move-assignable) so you must define your own.
    */
    Connection() = default;
    // Connection(Connection &c);

    void accept(struct device *dev, Net::Ipv4Header &ip_h, Net::TcpHeader &tcp_h);
    void on_packet(struct device *dev, Net::Ipv4Header &ip_h, Net::TcpHeader &tcp_h, uint8_t *data, int data_len);
    void write(struct device *dev, uint32_t seq, uint32_t limit);

    /** TODO: define or delete functions below **/
    void availability();
};

#endif /* CONNECTION_H */
