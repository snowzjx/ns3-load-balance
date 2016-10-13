/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef TCP_FLOW_BENDER
#define TCP_FLOW_BENDER

#include "ns3/object.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/sequence-number.h"

namespace ns3 {

class TcpFlowBender : public Object
{
public:
    static TypeId GetTypeId (void);

    TcpFlowBender ();
    TcpFlowBender (const TcpFlowBender &other);
    ~TcpFlowBender ();

    virtual void DoDispose (void);

    void ReceivedPacket (SequenceNumber32 higTxhMark, SequenceNumber32 ackNumber, uint32_t ackedBytes, bool withECE);

    uint32_t GetV ();

    Time GetPauseTime ();

private:

    void CheckCongestion ();

    // Variables
    uint32_t m_totalBytes;
    uint32_t m_markedBytes;

    uint32_t m_numCongestionRtt;
    uint32_t m_V;

    SequenceNumber32 m_highTxMark;

    // Parameters
    Time m_rtt;
    double m_T;
    uint32_t m_N;
};

}

#endif
