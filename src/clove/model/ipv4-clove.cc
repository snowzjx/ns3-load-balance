/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ipv4-clove.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv4Clove");

NS_OBJECT_ENSURE_REGISTERED (Ipv4Clove);

Ipv4Clove::Ipv4Clove () :
    m_flowletTimeout (MicroSeconds (200)),
    m_runMode (CLOVE_RUNMODE_EDGE_FLOWLET)
{
    NS_LOG_FUNCTION (this);
}

Ipv4Clove::Ipv4Clove (const Ipv4Clove &other) :
    m_flowletTimeout (other.m_flowletTimeout),
    m_runMode (other.m_runMode)
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

    struct Flowlet flowlet;
    std::map<uint32_t, Flowlet>::iterator flowletItr = m_flowletMap.find (flowId);
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
    else
    {
        // TODO CLOVE ECN and CLOVE INT
        return 0;
    }
}

}
