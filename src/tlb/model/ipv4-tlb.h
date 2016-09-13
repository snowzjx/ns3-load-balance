/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef IPV4_TLB_H
#define IPV4_TLB_H

#include "ns3/object.h"
#include "ns3/ipv4-address.h"
#include "tlb-flow-info.h"
#include "tlb-path-info.h"

#include <vector>
#include <map>


namespace ns3 {

class Node;

class Ipv4TLB : public Object
{

public:

    Ipv4TLB ();

    Ipv4TLB (const Ipv4TLB&);

    static TypeId GetTypeId (void);

    void AddAddressWithTor (Ipv4Address address, uint32_t torId);

    // These methods are used for TCP flows

    uint32_t GetPath (uint32_t flowId);

    void FlowRecv (uint32_t flowId, Ipv4Address daddr, uint32_t size, bool withECN);

    void FlowRetransmission (uint32_t flowId);

    void FlowTimeout (uint32_t flowId);

    // These methods are used in probing
    void ProbeRecv (uint32_t pathId, Ipv4Address daddr, uint32_t size, bool withECN);

    void ProbeTimeout (uint32_t pathId);

    // Node
    void SetNode (Ptr<Node> node);

private:

    // Parameters
    uint32_t m_S;

    // Variables
    std::map<uint32_t, TLBFlowInfo> m_flowInfo; /* <FlowId, TLBFlowInfo> */
    std::map<uint32_t, std::map<uint32_t, TLBPathInfo> > m_pathInfo; /* <DestTorId, Map<PathId, TLBPathInfo>> */
    std::map<Ipv4Address, uint32_t> m_ipTorMap; /* <DestAddress, DestTorId> */

    std::map<uint32_t, std::vector<uint32_t> > m_availablePath; /* <DestTorId, List<PathId>> */

    std::map<uint32_t, Ipv4Address> m_probingAgent; /* <DestTorId, ProbingAgentAddress>*/

    Ptr<Node> m_node;
};

}

#endif /* TLB_H */

