/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ipv4-tlb.h"

#include "ns3/log.h"
#include "ns3/node.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv4TLB");

NS_OBJECT_ENSURE_REGISTERED (Ipv4TLB);

Ipv4TLB::Ipv4TLB ():
    m_S (1000000)
{
    NS_LOG_FUNCTION (this);
}

Ipv4TLB::Ipv4TLB (const Ipv4TLB &other):
    m_S (other.m_S)
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
    m_IpTorMap[address] = torId;
}

uint32_t
Ipv4TLB::GetPath (void)
{
    return 0;
}

void
Ipv4TLB::FlowSent (uint32_t flowId, Ipv4Address daddr, uint32_t pathId)
{
    NS_LOG_FUNCTION (this << flowId << daddr << pathId << isRetransmission);

    uint32_t destTorId = 0;
    std::map<Ipv4Address, uint32_t>::iterator torItr = m_ipTorMap.find (daddr);
    if (torItr == m_ipTorMap.end ())
    {
        NS_LOG_ERROR (this << "Cannot find tor id based on destination address");
        return;
    }
    else
    {
        destTorId = torItr->second;
    }

    bool hasChangedPath = false;
    uint32_t oldPath = 0;

    std::map<uint32_t, TLBFlowInfo>::iterator itr = m_flowInfo.find (flowId);
    if (itr == m_flowInfo.end ())
    {
        TLBFlowInfo flowInfo;
        flowInfo.flowId = flowId;
        flowInfo.path = pathId;
        // A new flow will not be retransmission or has time out event
        flowInfo.retransmissionSize = 0;
        flowInfo.isTimeout = false;
        m_flowInfo[flowId] = flowInfo;
    }
    else
    {
        if ((itr->second).path != pathId)
        {
            // A flow has changed the path
            hasChangedPath = true;
            oldPath = (itr->second).path;
            (itr->second).path = pathId;
            (itr->second).size = 0;
            (itr->second).retransmissionSize = 0;
            (itr->second).isTimeout = false;
        }
        (itr->second).size += size;
        if (isRetransmission)
        {
            (itr->second).retransmissionSize += size;
        }
    }

    std::map<uint32_t, std::map<uint32_t, TLBPathInfo> >::iterator torPathMapItr = m_pathInfo.find (destTorId);
    if (torPathMapItr == m_pathInfo.end ())
    {
        std::map<uint32_t, TLBPathInfo> pathMap;
        m_pathInfo[destTorId] = pathMap;
        torPathMapItr = m_pathInfo.find (destTorId);
    }

    std::map<uint32_t, TLBPathInfo>::iterator pathMapItr = (torPathMapItr->second).find (pathId);
    if (pathMapItr == (torPathMapItr->second).end ())
    {
        TLBPathInfo pathInfo;
        pathInfo.pathId = pathId;
        pathInfo.size = 0;
        pathInfo.ecnSize = 0;
        pathInfo.minRtt = Seconds (1);
        pathInfo.isRetransmission = false;
        pathInfo.isTimeout = false;
        pathInfo.isProbingTimeout = false;
        pathInfo.flowCounter = 1;
        (torPathMapItr->second)[pathId] = pathInfo;
    }
    else
    {

    }
}

void
Ipv4TLB::FlowTimeout (uint32_t flowId)
{
    NS_LOG_FUNCTION (this << flowId);
    std::map<uint32_t, TLBFlowInfo>::iterator itr = m_flowInfo.find (flowId);

    if (itr == m_flowInfo.end ())
    {
        NS_LOG_ERROR ("Cannot timeout a flow that has not been recorded before");
        return;
    }

    (itr->second).isTimeout = true;
}

void
Ipv4TLB::SetNode (Ptr<Node> node)
{
    m_node = node;
}

}
