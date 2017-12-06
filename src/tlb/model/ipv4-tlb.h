/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef IPV4_TLB_H
#define IPV4_TLB_H

#include "ns3/object.h"
#include "ns3/callback.h"
#include "ns3/traced-value.h"
#include "ns3/ipv4-address.h"
#include "ns3/data-rate.h"
#include "ns3/event-id.h"
#include "tlb-flow-info.h"
#include "tlb-path-info.h"

#include <vector>
#include <map>
#include <utility>
#include <string>

#define TLB_RUNMODE_COUNTER 0
#define TLB_RUNMODE_MINRTT 1
#define TLB_RUNMODE_RANDOM 2
#define TLB_RUNMODE_RTT_COUNTER 11
#define TLB_RUNMODE_RTT_DRE 12

namespace ns3 {

enum PathType {
    GoodPath,
    GreyPath,
    BadPath,
    FailPath,
};

struct PathInfo {
    uint32_t pathId;
    PathType pathType;
    Time rttMin;
    uint32_t size;
    double ecnPortion;
    uint32_t counter;
    uint32_t quantifiedDre;
};

struct TLBAcklet {
    uint32_t pathId;
    Time activeTime;
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
    uint32_t GetPath (uint32_t flowId, Ipv4Address saddr, Ipv4Address daddr);

    uint32_t GetAckPath (uint32_t flowId, Ipv4Address saddr, Ipv4Address daddr);

    Time GetPauseTime (uint32_t flowId);

    void FlowRecv (uint32_t flowId, uint32_t path, Ipv4Address daddr, uint32_t size, bool withECN, Time rtt);

    void FlowSend (uint32_t flowId, Ipv4Address daddr, uint32_t path, uint32_t size, bool isRetrasmission);

    void FlowTimeout (uint32_t flowId, Ipv4Address daddr, uint32_t path);

    void FlowFinish (uint32_t flowId, Ipv4Address daddr);

    // These methods are used in probing
    void ProbeSend (Ipv4Address daddr, uint32_t path);

    void ProbeRecv (uint32_t path, Ipv4Address daddr, uint32_t size, bool withECN, Time rtt);

    void ProbeTimeout (uint32_t path, Ipv4Address daddr);

    // Node
    void SetNode (Ptr<Node> node);

    static std::string GetPathType (PathType type);

    static std::string GetLogo (void);

private:

    void PacketReceive (uint32_t flowId, uint32_t path, uint32_t destTorId,
                        uint32_t size, bool withECN, Time rtt, bool isProbing);

    bool UpdateFlowInfo (uint32_t flowId, uint32_t path, uint32_t size, bool withECN, Time rtt);

    TLBPathInfo GetInitPathInfo (uint32_t path);

    void UpdatePathInfo (uint32_t destTor, uint32_t path, uint32_t size, bool withECN, Time rtt);

    bool TimeoutFlow (uint32_t flowId, uint32_t path, bool &isVeryTimeout);

    void TimeoutPath (uint32_t destTor, uint32_t path, bool isProbing, bool isVeryTimeout);

    bool SendFlow (uint32_t flowId, uint32_t path, uint32_t size);

    void SendPath (uint32_t destTor, uint32_t path, uint32_t size);

    bool RetransFlow (uint32_t flowId, uint32_t path, uint32_t size, bool &needRetranPath, bool &needHighRetransPath);

    void RetransPath (uint32_t destTor, uint32_t path, bool needHighRetransPath);

    void UpdateFlowPath (uint32_t flowId, uint32_t path, uint32_t destTor);

    void AssignFlowToPath (uint32_t flowId, uint32_t destTor, uint32_t path);

    void RemoveFlowFromPath (uint32_t flowId, uint32_t destTor, uint32_t path);

    bool WhereToChange (uint32_t destTor, struct PathInfo &newPath, bool hasOldPath, uint32_t oldPath);

    struct PathInfo SelectRandomPath (uint32_t destTor);

    struct PathInfo JudgePath (uint32_t destTor, uint32_t path);

    bool PathLIsBetterR (struct PathInfo pathL, struct PathInfo pathR);

    bool FindTorId (Ipv4Address daddr, uint32_t &destTorId);

    void PathAging (void);

    void DreAging (void);

    std::vector<PathInfo> GatherParallelPaths (uint32_t destTor);

    uint32_t QuantifyRtt (Time rtt);
    uint32_t QuantifyDre (uint32_t dre);

    // Parameters
    uint32_t m_runMode; // Running Mode 0 for minimize counter, 1 for minimize RTT, 2 for random

    bool m_rerouteEnable;

    uint32_t m_S;

    Time m_T;

    uint32_t m_K;

    Time m_T1;

    Time m_T2;

    Time m_agingCheckTime;

    Time m_dreTime;

    double m_dreAlpha;

    DataRate m_dreDataRate;

    uint32_t m_dreQ;

    uint32_t m_dreMultiply;

    Time m_minRtt;

    Time m_highRtt;

    uint32_t m_ecnSampleMin;

    double m_ecnPortionLow;

    double m_ecnPortionHigh;

    // Failure Related Configurations
    bool m_respondToFailure;
    uint32_t m_flowRetransHigh;
    uint32_t m_flowRetransVeryHigh;

    uint32_t m_flowTimeoutCount;
    // End of Failure Related Configurations

    double m_betterPathEcnThresh;

    Time m_betterPathRttThresh;

    uint32_t m_pathChangePoss;

    Time m_flowDieTime;

    bool m_isSmooth;
    uint32_t m_smoothAlpha;
    uint32_t m_smoothDesired;
    uint32_t m_smoothBeta1;
    uint32_t m_smoothBeta2;

    Time m_quantifyRttBase;

    Time m_ackletTimeout;

    // Added at Jan 11st
    /*
    double m_epDefaultEcnPortion;
    double m_epAlpha;
    Time m_epCheckTime;
    Time m_epAgingTime;
    */
    // --

    // Added at Jan 12nd
    Time m_flowletTimeout;
    // --

    double m_rttAlpha;
    double m_ecnBeta;

    // Variables
    std::map<uint32_t, TLBFlowInfo> m_flowInfo; /* <FlowId, TLBFlowInfo> */
    std::map<std::pair<uint32_t, uint32_t>, TLBPathInfo> m_pathInfo; /* <DestTorId, PathId>, TLBPathInfo> */

    std::map<uint32_t, TLBAcklet> m_acklets; /* <FlowId, TLBAcklet> */

    std::map<Ipv4Address, uint32_t> m_ipTorMap; /* <DestAddress, DestTorId> */

    std::map<uint32_t, std::vector<uint32_t> > m_availablePath; /* <DestTorId, List<PathId>> */

    std::map<uint32_t, Ipv4Address> m_probingAgent; /* <DestTorId, ProbingAgentAddress>*/

    EventId m_agingEvent;

    EventId m_dreEvent;

    Ptr<Node> m_node;

    std::map<uint32_t, Time> m_pauseTime; // Used in the TCP pause, not mandatory

    typedef void (* TLBPathCallback) (uint32_t flowId, uint32_t fromTor,
            uint32_t toTor, uint32_t path, bool isRandom, PathInfo info, std::vector<PathInfo> parallelPaths);

    typedef void (* TLBPathChangeCallback) (uint32_t flowId, uint32_t fromTor, uint32_t toTor,
            uint32_t newPath, uint32_t oldPath, bool isRandom, std::vector<PathInfo> parallelPaths);

    TracedCallback <uint32_t, uint32_t, uint32_t, uint32_t, bool, PathInfo, std::vector<PathInfo> > m_pathSelectTrace;

    TracedCallback <uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, bool, std::vector<PathInfo> > m_pathChangeTrace;


};

}

#endif /* TLB_H */

