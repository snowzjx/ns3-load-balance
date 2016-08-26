/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef CONGESTION_PROBING_H
#define CONGESTION_PROBING_H

#include "ns3/object.h"
#include "ns3/callback.h"
#include "ns3/traced-callback.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include <vector>

namespace ns3 {

// In the congestion probing module, the packet is labeled with (flowId ^ V)
// and go through the per-flow ECMP based router which determines the path based
// on flow label.

// The V is ranging from 0 to MaxV

class Packet;
class Socket;
class Node;
class Ipv4Header;

class CongestionProbing : public Object
{
public:

    static TypeId GetTypeId (void);
    virtual TypeId GetInstanceTypeId (void) const;

    CongestionProbing ();
    CongestionProbing (const CongestionProbing&);
    ~CongestionProbing ();
    virtual void DoDispose (void);

    void SetSourceAddress (Ipv4Address address);
    void SetProbeAddress (Ipv4Address address);
    void SetFlowId (uint32_t flowId);
    void SetNode (Ptr<Node> node);

    void DoStartProbe ();
    void StartProbe ();
    void DoStopProbe ();
    void StopProbe (Time time);

    void ProbeEvent ();

    void ReceivePacket (Ptr<Socket> socket);

    typedef void (*ProbingCallback)
        (uint32_t v, Ptr<Packet> packet, Ipv4Header header, Time rtt, bool isCE);

private:

    // Variables
    uint32_t m_V;   // The current V and V follows a round robin fashion (0 - MaxV)

    EventId m_probeEvent;

    // Parameters
    Ipv4Address m_sourceAddress;
    Ipv4Address m_probeAddress; // The flow destination
    uint32_t m_flowId; // Flow Id of the flow being probed
    Ptr<Node> m_node;

    uint32_t m_maxV;
    Time m_probeInterval;

    Ptr<Socket> m_socket;

    TracedCallback <uint32_t, Ptr<Packet>, Ipv4Header ,Time, bool> m_probingCallback;
};

}

#endif /* CONGESTION_PROBING_H */

