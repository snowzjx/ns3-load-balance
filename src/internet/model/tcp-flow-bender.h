/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef TCP_FLOW_BENDER
#define TCP_FLOW_BENDER

#include "ns3/object.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"

namespace ns3 {

class TcpFlowBender : public Object
{
public:
    static TypeId GetTypeId (void);

    TcpFlowBender ();
    TcpFlowBender (const TcpFlowBender &other);
    ~TcpFlowBender ();

    virtual void DoDispose (void);

    void ReceivedPacket ();
    void ReceivedMarkedPacket ();

    uint32_t GetV ();

private:
    void CheckEvent ();

    // Variables
    uint32_t m_totalPackets;
    uint32_t m_markedPackets;

    uint32_t m_numCongestionRtt;
    uint32_t m_V;

    EventId m_checkEvent;

    // Parameters
    Time m_rtt;
    double m_T;
    uint32_t m_N;

    // Used for statistics
    uint32_t m_totalPacketsStatis;
    uint32_t m_markedPacketsStatis;
};

}

#endif
