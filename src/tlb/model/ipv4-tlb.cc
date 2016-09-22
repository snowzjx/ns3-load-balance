/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ipv4-tlb.h"

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"

#define RANDOM_BASE 100
// RAND POSS 30% 60% 90% will change path

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv4TLB");

NS_OBJECT_ENSURE_REGISTERED (Ipv4TLB);

Ipv4TLB::Ipv4TLB ():
    m_S (64000),
    m_T (MicroSeconds (1500)),
    m_K (10000),
    m_T1 (MicroSeconds (320)), // 100 200 300
    m_T2 (MicroSeconds (320)),
    m_agingCheckTime (MicroSeconds (100)),
    m_minRtt (MicroSeconds (60)), // 50 70 100
    m_ecnSampleMin (14000),
    m_ecnPortionLow (0.3), // 0.3 0.1
    m_ecnPortionHigh (1.1),
    m_flowRetransHigh (1400000000),
    m_flowRetransVeryHigh (1400000000),
    m_flowTimeoutCount (10),
    m_betterPathEcnThresh (0),
    m_betterPathRttThresh (MicroSeconds (1)), // 100 200 300
    m_pathChangePoss (50)
{
    NS_LOG_FUNCTION (this);
}

Ipv4TLB::Ipv4TLB (const Ipv4TLB &other):
    m_S (other.m_S),
    m_T (other.m_T),
    m_K (other.m_K),
    m_T1 (other.m_T1),
    m_T2 (other.m_T2),
    m_agingCheckTime (other.m_agingCheckTime),
    m_minRtt (other.m_minRtt),
    m_ecnSampleMin (other.m_ecnSampleMin),
    m_ecnPortionLow (other.m_ecnPortionLow),
    m_ecnPortionHigh (other.m_ecnPortionHigh),
    m_flowRetransHigh (other.m_flowRetransHigh),
    m_flowRetransVeryHigh (other.m_flowRetransVeryHigh),
    m_flowTimeoutCount (other.m_flowTimeoutCount),
    m_betterPathEcnThresh (other.m_betterPathEcnThresh),
    m_betterPathRttThresh (other.m_betterPathRttThresh),
    m_pathChangePoss (other.m_pathChangePoss)
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
        .AddAttribute ("MinRTT", "Min RTT used to judge a good path",
                      TimeValue (MicroSeconds(50)),
                      MakeTimeAccessor (&Ipv4TLB::m_minRtt),
                      MakeTimeChecker ())
        .AddAttribute ("BetterPathRTTThresh", "RTT Threshold used to judge one path is better than another",
                      TimeValue (MicroSeconds (300)),
                      MakeTimeAccessor (&Ipv4TLB::m_betterPathRttThresh),
                      MakeTimeChecker ())
        .AddAttribute ("ChangePathPoss", "Possibility to change the path",
                      UintegerValue (50),
                      MakeUintegerAccessor (&Ipv4TLB::m_pathChangePoss),
                      MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("T1", "The path aging time interval",
                      TimeValue (MicroSeconds (320)),
                      MakeTimeAccessor (&Ipv4TLB::m_T1),
                      MakeTimeChecker ())
        .AddAttribute ("ECNPortionLow", "The ECN portion used in judging a good path",
                      DoubleValue (0.3),
                      MakeDoubleAccessor (&Ipv4TLB::m_ecnPortionLow),
                      MakeDoubleChecker<double> (0.0))
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
Ipv4TLB::GetPath (uint32_t flowId, Ipv4Address daddr)
{
    if (!m_agingEvent.IsRunning ())
    {
        m_agingEvent = Simulator::Schedule (m_agingCheckTime, &Ipv4TLB::PathAging, this);
    }

    uint32_t destTor = 0;
    if (!Ipv4TLB::FindTorId (daddr, destTor))
    {
        NS_LOG_ERROR ("Cannot find dest tor id based on the given dest address");
        return 0;
    }

    std::map<uint32_t, TLBFlowInfo>::iterator flowItr = m_flowInfo.find (flowId);

    // First check if the flow is a new flow
    if (flowItr == m_flowInfo.end ())
    {
        // New flow
        uint32_t newPath = 0;
        if (!Ipv4TLB::WhereToChange (destTor, newPath, false, 0))
        {
            newPath = Ipv4TLB::SelectRandomPath (destTor);
        }
        /*std::cout <<flowId << " - " << newPath << std::endl;*/
        Ipv4TLB::UpdateFlowPath (flowId, newPath);
        Ipv4TLB::AssignFlowToPath (flowId, destTor, newPath);
        return newPath;
    }
    else
    {
        // Old flow
        uint32_t oldPath = (flowItr->second).path;
        if (0 == 1 && ((flowItr->second).retransmissionSize > m_flowRetransVeryHigh
                || (flowItr->second).timeoutCount >= 1))
        {
            std::cout << "!!!" << std::endl;
            uint32_t newPath = 0;
            if (Ipv4TLB::WhereToChange (destTor, newPath, true, oldPath))
            {
                if (newPath == oldPath)
                {
                    return oldPath;
                }

                // Change path
                Ipv4TLB::UpdateFlowPath (flowId, newPath);
                Ipv4TLB::RemoveFlowFromPath (flowId, destTor, oldPath);
                Ipv4TLB::AssignFlowToPath (flowId, destTor, newPath);
                return newPath;

            }
            else
            {
                return Ipv4TLB::SelectRandomPath (destTor);
            }
        }
        else if (Ipv4TLB::JudgePath (destTor, oldPath).pathType == BadPath
                && (flowItr->second).size >= m_S
                && ((static_cast<double> ((flowItr->second).ecnSize) / (flowItr->second).size > m_ecnPortionHigh && Simulator::Now () - (flowItr->second).timeStamp >= m_T) || (flowItr->second).retransmissionSize > m_flowRetransHigh)
                && Simulator::Now() - (flowItr->second).tryChangePath > MicroSeconds (100))
        {
            if (rand () % RANDOM_BASE < static_cast<int>(RANDOM_BASE - m_pathChangePoss))
            {
                (flowItr->second).tryChangePath = Simulator::Now ();
                return oldPath;
            }
            std::cout << "!!!" << std::endl;
            uint32_t newPath = 0;
            if (Ipv4TLB::WhereToChange (destTor, newPath, true, oldPath))
            {
                if (newPath == oldPath)
                {
                    return oldPath;
                }

                // Change path
                Ipv4TLB::UpdateFlowPath (flowId, newPath);
                Ipv4TLB::RemoveFlowFromPath (flowId, destTor, oldPath);
                Ipv4TLB::AssignFlowToPath (flowId, destTor, newPath);
                return newPath;
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
Ipv4TLB::FlowFinish (uint32_t flowId, Ipv4Address daddr, uint32_t path)
{
    uint32_t destTor = 0;
    if (!Ipv4TLB::FindTorId (daddr, destTor))
    {
        NS_LOG_ERROR ("Cannot find dest tor id based on the given dest address");
        return;
    }
    Ipv4TLB::RemoveFlowFromPath (flowId, destTor, path);

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
        bool notChangePath = Ipv4TLB::UpdateFlowInfo (flowId, path, size, withECN);
        if (!notChangePath)
        {
            NS_LOG_LOGIC ("The flow has changed the path");
        }
    }
    Ipv4TLB::UpdatePathInfo (destTorId, path, size, withECN, rtt);
}

bool
Ipv4TLB::UpdateFlowInfo (uint32_t flowId, uint32_t path, uint32_t size, bool withECN)
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
    if (rtt < pathInfo.minRtt)
    {
        pathInfo.minRtt = rtt;
    }
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
Ipv4TLB::UpdateFlowPath (uint32_t flowId, uint32_t path)
{
    TLBFlowInfo flowInfo;
    flowInfo.path = path;
    flowInfo.size = 0;
    flowInfo.ecnSize = 0;
    flowInfo.sendSize = 0;
    flowInfo.retransmissionSize = 0;
    flowInfo.timeoutCount = 0;
    flowInfo.timeStamp = Simulator::Now ();
    flowInfo.tryChangePath = Simulator::Now ();
    m_flowInfo[flowId] = flowInfo;
}

TLBPathInfo
Ipv4TLB::GetInitPathInfo (uint32_t path)
{
    TLBPathInfo pathInfo;
    pathInfo.pathId = path;
    pathInfo.size = 3;
    pathInfo.ecnSize = 1;
    pathInfo.minRtt = m_betterPathRttThresh + MicroSeconds (100);
    pathInfo.isRetransmission = false;
    pathInfo.isHighRetransmission = false;
    pathInfo.isTimeout = false;
    pathInfo.isVeryTimeout = false;
    pathInfo.isProbingTimeout = false;
    pathInfo.flowCounter = 0; // XXX Notice the flow count will be update using Add/Remove Flow To/From Path method
    pathInfo.timeStamp1 = Simulator::Now ();
    pathInfo.timeStamp2 = Simulator::Now ();

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
Ipv4TLB::WhereToChange (uint32_t destTor, uint32_t &newPath, bool hasOldPath, uint32_t oldPath)
{
    std::map<uint32_t, std::vector<uint32_t> >::iterator itr = m_availablePath.find (destTor);

    if (itr == m_availablePath.end ())
    {
        NS_LOG_ERROR ("Cannot find available paths");
        return false;
    }

    std::vector<uint32_t>::iterator vectorItr = (itr->second).begin ();

    /*newPath = (itr->second)[rand () % (itr->second).size ()];*/
    /*return true;*/

    // Firstly, checking good path
    uint32_t minCounter = std::numeric_limits<uint32_t>::max ();
    for ( ; vectorItr != (itr->second).end (); ++vectorItr)
    {
        uint32_t pathId = *vectorItr;
        uint32_t randomNumber = rand () % RANDOM_BASE;
        struct PathInfo pathInfo = JudgePath (destTor, pathId);
        if (pathInfo.pathType == GoodPath
            /*&& pathInfo.counter < minCounter)*/
            && randomNumber < minCounter)
        {
            newPath = pathId;
            /*minCounter = pathInfo.counter;*/
            minCounter = randomNumber;
        }
    }

    /*if (minCounter <= m_K)*/
    if (minCounter < RANDOM_BASE)
    {
        NS_LOG_LOGIC ("Find Good Path: " << newPath);
        return true;
    }

    // Secondly, checking gray path
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
    }

    minCounter = std::numeric_limits<uint32_t>::max ();
    vectorItr = (itr->second).begin ();
    for ( ; vectorItr != (itr->second).end (); ++vectorItr)
    {
        uint32_t pathId = *vectorItr;
        uint32_t randomNumber = rand () % RANDOM_BASE;
        struct PathInfo pathInfo = JudgePath (destTor, pathId);
        if (pathInfo.pathType == GreyPath
            /*&& pathInfo.counter < minCounter*/
            && randomNumber < minCounter
            && Ipv4TLB::PathLIsBetterR (pathInfo, originalPath))
        {
            newPath = pathId;
            /*minCounter = pathInfo.counter;*/
            minCounter = randomNumber;
        }
    }

    /*if (minCounter <= m_K)*/
    if (minCounter < RANDOM_BASE)
    {
        NS_LOG_LOGIC ("Find Grey Path: " << newPath);
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
            newPath = pathId;
            NS_LOG_LOGIC ("Find Bad Path: " << newPath);
            return true;
        }
    }

    // Thirdly, indicating no paths available
    NS_LOG_LOGIC ("No Path Returned");
    return false;
}

uint32_t
Ipv4TLB::SelectRandomPath (uint32_t destTor)
{
    std::map<uint32_t, std::vector<uint32_t> >::iterator itr = m_availablePath.find (destTor);

    if (itr == m_availablePath.end ())
    {
        NS_LOG_ERROR ("Cannot find available paths");
        return false;
    }

    std::vector<uint32_t>::iterator vectorItr = (itr->second).begin ();
    std::vector<uint32_t> availablePaths;
    for ( ; vectorItr != (itr->second).end (); ++vectorItr)
    {
        uint32_t pathId = *vectorItr;
        struct PathInfo pathInfo = JudgePath (destTor, pathId);
        if (pathInfo.pathType == GoodPath || pathInfo.pathType == GreyPath || pathInfo.pathType == BadPath)
        {
            availablePaths.push_back (pathId);
        }
    }

    uint32_t newPath;
    if (!availablePaths.empty ())
    {
        newPath = availablePaths[rand() % availablePaths.size ()];
    }
    else
    {
        newPath = (itr->second)[rand() % (itr->second).size ()];
    }
    NS_LOG_LOGIC ("Random selection return path: " << newPath);
    return newPath;
}

struct PathInfo
Ipv4TLB::JudgePath (uint32_t destTor, uint32_t pathId)
{
    std::pair<uint32_t, uint32_t> key = std::make_pair (destTor, pathId);
    std::map<std::pair<uint32_t, uint32_t>, TLBPathInfo>::iterator itr = m_pathInfo.find (key);

    struct PathInfo path;
    if (itr == m_pathInfo.end ())
    {
        path.pathType = GreyPath;
        path.rttMin = m_betterPathRttThresh + MicroSeconds (100);
        path.ecnPortion = 0.3;
        path.counter = 0;
        return path;
    }
    TLBPathInfo pathInfo = itr->second;
    path.rttMin = pathInfo.minRtt;
    path.ecnPortion = static_cast<double>(pathInfo.ecnSize) / pathInfo.size;
    path.counter = pathInfo.flowCounter;
    if ((pathInfo.minRtt < m_minRtt
            && (pathInfo.size > m_ecnSampleMin && static_cast<double>(pathInfo.ecnSize) / pathInfo.size < m_ecnPortionLow))
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

    if ((static_cast<double>(pathInfo.ecnSize) / pathInfo.size > m_ecnPortionHigh
            && Simulator::Now () - pathInfo.timeStamp1 > m_T1 / 2 )
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
    if (pathR.ecnPortion - pathL.ecnPortion >= m_betterPathEcnThresh
        && pathR.rttMin - pathL.rttMin >= m_betterPathRttThresh)
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
            (itr->second).minRtt = Seconds (666);
            (itr->second).timeStamp1 = Simulator::Now ();
        }
        if (Simulator::Now () - (itr->second).timeStamp2 > m_T2)
        {
            (itr->second).isRetransmission = false;
            (itr->second).isHighRetransmission = false;
            (itr->second).isTimeout = false;
            (itr->second).isVeryTimeout = false;
            (itr->second).isProbingTimeout = false;
            (itr->second).timeStamp2 = Simulator::Now ();
        }
    }

    m_agingEvent = Simulator::Schedule (m_agingCheckTime, &Ipv4TLB::PathAging, this);
}

}
