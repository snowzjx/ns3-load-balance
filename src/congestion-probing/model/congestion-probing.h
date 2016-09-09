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
#include <map>

namespace ns3 {

// In the congestion probing module, the packet is labeled with (flowId ^ V)
// and go through the per-flow ECMP based router which determines the path based
// on flow label.

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
    void SetPathId (uint32_t pathId);
    void SetNode (Ptr<Node> node);

    void DoStartProbe ();
    void StartProbe ();
    void DoStopProbe ();
    void StopProbe (Time time);

    void ProbeEvent ();

    void ProbeEventTimeout(uint32_t id);

    void ReceivePacket (Ptr<Socket> socket);

    typedef void (*ProbingCallback)
        (uint32_t pathId, Ptr<Packet> packet, Ipv4Header header, Time rtt, bool isCE);

    typedef void (*ProbingTimeoutCallback)
        (uint32_t pathId);

private:

    EventId m_probeEvent;

    std::map <uint32_t, EventId> m_probingTimeoutMap;

    uint32_t m_id;

    Ptr<Socket> m_socket;

    // Parameters
    Ipv4Address m_sourceAddress;
    Ipv4Address m_probeAddress; // The flow destination
    uint32_t m_pathId; // Path Id of the flow being probed
    Ptr<Node> m_node;

    Time m_probeInterval;

    Time m_probeTimeout;

    // Trace source
    TracedCallback <uint32_t, Ptr<Packet>, Ipv4Header ,Time, bool> m_probingCallback;

    TracedCallback <uint32_t> m_probingTimeoutCallback;
};

}

#endif /* CONGESTION_PROBING_H */

