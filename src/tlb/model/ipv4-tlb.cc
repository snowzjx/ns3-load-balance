/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ipv4-tlb.h"

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv4TLB");

NS_OBJECT_ENSURE_REGISTERED (Ipv4TLB);

Ipv4TLB::Ipv4TLB ():
    m_S (1000000),
    m_K (5),
    m_T1 (MicroSeconds (200)),
    m_T2 (Seconds (5)),
    m_agingCheckTime (MicroSeconds (100)),
    m_minRtt (MicroSeconds (150)),
    m_ecnSampleMin (14000),
    m_ecnPortionLow (0.1),
    m_ecnPortionHigh (0.5),
    m_flowRetransHigh (14000),
    m_betterPathEcnThresh (0.1),
    m_betterPathRttThresh (MicroSeconds (100))
{
    NS_LOG_FUNCTION (this);
}

Ipv4TLB::Ipv4TLB (const Ipv4TLB &other):
    m_S (other.m_S),
    m_K (other.m_K),
    m_T1 (other.m_T1),
    m_T2 (other.m_T2),
    m_agingCheckTime (other.m_agingCheckTime),
    m_minRtt (other.m_minRtt),
    m_ecnSampleMin (other.m_ecnSampleMin),
    m_ecnPortionLow (other.m_ecnPortionLow),
    m_ecnPortionHigh (other.m_ecnPortionHigh),
    m_flowRetransHigh (other.m_flowRetransHigh),
    m_betterPathEcnThresh (other.m_betterPathEcnThresh),
    m_betterPathRttThresh (other.m_betterPathRttThresh)
{
    NS_LOG_FUNCTION (this);
}

TypeId
Ipv4TLB::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::Ipv4TLB")
        .SetParent<Object> ()
        .SetGroupName ("TLB")
        .AddConstructor<Ipv4TLB> ();

    return tid;
}

void
Ipv4TLB::AddAddressWithTor (Ipv4Address address, uint32_t torId)
{
    m_ipTorMap[address] = torId;
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
        Ipv4TLB::UpdateFlowPath (flowId, newPath);
        Ipv4TLB::AssignFlowToPath (flowId, destTor, newPath);
        return newPath;
    }
    else
    {
        // Old flow
        uint32_t oldPath = (flowItr->second).path;
        if ((flowItr->second).retransmissionSize > m_flowRetransHigh
                || (flowItr->second).isTimeout)
        {
            uint32_t newPath = 0;
            if (Ipv4TLB::WhereToChange (destTor, newPath, true, oldPath))
            {
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
        else if (Ipv4TLB::JudgePath (destTor, oldPath).pathType == BadPath && (flowItr->second).size >= m_S)
        {
            uint32_t newPath = 0;
            if (Ipv4TLB::WhereToChange (destTor, newPath, true, oldPath))
            {
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
        bool notChangePath = Ipv4TLB::RetransFlow (flowId, path, size, needRetransPath);
        if (!notChangePath)
        {
            NS_LOG_LOGIC ("Cannot send flow on the expired path");
            return;
        }
        if (needRetransPath)
        {
            Ipv4TLB::RetransPath (destTor, path);
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

    bool notChangePath = Ipv4TLB::TimeoutFlow (flowId, path);
    if (!notChangePath)
    {
        NS_LOG_LOGIC ("The flow has changed the path");
    }
    Ipv4TLB::TimeoutPath (destTor, path, false);
}

void
Ipv4TLB::ProbeRecv (uint32_t path, Ipv4Address daddr, uint32_t size, bool withECN, Time rtt)
{
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

    Ipv4TLB::TimeoutPath (path, destTor, true);
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
        pathInfo.pathId = path;
        pathInfo.size = 1;
        pathInfo.ecnSize = 0;
        pathInfo.minRtt = Seconds (666);
        pathInfo.isRetransmission = false;
        pathInfo.isTimeout = false;
        pathInfo.isProbingTimeout = false;
        pathInfo.flowCounter = 0; // XXX Notice the flow count will be update using Add/Remove Flow To/From Path method
        pathInfo.timeStamp1 = Simulator::Now ();
        pathInfo.timeStamp2 = Simulator::Now ();
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
Ipv4TLB::TimeoutFlow (uint32_t flowId, uint32_t path)
{
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
    (itr->second).isTimeout = true;
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
Ipv4TLB::RetransFlow (uint32_t flowId, uint32_t path, uint32_t size, bool &needRetranPath)
{
    needRetranPath = false;
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
    (itr->second).retransmissionSize += size;
    if ((itr->second).retransmissionSize > m_flowRetransHigh)
    {
        needRetranPath = true;
    }
    return true;
}


void
Ipv4TLB::TimeoutPath (uint32_t destTor, uint32_t path, bool isProbing)
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
    }
    else
    {
        (itr->second).isProbingTimeout = true;
    }
}

void
Ipv4TLB::RetransPath (uint32_t destTor, uint32_t path)
{
    std::pair<uint32_t, uint32_t> key = std::make_pair(destTor, path);
    std::map<std::pair<uint32_t, uint32_t>, TLBPathInfo>::iterator itr = m_pathInfo.find (key);
    if (itr == m_pathInfo.end ())
    {
        NS_LOG_ERROR ("Cannot timeout a non-existing path");
        return;
    }
    (itr->second).isRetransmission = true;
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
    flowInfo.isTimeout = false;

    m_flowInfo[flowId] = flowInfo;
}

void
Ipv4TLB::AssignFlowToPath (uint32_t flowId, uint32_t destTor, uint32_t path)
{
    std::pair<uint32_t, uint32_t> key = std::make_pair(destTor, path);
    std::map<std::pair<uint32_t, uint32_t>, TLBPathInfo>::iterator itr = m_pathInfo.find (key);
    if (itr == m_pathInfo.end ())
    {
        NS_LOG_ERROR ("Cannot assign flow to a non-existing path");
        return;
    }
    (itr->second).flowCounter ++;
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

    // Firstly, checking good path
    uint32_t minCounter = std::numeric_limits<uint32_t>::max ();
    for ( ; vectorItr != (itr->second).end (); ++vectorItr)
    {
        uint32_t pathId = *vectorItr;
        struct PathInfo pathInfo = JudgePath (destTor, pathId);
        if (pathInfo.pathType == GoodPath && pathInfo.counter < minCounter)
        {
            newPath = pathId;
            minCounter = pathInfo.counter;
        }
    }

    if (minCounter <= m_K)
    {
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
        struct PathInfo pathInfo = JudgePath (destTor, pathId);
        if (pathInfo.pathType == GreyPath
            && pathInfo.counter < minCounter
            && Ipv4TLB::PathLIsBetterR (pathInfo, originalPath))
        {
            newPath = pathId;
            minCounter = pathInfo.counter;
        }
    }

    if (minCounter <= m_K)
    {
        return true;
    }

    // Thirdly, indicating no paths available
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
        if (pathInfo.pathType == GoodPath || pathInfo.pathType == GreyPath)
        {
            availablePaths.push_back (pathId);
        }
    }

    if (!availablePaths.empty ())
    {
        return availablePaths[rand() % availablePaths.size ()];
    }
    else
    {
        return (itr->second)[rand() % (itr->second).size ()];
    }
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
        path.rttMin = Seconds (666);
        path.ecnPortion = 1;
        path.counter = std::numeric_limits<uint32_t>::max ();
        return path;
    }
    TLBPathInfo pathInfo = itr->second;
    path.rttMin = pathInfo.minRtt;
    path.ecnPortion = static_cast<double>(pathInfo.ecnSize) / pathInfo.size;
    path.counter = pathInfo.flowCounter;
    if (pathInfo.minRtt < m_minRtt
            || (pathInfo.size > m_ecnSampleMin && static_cast<double>(pathInfo.ecnSize) / pathInfo.size < m_ecnPortionLow))
    {
        path.pathType = GoodPath;
        return path;
    }
    if (static_cast<double>(pathInfo.ecnSize) / pathInfo.size > m_ecnPortionHigh)
    {
        path.pathType = BadPath;
        return path;
    }
    if (pathInfo.isRetransmission
            || pathInfo.isTimeout
            || pathInfo.isProbingTimeout)
    {
        path.pathType = FailPath;
        return path;
    }
    path.pathType = GreyPath;
    return path;
}

bool
Ipv4TLB::PathLIsBetterR (struct PathInfo pathL, struct PathInfo pathR)
{
    if (pathR.ecnPortion - pathL.ecnPortion > m_betterPathEcnThresh
        && pathR.rttMin - pathL.rttMin > m_betterPathRttThresh)
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
    std::map<std::pair<uint32_t, uint32_t>, TLBPathInfo>::iterator itr = m_pathInfo.begin ();
    for ( ; itr != m_pathInfo.end (); ++itr)
    {
        if (Simulator::Now() - (itr->second).timeStamp1 > m_T1)
        {
            (itr->second).size = 1;
            (itr->second).ecnSize = 0;
            (itr->second).minRtt = Seconds (666);
            (itr->second).flowCounter = 0;
            (itr->second).timeStamp1 = Simulator::Now ();
        }
        if (Simulator::Now () - (itr->second).timeStamp2 > m_T2)
        {
            (itr->second).isRetransmission = false;
            (itr->second).isTimeout = false;
            (itr->second).isProbingTimeout = false;
            (itr->second).timeStamp2 = Simulator::Now ();
        }
    }

    m_agingEvent = Simulator::Schedule (m_agingCheckTime, &Ipv4TLB::PathAging, this);
}

}
