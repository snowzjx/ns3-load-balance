/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ipv4-clove.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv4Clove");

NS_OBJECT_ENSURE_REGISTERED (Ipv4Clove);

Ipv4Clove::Ipv4Clove () :
    m_flowletTimeout (MicroSeconds (200)),
    m_runMode (CLOVE_RUNMODE_EDGE_FLOWLET),
    m_halfRTT (MicroSeconds (40)),
    m_disToUncongestedPath (false)
{
    NS_LOG_FUNCTION (this);
}

Ipv4Clove::Ipv4Clove (const Ipv4Clove &other) :
    m_flowletTimeout (other.m_flowletTimeout),
    m_runMode (other.m_runMode),
    m_halfRTT (other.m_halfRTT),
    m_disToUncongestedPath (other.m_disToUncongestedPath)
{
    NS_LOG_FUNCTION (this);
}

TypeId
Ipv4Clove::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::Ipv4Clove")
        .SetParent<Object> ()
        .SetGroupName ("Clove")
        .AddConstructor<Ipv4Clove> ()
        .AddAttribute ("FlowletTimeout", "FlowletTimeout",
                       TimeValue (MicroSeconds (40)),
                       MakeTimeAccessor (&Ipv4Clove::m_flowletTimeout),
                       MakeTimeChecker ())
        .AddAttribute ("RunMode", "RunMode",
                       UintegerValue (0),
                       MakeUintegerAccessor (&Ipv4Clove::m_runMode),
                       MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("HalfRTT", "HalfRTT",
                       TimeValue (MicroSeconds (40)),
                       MakeTimeAccessor (&Ipv4Clove::m_halfRTT),
                       MakeTimeChecker ())
        .AddAttribute ("DisToUncongestedPath", "DisToUncongestedPath",
                       BooleanValue (false),
                       MakeBooleanAccessor (&Ipv4Clove::m_disToUncongestedPath),
                       MakeBooleanChecker ())
    ;

    return tid;
}

void
Ipv4Clove::AddAddressWithTor (Ipv4Address address, uint32_t torId)
{
    m_ipTorMap[address] = torId;
}

void
Ipv4Clove::AddAvailPath (uint32_t destTor, uint32_t path)
{
    std::map<uint32_t, std::vector<uint32_t> >::iterator itr = m_availablePath.find (destTor);
    if (itr == m_availablePath.end ())
    {
        std::vector<uint32_t> paths;
        m_availablePath[destTor] = paths;
        itr = m_availablePath.find (destTor);
    }
    (itr->second).push_back (path);

    std::pair<uint32_t, uint32_t> key = std::make_pair (destTor, path);
    m_pathWeight[key] = 1;
}

uint32_t
Ipv4Clove::GetPath (uint32_t flowId, Ipv4Address saddr, Ipv4Address daddr)
{
    uint32_t destTor = 0;
    if (!Ipv4Clove::FindTorId (daddr, destTor))
    {
        NS_LOG_ERROR ("Cannot find dest tor id based on the given dest address");
        return 0;
    }

    uint32_t sourceTor = 0;
    if (!Ipv4Clove::FindTorId (saddr, sourceTor))
    {
        NS_LOG_ERROR ("Cannot find source tor id based on the given source address");
    }

    struct CloveFlowlet flowlet;
    std::map<uint32_t, CloveFlowlet>::iterator flowletItr = m_flowletMap.find (flowId);
    if (flowletItr != m_flowletMap.end ())
    {
        flowlet = flowletItr->second;
    }
    else
    {
        flowlet.path = Ipv4Clove::CalPath (destTor);
    }

    if (Simulator::Now () - flowlet.lastSeen >= m_flowletTimeout)
    {
        flowlet.path = Ipv4Clove::CalPath (destTor);
    }

    flowlet.lastSeen = Simulator::Now ();
    m_flowletMap[flowId] = flowlet;

    return flowlet.path;
}


bool
Ipv4Clove::FindTorId (Ipv4Address daddr, uint32_t &destTorId)
{
    std::map<Ipv4Address, uint32_t>::iterator torItr = m_ipTorMap.find (daddr);

    if (torItr == m_ipTorMap.end ())
    {
        return false;
    }
    destTorId = torItr->second;
    return true;
}

uint32_t
Ipv4Clove::CalPath (uint32_t destTor)
{
    std::map<uint32_t, std::vector<uint32_t> >::iterator itr = m_availablePath.find (destTor);
    if (itr == m_availablePath.end ())
    {
        return 0;
    }
    std::vector<uint32_t> paths = itr->second;
    if (m_runMode == CLOVE_RUNMODE_EDGE_FLOWLET)
    {
        return paths[rand() % paths.size ()];
    }
    else if (m_runMode == CLOVE_RUNMODE_ECN)
    {
        double r = ((double) rand () / RAND_MAX);
        std::vector<uint32_t>::iterator itr = paths.begin ();
        double weightSum = 0.0;
        for ( ; itr != paths.end (); ++itr)
        {
            uint32_t path = *itr;
            std::pair<uint32_t, uint32_t> key = std::make_pair (destTor, path);
            std::map<std::pair<uint32_t, uint32_t>, double>::iterator pathWeightItr = m_pathWeight.find (key);
            if (pathWeightItr != m_pathWeight.end ())
            {
                weightSum += pathWeightItr->second;
            }
            if (r <= (weightSum / (double) paths.size ()))
            {
                return path;
            }
        }
        return 0;
    }
    else if (m_runMode == CLOVE_RUNMODE_INT)
    {
        return 0;
    }
    return 0;
}

void
Ipv4Clove::FlowRecv (uint32_t path, Ipv4Address daddr, bool withECN)
{
    if (!withECN)
    {
        return;
    }

    uint32_t destTor = 0;
    if (!Ipv4Clove::FindTorId (daddr, destTor))
    {
        NS_LOG_ERROR ("Cannot find dest tor id based on the given dest address");
        return;
    }

    std::map<uint32_t, std::vector<uint32_t> >::iterator itr = m_availablePath.find (destTor);
    if (itr == m_availablePath.end ())
    {
        return;
    }

    std::vector<uint32_t> paths = itr->second;
    std::pair<uint32_t, uint32_t> key = std::make_pair (destTor, path);

    std::map<std::pair<uint32_t, uint32_t>, Time>::iterator ecnItr = m_pathECNSeen.find (key);

    if (ecnItr == m_pathECNSeen.end ()
            || Simulator::Now () - ecnItr->second >= m_halfRTT)
    {
        // Update the weight
        m_pathECNSeen[key] = Simulator::Now ();

        double originalPathWeight = 1.0;
        std::map<std::pair<uint32_t, uint32_t>, double>::iterator weightItr = m_pathWeight.find (key);
        if (weightItr != m_pathWeight.end ())
        {
            originalPathWeight = weightItr->second;
        }

        m_pathWeight[key] = 0.67 * originalPathWeight;

        std::vector<uint32_t>::iterator pathItr = paths.begin ();

        uint32_t uncongestedPathCount = 0;

        for ( ; pathItr != paths.end (); ++pathItr)
        {
            if (*pathItr != path)
            {
                std::pair<uint32_t, uint32_t> uKey = std::make_pair (destTor, *pathItr);
                std::map<std::pair<uint32_t, uint32_t>, Time>::iterator uEcnItr = m_pathECNSeen.find (uKey);
                if (!m_disToUncongestedPath ||
                        (m_disToUncongestedPath && Simulator::Now () - uEcnItr->second < m_halfRTT))
                {
                    uncongestedPathCount ++;
                }
            }
        }

        if (uncongestedPathCount == 0)
        {
            m_pathWeight[key] = originalPathWeight;
            return;
        }

        pathItr = paths.begin ();
        for ( ; pathItr != paths.end (); ++pathItr)
        {
            if (*pathItr != path)
            {
                std::pair<uint32_t, uint32_t> uKey = std::make_pair (destTor, *pathItr);
                std::map<std::pair<uint32_t, uint32_t>, Time>::iterator uEcnItr = m_pathECNSeen.find (uKey);
                if (!m_disToUncongestedPath ||
                        (m_disToUncongestedPath && Simulator::Now () - uEcnItr->second < m_halfRTT))
                {
                    double uPathWeight = 1.0;
                    std::map<std::pair<uint32_t, uint32_t>, double>::iterator uPathWeightItr = m_pathWeight.find (uKey);
                    if (uPathWeightItr != m_pathWeight.end ())
                    {
                        uPathWeight = uPathWeightItr->second;
                    }
                    m_pathWeight[uKey] = uPathWeight + (0.33 * originalPathWeight) / uncongestedPathCount;
                }
            }
        }
    }
}

}
