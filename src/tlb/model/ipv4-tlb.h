/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef IPV4_TLB_H
#define IPV4_TLB_H

#include "ns3/object.h"
#include "ns3/ipv4-address.h"
#include "ns3/event-id.h"
#include "tlb-flow-info.h"
#include "tlb-path-info.h"

#include <vector>
#include <map>
#include <utility>

namespace ns3 {

enum PathType {
    GoodPath,
    GreyPath,
    BadPath,
    FailPath
};

struct PathInfo {
    PathType pathType;
    Time rttMin;
    double ecnPortion;
    uint32_t counter;
};

class Node;

class Ipv4TLB : public Object
{

public:

    Ipv4TLB ();

    Ipv4TLB (const Ipv4TLB&);

    static TypeId GetTypeId (void);

    void AddAddressWithTor (Ipv4Address address, uint32_t torId);

    void AddAvailPath (uint32_t destTor, uint32_t path);

    std::vector<uint32_t> GetAvailPath (Ipv4Address daddr);

    // These methods are used for TCP flows
    uint32_t GetPath (uint32_t flowId, Ipv4Address daddr);

    void FlowRecv (uint32_t flowId, uint32_t path, Ipv4Address daddr, uint32_t size, bool withECN, Time rtt);

    void FlowSend (uint32_t flowId, Ipv4Address daddr, uint32_t path, uint32_t size, bool isRetrasmission);

    void FlowTimeout (uint32_t flowId, Ipv4Address daddr, uint32_t path);

    void FlowFinish (uint32_t flowId, Ipv4Address daddr, uint32_t path);

    // These methods are used in probing
    void ProbeSend (Ipv4Address daddr, uint32_t path);

    void ProbeRecv (uint32_t path, Ipv4Address daddr, uint32_t size, bool withECN, Time rtt);

    void ProbeTimeout (uint32_t path, Ipv4Address daddr);

    // Node
    void SetNode (Ptr<Node> node);

private:

    void PacketReceive (uint32_t flowId, uint32_t path, uint32_t destTorId,
                        uint32_t size, bool withECN, Time rtt, bool isProbing);

    bool UpdateFlowInfo (uint32_t flowId, uint32_t path, uint32_t size, bool withECN);

    TLBPathInfo GetInitPathInfo (uint32_t path);

    void UpdatePathInfo (uint32_t destTor, uint32_t path, uint32_t size, bool withECN, Time rtt);

    bool TimeoutFlow (uint32_t flowId, uint32_t path, bool &isVeryTimeout);

    void TimeoutPath (uint32_t destTor, uint32_t path, bool isProbing, bool isVeryTimeout);

    bool SendFlow (uint32_t flowId, uint32_t path, uint32_t size);

    bool RetransFlow (uint32_t flowId, uint32_t path, uint32_t size, bool &needRetranPath, bool &needHighRetransPath);

    void RetransPath (uint32_t destTor, uint32_t path, bool needHighRetransPath);

    void UpdateFlowPath (uint32_t flowId, uint32_t path);

    void AssignFlowToPath (uint32_t flowId, uint32_t destTor, uint32_t path);

    void RemoveFlowFromPath (uint32_t flowId, uint32_t destTor, uint32_t path);

    bool WhereToChange (uint32_t destTor, uint32_t &newPath, bool hasOldPath, uint32_t oldPath);

    uint32_t SelectRandomPath (uint32_t destTor);

    struct PathInfo JudgePath (uint32_t destTor, uint32_t path);

    bool PathLIsBetterR (struct PathInfo pathL, struct PathInfo pathR);

    bool FindTorId (Ipv4Address daddr, uint32_t &destTorId);

    void PathAging (void);

    // Parameters
    uint32_t m_S;

    Time m_T;

    uint32_t m_K;

    Time m_T1;

    Time m_T2;

    Time m_agingCheckTime;

    Time m_minRtt;

    uint32_t m_ecnSampleMin;

    double m_ecnPortionLow;

    double m_ecnPortionHigh;

    uint32_t m_flowRetransHigh;
    uint32_t m_flowRetransVeryHigh;

    uint32_t m_flowTimeoutCount;

    double m_betterPathEcnThresh;

    Time m_betterPathRttThresh;

    uint32_t m_pathChangePoss;

    // Variables
    std::map<uint32_t, TLBFlowInfo> m_flowInfo; /* <FlowId, TLBFlowInfo> */
    std::map<std::pair<uint32_t, uint32_t>, TLBPathInfo> m_pathInfo; /* <DestTorId, PathId>, TLBPathInfo> */

    std::map<Ipv4Address, uint32_t> m_ipTorMap; /* <DestAddress, DestTorId> */

    std::map<uint32_t, std::vector<uint32_t> > m_availablePath; /* <DestTorId, List<PathId>> */

    std::map<uint32_t, Ipv4Address> m_probingAgent; /* <DestTorId, ProbingAgentAddress>*/

    EventId m_agingEvent;

    Ptr<Node> m_node;
};

}

#endif /* TLB_H */

