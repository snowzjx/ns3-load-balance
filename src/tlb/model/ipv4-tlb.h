/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef IPV4_TLB_H
#define IPV4_TLB_H

#include "ns3/object.h"
#include "ns3/ipv4-address.h"
#include "tlb-flow-info.h"
#include "tlb-path-info.h"

#include <vector>
#include <map>
#include <utility>

namespace ns3 {

enum PathType
{
    GoodPath,
    BadPath,
    GreyPath
};

class Node;

class Ipv4TLB : public Object
{

public:

    Ipv4TLB ();

    Ipv4TLB (const Ipv4TLB&);

    static TypeId GetTypeId (void);

    void AddAddressWithTor (Ipv4Address address, uint32_t torId);

    // These methods are used for TCP flows

    uint32_t GetPath (uint32_t flowId, Ipv4Address daddr);

    void FlowRecv (uint32_t flowId, uint32_t path, Ipv4Address daddr, uint32_t size, bool withECN, Time rtt);

    void FlowSend (uint32_t flowId, Ipv4Address daddr, uint32_t path, uint32_t size);

    void FlowRetransmission (uint32_t flowId, Ipv4Address daddr, uint32_t path, uint32_t size);

    void FlowTimeout (uint32_t flowId, Ipv4Address daddr, uint32_t path);

    // These methods are used in probing

    uint32_t GetProbingPath (Ipv4Address daddr);

    void ProbeRecv (uint32_t path, Ipv4Address daddr, uint32_t size, bool withECN, Time rtt);

    void ProbeTimeout (uint32_t path, Ipv4Address daddr);

    // Node
    void SetNode (Ptr<Node> node);

private:

    void PacketReceive (uint32_t flowId, uint32_t path, uint32_t destTorId,
                        uint32_t size, bool withECN, Time rtt, bool isProbing);

    bool UpdateFlowInfo (uint32_t flowId, uint32_t path, uint32_t size, bool withECN);

    void UpdatePathInfo (uint32_t destTor, uint32_t path, uint32_t size, bool withECN, Time rtt);

    bool TimeoutFlow (uint32_t flowId, uint32_t path);

    void TimeoutPath (uint32_t destTor, uint32_t path, bool isProbing);

    bool SendFlow (uint32_t flowId, uint32_t path, uint32_t size);

    bool RetransFlow (uint32_t flowId, uint32_t path, uint32_t size, bool &needRetranPath);

    void RetransPath (uint32_t destTor, uint32_t path);

    void UpdateFlowPath (uint32_t flowId, uint32_t path);

    void AssignFlowToPath (uint32_t flowId, uint32_t destTor, uint32_t path);

    void RemoveFlowFromPath (uint32_t flowId, uint32_t destTor, uint32_t path);

    bool WhereToChange (uint32_t destTor, uint32_t &newPath);

    uint32_t SelectRandomPath (uint32_t destTor);

    PathType JudgePath (uint32_t destTor, uint32_t path);

    bool FindTorId (Ipv4Address daddr, uint32_t &destTorId);

    void PathAging (void);

    // Parameters
    uint32_t m_S;

    uint32_t m_K;

    Time m_minRtt;

    uint32_t m_ecnSampleMin;

    double m_ecnPortionLow;

    double m_ecnPortionHigh;

    double m_flowRetrasPortion;

    // Variables
    std::map<uint32_t, TLBFlowInfo> m_flowInfo; /* <FlowId, TLBFlowInfo> */
    std::map<std::pair<uint32_t, uint32_t>, TLBPathInfo> m_pathInfo; /* <DestTorId, PathId>, TLBPathInfo> */

    std::map<Ipv4Address, uint32_t> m_ipTorMap; /* <DestAddress, DestTorId> */

    std::map<uint32_t, std::vector<uint32_t> > m_availablePath; /* <DestTorId, List<PathId>> */

    std::map<uint32_t, Ipv4Address> m_probingAgent; /* <DestTorId, ProbingAgentAddress>*/

    Ptr<Node> m_node;
};

}

#endif /* TLB_H */

