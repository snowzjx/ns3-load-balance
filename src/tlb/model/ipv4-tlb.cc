/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ipv4-tlb.h"

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/boolean.h"

#include <cstdio>
#include <algorithm>

#define RANDOM_BASE 100
#define SMOOTH_BASE 100

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv4TLB");

NS_OBJECT_ENSURE_REGISTERED (Ipv4TLB);

Ipv4TLB::Ipv4TLB ():
    m_runMode (0),
    m_rerouteEnable (false),
    m_S (64000),
    m_T (MicroSeconds (1500)),
    m_K (10000),
    m_T1 (MicroSeconds (320)), // 100 200 300
    m_T2 (MicroSeconds (100000000)),
    m_agingCheckTime (MicroSeconds (25)),
    m_dreTime (MicroSeconds (30)),
    m_dreAlpha (0.2),
    m_dreDataRate (DataRate ("1Gbps")),
    m_dreQ (3),
    m_dreMultiply (5),
    m_minRtt (MicroSeconds (60)), // 50 70 100
    m_highRtt (MicroSeconds (80)),
    m_ecnSampleMin (14000),
    m_ecnPortionLow (0.3), // 0.3 0.1
    m_ecnPortionHigh (1.1),
    m_respondToFailure (false),
    m_flowRetransHigh (140000000),
    m_flowRetransVeryHigh (140000000),
    m_flowTimeoutCount (10000),
    m_betterPathEcnThresh (0),
    m_betterPathRttThresh (MicroSeconds (1)), // 100 200 300
    m_pathChangePoss (50),
    m_flowDieTime (MicroSeconds (1000)),
    m_isSmooth (false),
    m_smoothAlpha (50),
    m_smoothDesired (150),
    m_smoothBeta1 (101),
    m_smoothBeta2 (99),
    m_quantifyRttBase (MicroSeconds (10)),
    m_ackletTimeout (MicroSeconds (300)),
    // Added at Jan 11st
    /*
    m_epDefaultEcnPortion (0.0),
    m_epAlpha (0.5),
    m_epCheckTime (MicroSeconds (10000)),
    m_epAgingTime (MicroSeconds (10000)),
    */
    // Added at Jan 12nd
    m_flowletTimeout (MicroSeconds (5000000)),
    m_rttAlpha(1.0),
    m_ecnBeta(0.0)
{
    NS_LOG_FUNCTION (this);
}

Ipv4TLB::Ipv4TLB (const Ipv4TLB &other):
    m_runMode (other.m_runMode),
    m_rerouteEnable (other.m_rerouteEnable),
    m_S (other.m_S),
    m_T (other.m_T),
    m_K (other.m_K),
    m_T1 (other.m_T1),
    m_T2 (other.m_T2),
    m_agingCheckTime (other.m_agingCheckTime),
    m_dreTime (other.m_dreTime),
    m_dreAlpha (other.m_dreAlpha),
    m_dreDataRate (other.m_dreDataRate),
    m_dreQ (other.m_dreQ),
    m_dreMultiply (other.m_dreMultiply),
    m_minRtt (other.m_minRtt),
    m_highRtt (other.m_highRtt),
    m_ecnSampleMin (other.m_ecnSampleMin),
    m_ecnPortionLow (other.m_ecnPortionLow),
    m_ecnPortionHigh (other.m_ecnPortionHigh),
    m_respondToFailure (other.m_respondToFailure),
    m_flowRetransHigh (other.m_flowRetransHigh),
    m_flowRetransVeryHigh (other.m_flowRetransVeryHigh),
    m_flowTimeoutCount (other.m_flowTimeoutCount),
    m_betterPathEcnThresh (other.m_betterPathEcnThresh),
    m_betterPathRttThresh (other.m_betterPathRttThresh),
    m_pathChangePoss (other.m_pathChangePoss),
    m_flowDieTime (other.m_flowDieTime),
    m_isSmooth (other.m_isSmooth),
    m_smoothAlpha (other.m_smoothAlpha),
    m_smoothDesired (other.m_smoothDesired),
    m_smoothBeta1 (other.m_smoothBeta1),
    m_smoothBeta2 (other.m_smoothBeta2),
    m_quantifyRttBase (other.m_quantifyRttBase),
    m_ackletTimeout (other.m_ackletTimeout),
    /*
    m_epDefaultEcnPortion (other.m_epDefaultEcnPortion),
    m_epAlpha (other.m_epAlpha),
    m_epCheckTime (other.m_epCheckTime),
    m_epAgingTime (other.m_epAgingTime),
    */
    m_flowletTimeout (other.m_flowletTimeout),
    m_rttAlpha (other.m_rttAlpha),
    m_ecnBeta (other.m_ecnBeta)
{
    NS_LOG_FUNCTION (this);
}

TypeId
Ipv4TLB::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::Ipv4TLB")
        .SetParent<Object> ()
        .SetGroupName ("TLB")
        .AddConstructor<Ipv4TLB> ()
        .AddAttribute ("RunMode", "The running mode of TLB (i.e., how to choose path from candidate paths), 0 for minimize counter, 1 for minimize RTT, 2 for random, 11 for RTT counter, 12 for RTT DRE",
                      UintegerValue (0),
                      MakeUintegerAccessor (&Ipv4TLB::m_runMode),
                      MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("Rerouting", "Whether rerouting is enabled",
                      BooleanValue (false),
                      MakeBooleanAccessor (&Ipv4TLB::m_rerouteEnable),
                      MakeBooleanChecker ())
        .AddAttribute ("MinRTT", "Min RTT threshold used to judge a good path",
                      TimeValue (MicroSeconds(50)),
                      MakeTimeAccessor (&Ipv4TLB::m_minRtt),
                      MakeTimeChecker ())
        .AddAttribute ("HighRTT", "High RTT threshold used to judge a bad path",
                      TimeValue (MicroSeconds(200)),
                      MakeTimeAccessor (&Ipv4TLB::m_highRtt),
                      MakeTimeChecker ())
        .AddAttribute ("DREMultiply", "DRE multiply factor (refer to CONGA)",
                      UintegerValue (5),
                      MakeUintegerAccessor (&Ipv4TLB::m_dreMultiply),
                      MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("S", "The sent size used to judge whether a flow should change path",
                      UintegerValue (64000),
                      MakeUintegerAccessor (&Ipv4TLB::m_S),
                      MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("BetterPathRTTThresh", "RTT Threshold used to judge whether one path is better than another",
                      TimeValue (MicroSeconds (300)),
                      MakeTimeAccessor (&Ipv4TLB::m_betterPathRttThresh),
                      MakeTimeChecker ())
        .AddAttribute ("ChangePathPoss", "Possibility to change the path (to avoid herd behavior)",
                      UintegerValue (50),
                      MakeUintegerAccessor (&Ipv4TLB::m_pathChangePoss),
                      MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("T1", "The path aging time interval (i.e., the frequency to update path condition)",
                      TimeValue (MicroSeconds (320)),
                      MakeTimeAccessor (&Ipv4TLB::m_T1),
                      MakeTimeChecker ())
        .AddAttribute ("ECNPortionLow", "The ECN portion threshold used in judging a good path",
                      DoubleValue (0.3),
                      MakeDoubleAccessor (&Ipv4TLB::m_ecnPortionLow),
                      MakeDoubleChecker<double> (0.0))
        .AddAttribute ("IsSmooth", "Whether the RTT calculation is smoothed (moving average)",
                      BooleanValue (false),
                      MakeBooleanAccessor (&Ipv4TLB::m_isSmooth),
                      MakeBooleanChecker ())
        .AddAttribute ("QuantifyRttBase", "The quantify RTT base  (granularity to quantify paths to differernt catagories according to RTT)",
                      TimeValue (MicroSeconds (10)),
                      MakeTimeAccessor (&Ipv4TLB::m_quantifyRttBase),
                      MakeTimeChecker ())
        .AddAttribute ("AckletTimeout", "The ACK flowlet timeout",
                      TimeValue (MicroSeconds (300)),
                      MakeTimeAccessor (&Ipv4TLB::m_ackletTimeout),
                      MakeTimeChecker ())
        .AddAttribute ("FlowletTimeout", "The flowlet timeout",
                      TimeValue (MicroSeconds (500)),
                      MakeTimeAccessor (&Ipv4TLB::m_flowletTimeout),
                      MakeTimeChecker ())
      .AddAttribute ("RespondToFailure", "Whether TLB reacts to failure",
                     BooleanValue (false),
                     MakeBooleanAccessor (&Ipv4TLB::m_respondToFailure),
                     MakeBooleanChecker ())
      .AddAttribute ("FlowRetransHigh",
                     "Threshold to determine whether the flow has experienced a serve retransmission",
                     UintegerValue (140000000),
                     MakeIntegerAccessor (&Ipv4TLB::m_flowRetransHigh),
                     MakeIntegerChecker<uint32_t> ())
      .AddAttribute ("FlowRetransVeryHigh",
                     "Threshold to determine whether the flow has experienced a very serve retransmission",
                     UintegerValue (140000000),
                     MakeIntegerAccessor (&Ipv4TLB::m_flowRetransVeryHigh),
                     MakeIntegerChecker<uint32_t> ())
      .AddAttribute ("FlowTimeoutCount",
                     "Threshold to determine whether the flow has expierienced a serve timeout",
                     UintegerValue (10000),
                     MakeIntegerAccessor (&Ipv4TLB::m_flowTimeoutCount),
                     MakeIntegerChecker<uint32_t> ())
      .AddAttribute ("RTTAlpha",
                     "The weight of RTT in characterizing paths into good, congested and gray",
                     DoubleValue (1.0),
                     MakeDoubleAccessor (&Ipv4TLB::m_rttAlpha),
                     MakeDoubleChecker<double> ())
      .AddAttribute ("ECNBeta",
                     "They weight of ECN in characterizing paths into good, congested and gray",
                     DoubleValue (0.0),
                     MakeDoubleAccessor (&Ipv4TLB::m_ecnBeta),
                     MakeDoubleChecker<double> ())
        // we set RTTAlpha = 1 and ECNBeta = 0 in our simulation as the RTT measurement is accurate. 
        // And we set RTTAlpha = 0 and ECNBeta = 1 in our testbed because the RTT measurement in our testbed is not accurate.
        // In general, Alpha and Beta can be flexibely tuned to reflect which value are more trusted.
        .AddTraceSource ("SelectPath",
                         "When the new flow is assigned the path",
                         MakeTraceSourceAccessor (&Ipv4TLB::m_pathSelectTrace),
                         "ns3::Ipv4TLB::TLBPathCallback")
        .AddTraceSource ("ChangePath",
                         "When the flow changes the path",
                         MakeTraceSourceAccessor (&Ipv4TLB::m_pathChangeTrace),
                         "ns3::Ipv4TLB::TLBPathChangeCallback")
    ;

    return tid;
}

void
Ipv4TLB::AddAddressWithTor (Ipv4Address address, uint32_t torId)
{
    m_ipTorMap[address] = torId;
}

void
Ipv4TLB::AddAvailPath (uint32_t destTor, uint32_t path)
{
    std::map<uint32_t, std::vector<uint32_t> >::iterator itr = m_availablePath.find (destTor);
    if (itr == m_availablePath.end ())
    {
        std::vector<uint32_t> paths;
        m_availablePath[destTor] = paths;
        itr = m_availablePath.find (destTor);
    }
    (itr->second).push_back (path);
}

std::vector<uint32_t>
Ipv4TLB::GetAvailPath (Ipv4Address daddr)
{
    std::vector<uint32_t> emptyVector;
    uint32_t destTor = 0;
    if (!Ipv4TLB::FindTorId (daddr, destTor))
    {
        NS_LOG_ERROR ("Cannot find dest tor id based on the given dest address");
        return emptyVector;
    }

    std::map<uint32_t, std::vector<uint32_t> >::iterator itr = m_availablePath.find (destTor);
    if (itr == m_availablePath.end ())
    {
        return emptyVector;
    }
    return itr->second;
}

uint32_t
Ipv4TLB::GetAckPath (uint32_t flowId, Ipv4Address saddr, Ipv4Address daddr)
{
    struct TLBAcklet acklet;
    std::map<uint32_t, TLBAcklet>::iterator ackletItr = m_acklets.find (flowId);

    if (ackletItr != m_acklets.end ())
    {
        // Existing flow
        acklet = ackletItr->second;
        if (Simulator::Now () - acklet.activeTime <= m_ackletTimeout) // Timeout
        {
            acklet.activeTime = Simulator::Now ();
            m_acklets[flowId] = acklet;
            return acklet.pathId;
        }

        // Bug Fix for bad small flow FCT in black hole case
        if (Simulator:: Now () - acklet.activeTime >= MilliSeconds (1))
        {
            uint32_t destTor = 0;
            if (!Ipv4TLB::FindTorId (daddr, destTor))
            {
                NS_LOG_ERROR ("Cannot find dest tor id based on the given dest address");
                return 0;
            }

            uint32_t oldPath = acklet.pathId;

            // Ipv4TLB::TimeoutPath (destTor, oldPath, false, true);

            struct PathInfo newPath;
            while (1)
            {
                newPath = Ipv4TLB::SelectRandomPath (destTor);
                if (newPath.pathId != oldPath)
                {
                    break;
                }
            }

            acklet.pathId = newPath.pathId;
            acklet.activeTime = Simulator::Now ();

            m_acklets[flowId] = acklet;

            return newPath.pathId;
        }
    }

    // New flow or expired flowlet
    uint32_t destTor = 0;
    if (!Ipv4TLB::FindTorId (daddr, destTor))
    {
        NS_LOG_ERROR ("Cannot find dest tor id based on the given dest address");
        return 0;
    }

    struct PathInfo newPath;
    if (!Ipv4TLB::WhereToChange (destTor, newPath, false, 0))
    {
        newPath = Ipv4TLB::SelectRandomPath (destTor);
    }

    acklet.pathId = newPath.pathId;
    acklet.activeTime = Simulator::Now ();

    m_acklets[flowId] = acklet;

    return newPath.pathId;
}

uint32_t
Ipv4TLB::GetPath (uint32_t flowId, Ipv4Address saddr, Ipv4Address daddr)
{
    if (!m_agingEvent.IsRunning ())
    {
        m_agingEvent = Simulator::Schedule (m_agingCheckTime, &Ipv4TLB::PathAging, this);
    }

    if (!m_dreEvent.IsRunning ())
    {
        m_dreEvent = Simulator::Schedule (m_dreTime, &Ipv4TLB::DreAging, this);
    }

    uint32_t destTor = 0;
    if (!Ipv4TLB::FindTorId (daddr, destTor))
    {
        NS_LOG_ERROR ("Cannot find dest tor id based on the given dest address");
        return 0;
    }

    uint32_t sourceTor = 0;
    if (!Ipv4TLB::FindTorId (saddr, sourceTor))
    {
        NS_LOG_ERROR ("Cannot find source tor id based on the given source address");
    }

    std::map<uint32_t, TLBFlowInfo>::iterator flowItr = m_flowInfo.find (flowId);

    // First check if the flow is a new flow
    if (flowItr == m_flowInfo.end ())
    {
        // New flow
        struct PathInfo newPath;
        if (Ipv4TLB::WhereToChange (destTor, newPath, false, 0))
        {
            m_pathSelectTrace (flowId, sourceTor, destTor, newPath.pathId, false, newPath, Ipv4TLB::GatherParallelPaths (destTor));
        }
        else
        {
            newPath = Ipv4TLB::SelectRandomPath (destTor);
            m_pathSelectTrace (flowId, sourceTor, destTor, newPath.pathId, true, newPath, Ipv4TLB::GatherParallelPaths (destTor));
        }
        Ipv4TLB::UpdateFlowPath (flowId, newPath.pathId, destTor);
        Ipv4TLB::AssignFlowToPath (flowId, destTor, newPath.pathId);
        return newPath.pathId;
    }
    else if (m_rerouteEnable)
    {
        Time flowActiveTime = (flowItr->second).activeTime;
        (flowItr->second).activeTime = Simulator::Now ();

        // Old flow
        uint32_t oldPath = (flowItr->second).path;
        struct PathInfo oldPathInfo = Ipv4TLB::JudgePath (destTor, oldPath);
        if (m_respondToFailure
                && ((flowItr->second).retransmissionSize > m_flowRetransVeryHigh
                || (flowItr->second).timeoutCount >= 1))
        {
            struct PathInfo newPath;
            if (Ipv4TLB::WhereToChange (destTor, newPath, true, oldPath))
            {
                if (newPath.pathId != oldPath)
                {
                    m_pathChangeTrace (flowId, sourceTor, destTor, newPath.pathId, oldPath, false, Ipv4TLB::GatherParallelPaths (destTor));
                }
            }
            else
            {
                newPath = Ipv4TLB::SelectRandomPath (destTor);
                if (newPath.pathId != oldPath)
                {
                    m_pathChangeTrace (flowId, sourceTor, destTor, newPath.pathId, oldPath, true, Ipv4TLB::GatherParallelPaths (destTor));
                }
            }

            if (newPath.pathId == oldPath)
            {
                return oldPath;
            }
            // Change path
            Ipv4TLB::UpdateFlowPath (flowId, newPath.pathId, destTor);
            Ipv4TLB::RemoveFlowFromPath (flowId, destTor, oldPath);
            Ipv4TLB::AssignFlowToPath (flowId, destTor, newPath.pathId);
            return newPath.pathId;
        }
        else if ((oldPathInfo.pathType == BadPath || Simulator::Now () - flowActiveTime > m_flowletTimeout) // Trigger for rerouting
                && oldPathInfo.quantifiedDre <= m_dreMultiply * 8  // TODO To be fixed
                && (flowItr->second).size >= m_S
                /*&& ((static_cast<double> ((flowItr->second).ecnSize) / (flowItr->second).size > m_ecnPortionHigh && Simulator::Now () - (flowItr->second).timeStamp >= m_T) || (flowItr->second).retransmissionSize > m_flowRetransHigh)*/
                && Simulator::Now() - (flowItr->second).tryChangePath > MicroSeconds (100))
        {
            if (rand () % RANDOM_BASE < static_cast<int> (RANDOM_BASE - m_pathChangePoss))
            {
                (flowItr->second).tryChangePath = Simulator::Now ();
                return oldPath;
            }
            struct PathInfo newPath;
            if (Ipv4TLB::WhereToChange (destTor, newPath, true, oldPath))
            {
                if (newPath.pathId == oldPath)
                {
                    return oldPath;
                }

                m_pathChangeTrace (flowId, sourceTor, destTor, newPath.pathId, oldPath, false, Ipv4TLB::GatherParallelPaths (destTor));

                // Calculate the pause time
                Time pauseTime = oldPathInfo.rttMin - newPath.rttMin;
                m_pauseTime[flowId] = std::max (pauseTime, MicroSeconds (1));

                // Change path
                Ipv4TLB::UpdateFlowPath (flowId, newPath.pathId, destTor);
                Ipv4TLB::RemoveFlowFromPath (flowId, destTor, oldPath);
                Ipv4TLB::AssignFlowToPath (flowId, destTor, newPath.pathId);
                return newPath.pathId;
            }
            else
            {
                // Do not change path
                return oldPath;
            }
        }
        else
        {
            return oldPath;
        }
    }
    else
    {
        (flowItr->second).activeTime = Simulator::Now ();

        uint32_t oldPath = (flowItr->second).path;
        return oldPath;
    }
}

Time
Ipv4TLB::GetPauseTime (uint32_t flowId)
{
   std::map<uint32_t, Time>::iterator itr = m_pauseTime.find (flowId);
   if (itr == m_pauseTime.end ())
   {
        return MicroSeconds (0);
   }
   return itr->second;
}

void
Ipv4TLB::FlowRecv (uint32_t flowId, uint32_t path, Ipv4Address daddr, uint32_t size, bool withECN, Time rtt)
{
    // NS_LOG_FUNCTION (flowId << path << daddr << size << withECN << rtt);
    uint32_t destTor = 0;
    if (!Ipv4TLB::FindTorId (daddr, destTor))
    {
        NS_LOG_ERROR ("Cannot find dest tor id based on the given dest address");
        return;
    }

    Ipv4TLB::PacketReceive (flowId, path, destTor, size, withECN, rtt, false);
}

void
Ipv4TLB::FlowSend (uint32_t flowId, Ipv4Address daddr, uint32_t path, uint32_t size, bool isRetransmission)
{
    // NS_LOG_FUNCTION (flowId << daddr << path << size << isRetransmission);
    uint32_t destTor = 0;
    if (!Ipv4TLB::FindTorId (daddr, destTor))
    {
        NS_LOG_ERROR ("Cannot find dest tor id based on the given dest address");
        return;
    }

    bool notChangePath = Ipv4TLB::SendFlow (flowId, path, size);

    if (!notChangePath)
    {
        NS_LOG_ERROR ("Cannot send flow on the expired path");
        return;
    }

    Ipv4TLB::SendPath (destTor, path, size);

    if (isRetransmission)
    {
        bool needRetransPath = false;
        bool needHighRetransPath = false;
        bool notChangePath = Ipv4TLB::RetransFlow (flowId, path, size, needRetransPath, needHighRetransPath);
        if (!notChangePath)
        {
            NS_LOG_LOGIC ("Cannot send flow on the expired path");
            return;
        }
        if (needRetransPath)
        {
            Ipv4TLB::RetransPath (destTor, path, needHighRetransPath);
        }
    }
}

void
Ipv4TLB::FlowTimeout (uint32_t flowId, Ipv4Address daddr, uint32_t path)
{
    uint32_t destTor = 0;
    if (!Ipv4TLB::FindTorId (daddr, destTor))
    {
        NS_LOG_ERROR ("Cannot find dest tor id based on the given dest address");
        return;
    }

    bool isVeryTimeout = false;
    bool notChangePath = Ipv4TLB::TimeoutFlow (flowId, path, isVeryTimeout);
    if (!notChangePath)
    {
        NS_LOG_LOGIC ("The flow has changed the path");
    }
    Ipv4TLB::TimeoutPath (destTor, path, false, isVeryTimeout);
}

void
Ipv4TLB::FlowFinish (uint32_t flowId, Ipv4Address daddr)
{
    uint32_t destTor = 0;
    if (!Ipv4TLB::FindTorId (daddr, destTor))
    {
        NS_LOG_ERROR ("Cannot find dest tor id based on the given dest address");
        return;
    }
    std::map<uint32_t, TLBFlowInfo>::iterator itr = m_flowInfo.find (flowId);
    if (itr == m_flowInfo.end ())
    {
        NS_LOG_ERROR ("Cannot finish a non-existing flow");
        return;
    }

    Ipv4TLB::RemoveFlowFromPath (flowId, destTor, (itr->second).path);

}

void
Ipv4TLB::ProbeSend (Ipv4Address daddr, uint32_t path)
{
    uint32_t destTor = 0;
    if (!Ipv4TLB::FindTorId (daddr, destTor))
    {
        NS_LOG_ERROR ("Cannot find dest tor id based on the given dest address");
        return;
    }
    std::pair<uint32_t, uint32_t> key = std::make_pair(destTor, path);
    std::map<std::pair<uint32_t, uint32_t>, TLBPathInfo>::iterator itr = m_pathInfo.find (key);

    if (itr == m_pathInfo.end ())
    {
        m_pathInfo[key] = Ipv4TLB::GetInitPathInfo (path);
    }

}

void
Ipv4TLB::ProbeRecv (uint32_t path, Ipv4Address daddr, uint32_t size, bool withECN, Time rtt)
{
    NS_LOG_FUNCTION (path << daddr << size << withECN << rtt);
    uint32_t destTor = 0;
    if (!Ipv4TLB::FindTorId (daddr, destTor))
    {
        NS_LOG_ERROR ("Cannot find dest tor id based on the given dest address");
        return;
    }
    Ipv4TLB::PacketReceive (0, path, destTor, size, withECN, rtt, true);
}

void
Ipv4TLB::ProbeTimeout (uint32_t path, Ipv4Address daddr)
{   uint32_t destTor = 0;
    if (!Ipv4TLB::FindTorId (daddr, destTor))
    {
        NS_LOG_ERROR ("Cannot find dest tor id based on the given dest address");
        return;
    }

    Ipv4TLB::TimeoutPath (path, destTor, true, false);
}


void
Ipv4TLB::SetNode (Ptr<Node> node)
{
    m_node = node;
}

void
Ipv4TLB::PacketReceive (uint32_t flowId, uint32_t path, uint32_t destTorId,
                        uint32_t size, bool withECN, Time rtt, bool isProbing)
{
    // If the packet acks the current path the flow goes, update the flow table and path table
    // If not or the packet is a probing, update the path table
    if (!isProbing)
    {
        bool notChangePath = Ipv4TLB::UpdateFlowInfo (flowId, path, size, withECN, rtt);
        if (!notChangePath)
        {
            NS_LOG_LOGIC ("The flow has changed the path");
        }
    }
    Ipv4TLB::UpdatePathInfo (destTorId, path, size, withECN, rtt);
}

bool
Ipv4TLB::UpdateFlowInfo (uint32_t flowId, uint32_t path, uint32_t size, bool withECN, Time rtt)
{
    std::map<uint32_t, TLBFlowInfo>::iterator itr = m_flowInfo.find (flowId);
    if (itr == m_flowInfo.end ())
    {
        NS_LOG_ERROR ("Cannot update info for a non-existing flow");
        return false;
    }
    if ((itr->second).path != path)
    {
        return false;
    }
    (itr->second).size += size;
    if (withECN)
    {
        (itr->second).ecnSize += size;
    }
    (itr->second).liveTime = Simulator::Now ();

    // Added Dec 23rd
    /*
    if (m_isSmooth)
    {
        (itr->second).rtt = (SMOOTH_BASE - m_smoothAlpha) * (itr->second).rtt / SMOOTH_BASE + m_smoothAlpha * rtt / SMOOTH_BASE;
    }
    else
    {
        if (rtt < (itr->second).rtt)
        {
            (itr->second).rtt = rtt;
        }
    }
    */
    // ---

    // Added Jan 11st
    /*
    (itr->second).epAckSize += size;
    if (withECN)
    {
        (itr->second).epEcnSize += size;
    }
    if (Simulator::Now () - (itr->second).epTimeStamp > m_epCheckTime)
    {
        double originalEcnPortion = (itr->second).epEcnPortion;
        double newEcnPortition = static_cast<double> ((itr->second).epEcnSize) / (itr->second).epAckSize;
        (itr->second).epAckSize = 1;
        (itr->second).epEcnSize = 0;
        (itr->second).epEcnPortion = m_epAlpha * originalEcnPortion + (1.0 - m_epAlpha) * newEcnPortition;
        (itr->second).epTimeStamp = Simulator::Now ();
    }
    */
    // --

    return true;
}

void
Ipv4TLB::UpdatePathInfo (uint32_t destTor, uint32_t path, uint32_t size, bool withECN, Time rtt)
{
    std::pair<uint32_t, uint32_t> key = std::make_pair(destTor, path);
    std::map<std::pair<uint32_t, uint32_t>, TLBPathInfo>::iterator itr = m_pathInfo.find (key);

    TLBPathInfo pathInfo;
    if (itr == m_pathInfo.end ())
    {
        pathInfo = Ipv4TLB::GetInitPathInfo (path);
    }
    else
    {
        pathInfo = itr->second;
    }

    pathInfo.size += size;
    if (withECN)
    {
        pathInfo.ecnSize += size;
    }
    if (m_isSmooth)
    {
        pathInfo.minRtt = (SMOOTH_BASE - m_smoothAlpha) * pathInfo.minRtt / SMOOTH_BASE + m_smoothAlpha * rtt / SMOOTH_BASE;
    }
    else
    {
        if (rtt < pathInfo.minRtt)
        {
            pathInfo.minRtt = rtt;
        }
    }
    pathInfo.timeStamp3 = Simulator::Now ();

    // Added Jan 11st
    /*
    pathInfo.epAckSize += size;
    if (withECN)
    {
        pathInfo.epEcnSize += size;
    }
    if (Simulator::Now () - pathInfo.epTimeStamp > m_epCheckTime)
    {
        double originalEcnPortion = pathInfo.epEcnPortion;
        double newEcnPortition = static_cast<double> (pathInfo.epEcnSize) / pathInfo.epAckSize;
        pathInfo.epAckSize = 1;
        pathInfo.epEcnSize = 0;
        pathInfo.epEcnPortion = m_epAlpha * originalEcnPortion + (1.0 - m_epAlpha) * newEcnPortition;
        pathInfo.epTimeStamp = Simulator::Now ();
    }
    */
    // --

    m_pathInfo[key] = pathInfo;
}

bool
Ipv4TLB::TimeoutFlow (uint32_t flowId, uint32_t path, bool &isVeryTimeout)
{
    isVeryTimeout = false;
    std::map<uint32_t, TLBFlowInfo>::iterator itr = m_flowInfo.find (flowId);
    if (itr == m_flowInfo.end ())
    {
        NS_LOG_ERROR ("Cannot timeout a non-existing flow");
        return false;
    }
    if ((itr->second).path != path)
    {
        return false;
    }
    (itr->second).timeoutCount ++;
    if ((itr->second).timeoutCount >= m_flowTimeoutCount)
    {
        isVeryTimeout = true;
    }
    return true;
}

bool
Ipv4TLB::SendFlow (uint32_t flowId, uint32_t path, uint32_t size)
{
    std::map<uint32_t, TLBFlowInfo>::iterator itr = m_flowInfo.find (flowId);
    if (itr == m_flowInfo.end ())
    {
        NS_LOG_ERROR ("Cannot retransmit a non-existing flow");
        return false;
    }
    if ((itr->second).path != path)
    {
        return false;
    }
    (itr->second).sendSize += size;
    return true;
}

void
Ipv4TLB::SendPath (uint32_t destTor, uint32_t path, uint32_t size)
{
    std::pair<uint32_t, uint32_t> key = std::make_pair(destTor, path);
    std::map<std::pair<uint32_t, uint32_t>, TLBPathInfo>::iterator itr = m_pathInfo.find (key);

    if (itr == m_pathInfo.end ())
    {
        NS_LOG_ERROR ("Cannot send a non-existing path");
        return;
    }

    (itr->second).dreValue += size;
}

bool
Ipv4TLB::RetransFlow (uint32_t flowId, uint32_t path, uint32_t size, bool &needRetranPath, bool &needHighRetransPath)
{
    needRetranPath = false;
    needHighRetransPath = false;
    std::map<uint32_t, TLBFlowInfo>::iterator itr = m_flowInfo.find (flowId);
    if (itr == m_flowInfo.end ())
    {
        NS_LOG_ERROR ("Cannot retransmit a non-existing flow");
        return false;
    }
    if ((itr->second).path != path)
    {
        return false;
    }
    if (Simulator::Now () - (itr->second).timeStamp < MicroSeconds (1000))
    {
        return false;
    }
    (itr->second).retransmissionSize += size;
    if ((itr->second).retransmissionSize > m_flowRetransHigh)
    {
        needRetranPath = true;
    }
    if ((itr->second).retransmissionSize > m_flowRetransVeryHigh)
    {
        needHighRetransPath = true;
    }
    return true;
}


void
Ipv4TLB::TimeoutPath (uint32_t destTor, uint32_t path, bool isProbing, bool isVeryTimeout)
{
    std::pair<uint32_t, uint32_t> key = std::make_pair(destTor, path);
    std::map<std::pair<uint32_t, uint32_t>, TLBPathInfo>::iterator itr = m_pathInfo.find (key);
    if (itr == m_pathInfo.end ())
    {
        NS_LOG_ERROR ("Cannot timeout a non-existing path");
        return;
    }
    if (!isProbing)
    {
        (itr->second).isTimeout = true;
        if (isVeryTimeout)
        {
            (itr->second).isVeryTimeout = true;
        }
    }
    else
    {
        (itr->second).isProbingTimeout = true;
    }
}

void
Ipv4TLB::RetransPath (uint32_t destTor, uint32_t path, bool needHighRetransPath)
{
    std::pair<uint32_t, uint32_t> key = std::make_pair(destTor, path);
    std::map<std::pair<uint32_t, uint32_t>, TLBPathInfo>::iterator itr = m_pathInfo.find (key);
    if (itr == m_pathInfo.end ())
    {
        NS_LOG_ERROR ("Cannot timeout a non-existing path");
        return;
    }
    (itr->second).isRetransmission = true;
    if (needHighRetransPath)
    {
        (itr->second).isHighRetransmission = true;
    }
}

void
Ipv4TLB::UpdateFlowPath (uint32_t flowId, uint32_t path, uint32_t destTor)
{
    TLBFlowInfo flowInfo;
    flowInfo.path = path;
    flowInfo.destTor = destTor;
    flowInfo.size = 0;
    flowInfo.ecnSize = 0;
    flowInfo.sendSize = 0;
    flowInfo.retransmissionSize = 0;
    flowInfo.timeoutCount = 0;
    flowInfo.timeStamp = Simulator::Now ();
    flowInfo.tryChangePath = Simulator::Now ();
    flowInfo.liveTime = Simulator::Now ();

    // Added Dec 23rd
    // Flow RTT default value
    // flowInfo.rtt = m_minRtt;

    // Added Jan 11st
    // Flow ECN portion default value
    /*
    flowInfo.epAckSize = 1;
    flowInfo.epEcnSize = 0;
    flowInfo.epEcnPortion = m_epDefaultEcnPortion;
    flowInfo.epTimeStamp = Simulator::Now ();
    */

    // Added Jan 12nd
    flowInfo.activeTime = Simulator::Now ();

    m_flowInfo[flowId] = flowInfo;
}

TLBPathInfo
Ipv4TLB::GetInitPathInfo (uint32_t path)
{
    TLBPathInfo pathInfo;
    pathInfo.pathId = path;
    pathInfo.size = 3;
    pathInfo.ecnSize = 1;
    /*pathInfo.minRtt = m_betterPathRttThresh + MicroSeconds (100);*/
    pathInfo.minRtt = m_minRtt;
    pathInfo.isRetransmission = false;
    pathInfo.isHighRetransmission = false;
    pathInfo.isTimeout = false;
    pathInfo.isVeryTimeout = false;
    pathInfo.isProbingTimeout = false;
    pathInfo.flowCounter = 0; // XXX Notice the flow count will be update using Add/Remove Flow To/From Path method
    pathInfo.timeStamp1 = Simulator::Now ();
    pathInfo.timeStamp2 = Simulator::Now ();
    pathInfo.timeStamp3 = Simulator::Now ();
    pathInfo.dreValue = 0;

    // Added Jan 11st
    // Path ECN portion default value
    /*
    pathInfo.epAckSize = 1;
    pathInfo.epEcnSize = 0;
    pathInfo.epEcnPortion = m_epDefaultEcnPortion;
    pathInfo.epTimeStamp = Simulator::Now ();
    */

    return pathInfo;
}

void
Ipv4TLB::AssignFlowToPath (uint32_t flowId, uint32_t destTor, uint32_t path)
{
    std::pair<uint32_t, uint32_t> key = std::make_pair(destTor, path);
    std::map<std::pair<uint32_t, uint32_t>, TLBPathInfo>::iterator itr = m_pathInfo.find (key);

    TLBPathInfo pathInfo;
    if (itr == m_pathInfo.end ())
    {
        pathInfo = Ipv4TLB::GetInitPathInfo (path);
    }
    else
    {
        pathInfo = itr->second;
    }

    pathInfo.flowCounter ++;
    m_pathInfo[key] = pathInfo;
}

void
Ipv4TLB::RemoveFlowFromPath (uint32_t flowId, uint32_t destTor, uint32_t path)
{
    std::pair<uint32_t, uint32_t> key = std::make_pair(destTor, path);
    std::map<std::pair<uint32_t, uint32_t>, TLBPathInfo>::iterator itr = m_pathInfo.find (key);
    if (itr == m_pathInfo.end ())
    {
        NS_LOG_ERROR ("Cannot remove flow from a non-existing path");
        return;
    }
    if ((itr->second).flowCounter == 0)
    {
        NS_LOG_ERROR ("Cannot decrease from counter while it has reached 0");
        return;
    }
    (itr->second).flowCounter --;

}

bool
Ipv4TLB::WhereToChange (uint32_t destTor, PathInfo &newPath, bool hasOldPath, uint32_t oldPath)
{
    std::map<uint32_t, std::vector<uint32_t> >::iterator itr = m_availablePath.find (destTor);

    if (itr == m_availablePath.end ())
    {
        NS_LOG_ERROR ("Cannot find available paths");
        return false;
    }

    std::vector<uint32_t>::iterator vectorItr = (itr->second).begin ();

    // Firstly, checking good path
    uint32_t minCounter = std::numeric_limits<uint32_t>::max ();
    Time minRTT = Seconds (666);
    uint32_t minRTTLevel = 5;
    uint32_t minDre = std::pow (2, m_dreQ);
    std::vector<PathInfo> candidatePaths;
    for ( ; vectorItr != (itr->second).end (); ++vectorItr)
    {
        uint32_t pathId = *vectorItr;
        struct PathInfo pathInfo = JudgePath (destTor, pathId);
        if (pathInfo.pathType == GoodPath)
        {
            if (m_runMode == TLB_RUNMODE_COUNTER)
            {
                if (pathInfo.counter <= minCounter)
                {
                    if (pathInfo.counter < minCounter)
                    {
                        candidatePaths.clear ();
                        minCounter = pathInfo.counter;
                    }
                    candidatePaths.push_back (pathInfo);
                }
            }
            else if (m_runMode == TLB_RUNMODE_MINRTT)
            {
                if (pathInfo.rttMin <= minRTT)
                {
                    if (pathInfo.rttMin < minRTT)
                    {
                        candidatePaths.clear ();
                        minRTT = pathInfo.rttMin;
                    }
                    candidatePaths.push_back (pathInfo);
                }
            }
            else if (m_runMode == TLB_RUNMODE_RTT_COUNTER || m_runMode == TLB_RUNMODE_RTT_DRE)
            {
                uint32_t RTTLevel = Ipv4TLB::QuantifyRtt (pathInfo.rttMin);
                if (RTTLevel < minRTTLevel)
                {
                    minRTTLevel = RTTLevel;
                    minCounter = std::numeric_limits<uint32_t>::max ();
                    candidatePaths.clear ();
                }
                if (RTTLevel == minRTTLevel)
                {

                    if (m_runMode == TLB_RUNMODE_RTT_DRE)
                    {
                        if (pathInfo.counter < minCounter)
                        {
                            minCounter = pathInfo.counter;
                            candidatePaths.clear ();
                        }
                        if (pathInfo.counter == minCounter)
                        {
                            candidatePaths.push_back (pathInfo);
                        }
                    }
                    else if (m_runMode == TLB_RUNMODE_RTT_DRE)
                    {
                        if (pathInfo.quantifiedDre < minDre)
                        {
                            minDre = pathInfo.quantifiedDre;
                            candidatePaths.clear ();
                        }
                        if (pathInfo.quantifiedDre == minDre)
                        {
                            candidatePaths.push_back (pathInfo);
                        }
                    }
                }
            }
            else
            {
                candidatePaths.push_back (pathInfo);
            }
        }
    }

    if (!candidatePaths.empty ())
    {
        if (m_runMode == TLB_RUNMODE_COUNTER)
        {
            if (minCounter <= m_K)
            {
                newPath = candidatePaths[rand () % candidatePaths.size ()];
            }
        }
        else if (m_runMode == TLB_RUNMODE_MINRTT)
        {
            newPath = candidatePaths[rand () % candidatePaths.size ()];
        }
        else if (m_runMode == TLB_RUNMODE_RTT_COUNTER || m_runMode == TLB_RUNMODE_RTT_DRE)
        {
            newPath = candidatePaths[rand () % candidatePaths.size ()];
        }
        else
        {
            newPath = candidatePaths[rand () % candidatePaths.size ()];
        }
        NS_LOG_LOGIC ("Find Good Path: " << newPath.pathId);
        return true;
    }

    // Secondly, checking grey path
    struct PathInfo originalPath;
    if (hasOldPath)
    {
        originalPath = JudgePath (destTor, oldPath);
    }
    else
    {
        originalPath.pathType = GreyPath;
        originalPath.rttMin = Seconds (666);
        originalPath.ecnPortion = 1;
        originalPath.counter = std::numeric_limits<uint32_t>::max ();
        originalPath.quantifiedDre = std::pow (2, m_dreQ);
    }

    minCounter = std::numeric_limits<uint32_t>::max ();
    minRTT = Seconds (666);
    minDre = std::pow (2, m_dreQ);
    candidatePaths.clear ();
    vectorItr = (itr->second).begin ();
    for ( ; vectorItr != (itr->second).end (); ++vectorItr)
    {
        uint32_t pathId = *vectorItr;
        struct PathInfo pathInfo = JudgePath (destTor, pathId);
        if (pathInfo.pathType == GreyPath
            && Ipv4TLB::PathLIsBetterR (pathInfo, originalPath))
        {
            if (m_runMode == TLB_RUNMODE_COUNTER)
            {
                if (pathInfo.counter <= minCounter)
                {
                    if (pathInfo.counter < minCounter)
                    {
                        candidatePaths.clear ();
                        minCounter = pathInfo.counter;
                    }
                    candidatePaths.push_back (pathInfo);
                }
            }
            else if (m_runMode == TLB_RUNMODE_MINRTT)
            {
                if (pathInfo.rttMin <= minRTT)
                {
                    if (pathInfo.rttMin < minRTT)
                    {
                        candidatePaths.clear ();
                        minRTT = pathInfo.rttMin;
                    }
                    candidatePaths.push_back (pathInfo);
                }
            }
            else if (m_runMode == TLB_RUNMODE_RTT_COUNTER || m_runMode == TLB_RUNMODE_RTT_DRE)
            {
                uint32_t RTTLevel = Ipv4TLB::QuantifyRtt (pathInfo.rttMin);
                if (RTTLevel < minRTTLevel)
                {
                    minRTTLevel = RTTLevel;
                    minCounter = std::numeric_limits<uint32_t>::max ();
                    candidatePaths.clear ();
                }
                if (RTTLevel == minRTTLevel)
                {

                    if (m_runMode == TLB_RUNMODE_RTT_DRE)
                    {
                        if (pathInfo.counter < minCounter)
                        {
                            minCounter = pathInfo.counter;
                            candidatePaths.clear ();
                        }
                        if (pathInfo.counter == minCounter)
                        {
                            candidatePaths.push_back (pathInfo);
                        }
                    }
                    else if (m_runMode == TLB_RUNMODE_RTT_DRE)
                    {
                        if (pathInfo.quantifiedDre < minDre)
                        {
                            minDre = pathInfo.quantifiedDre;
                            candidatePaths.clear ();
                        }
                        if (pathInfo.quantifiedDre == minDre)
                        {
                            candidatePaths.push_back (pathInfo);
                        }
                    }
                }
            }
            else
            {
                candidatePaths.push_back (pathInfo);
            }
        }
    }

    if (!candidatePaths.empty ())
    {
        if (m_runMode == TLB_RUNMODE_COUNTER)
        {
            if (minCounter <= m_K)
            {
                newPath = candidatePaths[rand () % candidatePaths.size ()];
            }
        }
        else if (m_runMode == TLB_RUNMODE_MINRTT)
        {
            newPath = candidatePaths[rand () % candidatePaths.size ()];
        }
        else if (m_runMode == TLB_RUNMODE_RTT_COUNTER || m_runMode == TLB_RUNMODE_RTT_DRE)
        {
            newPath = candidatePaths[rand () % candidatePaths.size ()];
        }

        else
        {
            newPath = candidatePaths[rand () % candidatePaths.size ()];
        }
        NS_LOG_LOGIC ("Find Grey Path: " << newPath.pathId);
        return true;
    }

   // Thirdly, checking bad path
    vectorItr = (itr->second).begin ();
    for ( ; vectorItr != (itr->second).end (); ++vectorItr)
    {
        uint32_t pathId = *vectorItr;
        struct PathInfo pathInfo = JudgePath (destTor, pathId);
        if (pathInfo.pathType == BadPath
            && Ipv4TLB::PathLIsBetterR (pathInfo, originalPath))
        {
            newPath = pathInfo;
            NS_LOG_LOGIC ("Find Bad Path: " << newPath.pathId);
            return true;
        }
    }

    // Thirdly, indicating no paths available
    NS_LOG_LOGIC ("No Path Returned");
    return false;
}

struct PathInfo
Ipv4TLB::SelectRandomPath (uint32_t destTor)
{
    std::map<uint32_t, std::vector<uint32_t> >::iterator itr = m_availablePath.find (destTor);

    if (itr == m_availablePath.end ())
    {
        NS_LOG_ERROR ("Cannot find available paths");
        PathInfo pathInfo;
        pathInfo.pathId = 0;
        return pathInfo;
    }

    std::vector<uint32_t>::iterator vectorItr = (itr->second).begin ();
    std::vector<PathInfo> availablePaths;
    for ( ; vectorItr != (itr->second).end (); ++vectorItr)
    {
        uint32_t pathId = *vectorItr;
        struct PathInfo pathInfo = JudgePath (destTor, pathId);
        if (pathInfo.pathType == GoodPath || pathInfo.pathType == GreyPath || pathInfo.pathType == BadPath)
        {
            availablePaths.push_back (pathInfo);
        }
    }

    struct PathInfo newPath;
    if (!availablePaths.empty ())
    {
        newPath = availablePaths[rand() % availablePaths.size ()];
    }
    else
    {
        uint32_t pathId = (itr->second)[rand() % (itr->second).size ()];
        newPath = Ipv4TLB::JudgePath (destTor, pathId);
    }
    NS_LOG_LOGIC ("Random selection return path: " << newPath.pathId);
    return newPath;
}

struct PathInfo
Ipv4TLB::JudgePath (uint32_t destTor, uint32_t pathId)
{
    std::pair<uint32_t, uint32_t> key = std::make_pair (destTor, pathId);
    std::map<std::pair<uint32_t, uint32_t>, TLBPathInfo>::iterator itr = m_pathInfo.find (key);

    struct PathInfo path;
    path.pathId = pathId;
    if (itr == m_pathInfo.end ())
    {
        path.pathType = GreyPath;
        /*path.pathType = GoodPath;*/
        /*path.rttMin = m_betterPathRttThresh + MicroSeconds (100);*/
        path.rttMin = m_minRtt;
        path.size = 0;
        path.ecnPortion = 0.3;
        path.counter = 0;
        path.quantifiedDre = 0;
        return path;
    }
    TLBPathInfo pathInfo = itr->second;
    path.rttMin = pathInfo.minRtt;
    path.size = pathInfo.size;
    path.ecnPortion = static_cast<double>(pathInfo.ecnSize) / pathInfo.size;
    path.counter = pathInfo.flowCounter;
    path.quantifiedDre = Ipv4TLB::QuantifyDre (pathInfo.dreValue);

    int consideECN = (pathInfo.size > m_ecnSampleMin) ? 1 : 0;

    if ((m_rttAlpha * pathInfo.minRtt + m_ecnBeta * consideECN * static_cast<double>(pathInfo.ecnSize) / pathInfo.size
                < m_rttAlpha * m_minRtt + m_ecnBeta * consideECN * m_ecnPortionLow)
            && (pathInfo.isRetransmission) == false
            /*&& (pathInfo.isHighRetransmission) == false*/
            && (pathInfo.isTimeout) == false
            /*&& (pathInfo.isVeryTimeout) == false*/
            && (pathInfo.isProbingTimeout == false))
    {
        path.pathType = GoodPath;
        return path;
    }

    if (pathInfo.isHighRetransmission
            || pathInfo.isVeryTimeout
            || pathInfo.isProbingTimeout)
    {
        path.pathType = FailPath;
        return path;
    }

    if ((m_rttAlpha * pathInfo.minRtt + m_ecnBeta * consideECN * static_cast<double>(pathInfo.ecnSize) / pathInfo.size
                >= m_rttAlpha * m_highRtt + m_ecnBeta * consideECN * m_ecnPortionHigh)
            || pathInfo.isTimeout == true
            || pathInfo.isRetransmission == true)
    {
        path.pathType = BadPath;
        return path;
    }

    path.pathType = GreyPath;
    return path;
}

bool
Ipv4TLB::PathLIsBetterR (struct PathInfo pathL, struct PathInfo pathR)
{
    if (/*pathR.ecnPortion - pathL.ecnPortion >= m_betterPathEcnThresh // TODO Comment ECN
        &&*/ pathR.rttMin - pathL.rttMin >= m_betterPathRttThresh)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool
Ipv4TLB::FindTorId (Ipv4Address daddr, uint32_t &destTorId)
{
    std::map<Ipv4Address, uint32_t>::iterator torItr = m_ipTorMap.find (daddr);

    if (torItr == m_ipTorMap.end ())
    {
        return false;
    }
    destTorId = torItr->second;
    return true;
}

void
Ipv4TLB::PathAging (void)
{
    NS_LOG_LOGIC (this << " Path Info: " << (Simulator::Now ()));
    std::map<std::pair<uint32_t, uint32_t>, TLBPathInfo>::iterator itr = m_pathInfo.begin ();
    for ( ; itr != m_pathInfo.end (); ++itr)
    {
        NS_LOG_LOGIC ("<" << (itr->first).first << "," << (itr->first).second << ">");
        NS_LOG_LOGIC ("\t" << " Size: " << (itr->second).size
                           << " ECN Size: " << (itr->second).ecnSize
                           << " Min RTT: " << (itr->second).minRtt
                           << " Is Retransmission: " << (itr->second).isRetransmission
                           << " Is HRetransmission: " << (itr->second).isHighRetransmission
                           << " Is Timeout: " << (itr->second).isTimeout
                           << " Is VTimeout: " << (itr->second).isVeryTimeout
                           << " Is ProbingTimeout: " << (itr->second).isProbingTimeout
                           << " Flow Counter: " << (itr->second).flowCounter);
        if (Simulator::Now() - (itr->second).timeStamp1 > m_T1)
        {
            (itr->second).size = 1;
            (itr->second).ecnSize = 0;
            (itr->second).isTimeout = false;
            (itr->second).timeStamp1 = Simulator::Now ();
        }
        if (Simulator::Now () - (itr->second).timeStamp2 > m_T2)
        {
            (itr->second).isRetransmission = false;
            (itr->second).isHighRetransmission = false;
            (itr->second).isVeryTimeout = false;
            (itr->second).isProbingTimeout = false;
            (itr->second).timeStamp2 = Simulator::Now ();
        }
        if (Simulator::Now () - (itr->second).timeStamp3 > m_T1)
        {
            if (m_isSmooth)
            {
                Time desiredRtt = m_minRtt * m_smoothDesired / SMOOTH_BASE;
                if ((itr->second).minRtt < desiredRtt)
                {
                    (itr->second).minRtt = std::min (desiredRtt, (itr->second).minRtt * m_smoothBeta1 / SMOOTH_BASE);
                }
                else
                {
                    (itr->second).minRtt = std::max (desiredRtt, (itr->second).minRtt * m_smoothBeta2 / SMOOTH_BASE);
                }
            }
            else
            {
                (itr->second).minRtt = Seconds (666);
            }
            (itr->second).timeStamp3 = Simulator::Now ();
        }

        /*
        if (Simulator::Now () - (itr->second).epTimeStamp > m_epAgingTime)
        {
            (itr->second).epAckSize = 1;
            (itr->second).epEcnSize = 0;
            (itr->second).epEcnPortion = m_epDefaultEcnPortion;
            (itr->second).epTimeStamp = Simulator::Now ();
        }
        */
    }

    std::map<uint32_t, TLBFlowInfo>::iterator itr2 = m_flowInfo.begin ();
    for ( ; itr2 != m_flowInfo.end (); ++itr2)
    {
        if (Simulator::Now () - (itr2->second).liveTime >= m_flowDieTime)
        {
            Ipv4TLB::RemoveFlowFromPath ((itr2->second).flowId, (itr2->second).destTor, (itr2->second).path);
            m_flowInfo.erase (itr2);
        }

        /*
        else if (Simulator::Now () - (itr2->second).epTimeStamp > m_epAgingTime)
        {
            (itr2->second).epAckSize = 1;
            (itr2->second).epEcnSize = 0;
            (itr2->second).epEcnPortion = m_epDefaultEcnPortion;
            (itr2->second).epTimeStamp = Simulator::Now ();
        }
        */
    }

    m_agingEvent = Simulator::Schedule (m_agingCheckTime, &Ipv4TLB::PathAging, this);
}

std::vector<PathInfo>
Ipv4TLB::GatherParallelPaths (uint32_t destTor)
{
    std::vector<PathInfo> paths;

    std::map<uint32_t, std::vector<uint32_t> >::iterator itr = m_availablePath.find (destTor);
    if (itr == m_availablePath.end ())
    {
        return paths;
    }

    std::vector<uint32_t>::iterator innerItr = (itr->second).begin ();
    for ( ; innerItr != (itr->second).end (); ++innerItr )
    {
        paths.push_back(Ipv4TLB::JudgePath ((itr->first), *innerItr));
    }

    return paths;
}

void
Ipv4TLB::DreAging (void)
{
    std::map<std::pair<uint32_t, uint32_t>, TLBPathInfo>::iterator itr = m_pathInfo.begin ();
    for ( ; itr != m_pathInfo.end (); ++itr)
    {
        NS_LOG_LOGIC ("<" << (itr->first).first << "," << (itr->first).second << ">");
        (itr->second).dreValue *= (1 - m_dreAlpha);
        NS_LOG_LOGIC ("\tDre value :" << Ipv4TLB::QuantifyDre ((itr->second).dreValue));
    }

    m_dreEvent = Simulator::Schedule (m_dreTime, &Ipv4TLB::DreAging, this);
}

uint32_t
Ipv4TLB::QuantifyRtt (Time rtt)
{
    if (rtt <= m_minRtt + m_quantifyRttBase)
    {
        return 0;
    }
    else if (rtt <= m_minRtt + 2 * m_quantifyRttBase)
    {
        return 1;
    }
    else if (rtt <= m_minRtt + 3 * m_quantifyRttBase)
    {
        return 2;
    }
    else if (rtt <= m_minRtt + 4 * m_quantifyRttBase)
    {
        return 3;
    }
    else
    {
        return 4;
    }
}

uint32_t
Ipv4TLB::QuantifyDre (uint32_t dre)
{
    double ratio = static_cast<double> (dre * 8) / (m_dreDataRate.GetBitRate () * m_dreTime.GetSeconds () / m_dreAlpha);
    return static_cast<uint32_t> (ratio * std::pow (2, m_dreQ));
}

std::string
Ipv4TLB::GetPathType (PathType type)
{
    if (type == GoodPath)
    {
        return "GoodPath";
    }
    else if (type == GreyPath)
    {
        return "GreyPath";
    }
    else if (type == BadPath)
    {
        return "BadPath";
    }
    else if (type == FailPath)
    {
        return "FailPath";
    }
    else
    {
        return "Unknown";
    }
}

std::string
Ipv4TLB::GetLogo (void)
{
    std::stringstream oss;
    oss << " .-') _           .-. .-')           ('-.       .-') _    ('-.    .-. .-')               ('-.  _ .-') _   " << std::endl;
    oss << "(  OO) )          \\  ( OO )        _(  OO)     ( OO ) )  ( OO ).-.\\  ( OO )            _(  OO)( (  OO) )  " << std::endl;
    oss << "/     '._ ,--.     ;-----.\\       (,------.,--./ ,--,'   / . --. / ;-----.\\  ,--.     (,------.\\     .'_  " << std::endl;
    oss << "|'--...__)|  |.-') | .-.  |        |  .---'|   \\ |  |\\   | \\-.  \\  | .-.  |  |  |.-')  |  .---',`'--..._) " << std::endl;
    oss << "'--.  .--'|  | OO )| '-' /_)       |  |    |    \\|  | ).-'-'  |  | | '-' /_) |  | OO ) |  |    |  |  \\  ' " << std::endl;
    oss << "   |  |   |  |`-' || .-. `.       (|  '--. |  .     |/  \\| |_.'  | | .-. `.  |  |`-' |(|  '--. |  |   ' | " << std::endl;
    oss << "   |  |  (|  '---.'| |  \\  |       |  .--' |  |\\    |    |  .-.  | | |  \\  |(|  '---.' |  .--' |  |   / : " << std::endl;
    oss << "   |  |   |      | | '--'  /       |  `---.|  | \\   |    |  | |  | | '--'  / |      |  |  `---.|  '--'  / " << std::endl;
    oss << "   `--'   `------' `------'        `------'`--'  `--'    `--' `--' `------'  `------'  `------'`-------'  " << std::endl;
    return oss.str ();
}

}
