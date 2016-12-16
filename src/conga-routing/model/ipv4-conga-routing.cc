/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ipv4-conga-routing.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/net-device.h"
#include "ns3/channel.h"
#include "ns3/node.h"
#include "ns3/flow-id-tag.h"
#include "ipv4-conga-tag.h"

#include <algorithm>

#define LOOPBACK_PORT 0

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Ipv4CongaRouting");

NS_OBJECT_ENSURE_REGISTERED (Ipv4CongaRouting);

Ipv4CongaRouting::Ipv4CongaRouting ():
    // Parameters
    m_isLeaf (false),
    m_leafId (0),
    m_tdre (MicroSeconds(200)),
    m_alpha (0.2),
    m_C (DataRate("1Gbps")),
    m_Q (3),
    m_agingTime (MilliSeconds (10)),
    m_flowletTimeout (MicroSeconds(50)), // The default value of flowlet timeout is small for experimental purpose
    m_ecmpMode (false),
    // Variables
    m_feedbackIndex (0),
    m_dreEvent (),
    m_agingEvent (),
    m_ipv4 (0)
{
  NS_LOG_FUNCTION (this);
}

Ipv4CongaRouting::~Ipv4CongaRouting ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
Ipv4CongaRouting::GetTypeId (void)
{
  static TypeId tid = TypeId("ns3::Ipv4CongaRouting")
      .SetParent<Object>()
      .SetGroupName ("Internet")
      .AddConstructor<Ipv4CongaRouting> ();

  return tid;
}

void
Ipv4CongaRouting::SetLeafId (uint32_t leafId)
{
  m_isLeaf = true;
  m_leafId = leafId;
}

void
Ipv4CongaRouting::SetFlowletTimeout (Time timeout)
{
  m_flowletTimeout = timeout;
}

void
Ipv4CongaRouting::SetAlpha (double alpha)
{
  m_alpha = alpha;
}

void
Ipv4CongaRouting::SetTDre (Time time)
{
  m_tdre = time;
}

void
Ipv4CongaRouting::SetLinkCapacity (DataRate dataRate)
{
  m_C = dataRate;
}

void
Ipv4CongaRouting::SetLinkCapacity (uint32_t interface, DataRate dataRate)
{
  m_Cs[interface] = dataRate;
}

void
Ipv4CongaRouting::SetQ (uint32_t q)
{
  m_Q = q;
}

void
Ipv4CongaRouting::AddAddressToLeafIdMap (Ipv4Address addr, uint32_t leafId)
{
  m_ipLeafIdMap[addr] = leafId;
}

void
Ipv4CongaRouting::EnableEcmpMode ()
{
  m_ecmpMode = true;
}

void
Ipv4CongaRouting::InitCongestion (uint32_t leafId, uint32_t port, uint32_t congestion)
{
  std::map<uint32_t, std::map<uint32_t, std::pair<Time, uint32_t> > >::iterator itr =
      m_congaToLeafTable.find(leafId);
  if (itr != m_congaToLeafTable.end ())
  {
    (itr->second)[port] = std::make_pair(Simulator::Now (), congestion);
  }
  else
  {
    std::map<uint32_t, std::pair<Time, uint32_t> > newMap;
    newMap[port] = std::make_pair(Simulator::Now (), congestion);
    m_congaToLeafTable[leafId] = newMap;
  }
}

void
Ipv4CongaRouting::AddRoute (Ipv4Address network, Ipv4Mask networkMask, uint32_t port)
{
  NS_LOG_LOGIC (this << " Add Conga routing entry: " << network << "/" << networkMask << " would go through port: " << port);
  CongaRouteEntry congaRouteEntry;
  congaRouteEntry.network = network;
  congaRouteEntry.networkMask = networkMask;
  congaRouteEntry.port = port;
  m_routeEntryList.push_back (congaRouteEntry);
}

std::vector<CongaRouteEntry>
Ipv4CongaRouting::LookupCongaRouteEntries (Ipv4Address dest)
{
  std::vector<CongaRouteEntry> congaRouteEntries;
  std::vector<CongaRouteEntry>::iterator itr = m_routeEntryList.begin ();
  for ( ; itr != m_routeEntryList.end (); ++itr)
  {
    if((*itr).networkMask.IsMatch(dest, (*itr).network))
    {
      congaRouteEntries.push_back (*itr);
    }
  }
  return congaRouteEntries;
}

Ptr<Ipv4Route>
Ipv4CongaRouting::ConstructIpv4Route (uint32_t port, Ipv4Address destAddress)
{
  Ptr<NetDevice> dev = m_ipv4->GetNetDevice (port);
  Ptr<Channel> channel = dev->GetChannel ();
  uint32_t otherEnd = (channel->GetDevice (0) == dev) ? 1 : 0;
  Ptr<Node> nextHop = channel->GetDevice (otherEnd)->GetNode ();
  uint32_t nextIf = channel->GetDevice (otherEnd)->GetIfIndex ();
  Ipv4Address nextHopAddr = nextHop->GetObject<Ipv4>()->GetAddress(nextIf,0).GetLocal();
  Ptr<Ipv4Route> route = Create<Ipv4Route> ();
  route->SetOutputDevice (m_ipv4->GetNetDevice (port));
  route->SetGateway (nextHopAddr);
  route->SetSource (m_ipv4->GetAddress (port, 0).GetLocal ());
  route->SetDestination (destAddress);
  return route;
}

Ptr<Ipv4Route>
Ipv4CongaRouting::RouteOutput (Ptr<Packet> packet, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{
  NS_LOG_ERROR (this << " Conga routing is not support for local routing output");
  return 0;
}

bool
Ipv4CongaRouting::RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                           UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                           LocalDeliverCallback lcb, ErrorCallback ecb)
{
  NS_LOG_LOGIC (this << " RouteInput: " << p << "Ip header: " << header);

  NS_ASSERT (m_ipv4->GetInterfaceForDevice (idev) >= 0);

  Ptr<Packet> packet = ConstCast<Packet> (p);

  Ipv4Address destAddress = header.GetDestination();

  // Conga routing only supports unicast
  if (destAddress.IsMulticast() || destAddress.IsBroadcast()) {
    NS_LOG_ERROR (this << " Conga routing only supports unicast");
    ecb (packet, header, Socket::ERROR_NOROUTETOHOST);
    return false;
  }

  // Check if input device supports IP forwarding
  uint32_t iif = m_ipv4->GetInterfaceForDevice (idev);
  if (m_ipv4->IsForwarding (iif) == false) {
    NS_LOG_ERROR (this << " Forwarding disabled for this interface");
    ecb (packet, header, Socket::ERROR_NOROUTETOHOST);
    return false;
  }

  // Packet arrival time
  Time now = Simulator::Now ();

  // Extract the flow id
  uint32_t flowId = 0;
  FlowIdTag flowIdTag;
  bool flowIdFound = packet->PeekPacketTag(flowIdTag);
  if (!flowIdFound)
  {
    NS_LOG_ERROR (this << " Conga routing cannot extract the flow id");
    ecb (packet, header, Socket::ERROR_NOROUTETOHOST);
    return false;
  }
  flowId = flowIdTag.GetFlowId ();

  std::vector<CongaRouteEntry> routeEntries = Ipv4CongaRouting::LookupCongaRouteEntries (destAddress);

  if (routeEntries.empty ())
  {
    NS_LOG_ERROR (this << " Conga routing cannot find routing entry");
    ecb (packet, header, Socket::ERROR_NOROUTETOHOST);
    return false;
  }

  // Dev use
  if (m_ecmpMode)
  {
    uint32_t selectedPort = routeEntries[flowId % routeEntries.size ()].port;
    Ptr<Ipv4Route> route = Ipv4CongaRouting::ConstructIpv4Route (selectedPort, destAddress);
    ucb (route, packet, header);
  }

  // Turn on DRE event scheduler if it is not running
  if (!m_dreEvent.IsRunning ())
  {
    NS_LOG_LOGIC (this << " Conga routing restarts dre event scheduling");
    m_dreEvent = Simulator::Schedule(m_tdre, &Ipv4CongaRouting::DreEvent, this);
  }

  // Turn on aging event scheduler if it is not running
  if (!m_agingEvent.IsRunning ())
  {
    NS_LOG_LOGIC (this << "Conga routing restarts aging event scheduling");
    m_agingEvent = Simulator::Schedule(m_agingTime / 4, &Ipv4CongaRouting::AgingEvent, this);
  }

  // First, check if this switch if leaf switch
  if (m_isLeaf)
  {
    // If the switch is leaf switch, two possible situations
    // 1. The sender is connected to this leaf switch
    // 2. The receiver is connected to this leaf switch
    // We can distinguish it by checking whether the packet has CongaTag
    Ipv4CongaTag ipv4CongaTag;
    bool found = packet->PeekPacketTag(ipv4CongaTag);

    if (!found)
    {
      uint32_t selectedPort;

      // When sending a new packet
      // Build an empty Conga header (as the packet tag)
      // Determine the port and fill the header fields

      Ipv4CongaRouting::PrintDreTable ();
      Ipv4CongaRouting::PrintCongaToLeafTable ();
      Ipv4CongaRouting::PrintFlowletTable ();

      // Determine the dest switch leaf id
      std::map<Ipv4Address, uint32_t>::iterator itr = m_ipLeafIdMap.find(destAddress);
      if (itr == m_ipLeafIdMap.end ())
      {
        NS_LOG_ERROR (this << " Conga routing cannot find leaf switch id");
        ecb (packet, header, Socket::ERROR_NOROUTETOHOST);
        return false;
      }
      uint32_t destLeafId = itr->second;

      // Check piggyback information
      std::map<uint32_t, std::map<uint32_t, FeedbackInfo> >::iterator fbItr =
          m_congaFromLeafTable.find (destLeafId);

      uint32_t fbLbTag = LOOPBACK_PORT;
      uint32_t fbMetric = 0;

      // Piggyback according to round robin and favoring those that has been changed
      if (fbItr != m_congaFromLeafTable.end ())
      {
        std::map<uint32_t, FeedbackInfo>::iterator innerFbItr = (fbItr->second).begin ();
        std::advance (innerFbItr, m_feedbackIndex++ % (fbItr->second).size ()); // round robin

        if ((innerFbItr->second).change == false)  // prefer the changed ones
        {
          for (unsigned loopIndex = 0; loopIndex < (fbItr->second).size (); loopIndex ++) // prevent infinite looping
          {
            if (++innerFbItr == (fbItr->second).end ()) {
              innerFbItr = (fbItr->second).begin (); // start from the beginning
            }
            if ((innerFbItr->second).change == true)
            {
              break;
            }
          }
        }

        fbLbTag = innerFbItr->first;
        fbMetric = (innerFbItr->second).ce;
        (innerFbItr->second).change = false;
      }

      // Port determination logic:
      // Firstly, check the flowlet table to see whether there is existing flowlet
      // If not hit, determine the port based on the congestion degree of the link

      // Flowlet table look up
      struct Flowlet *flowlet = NULL;

      // If the flowlet table entry is valid, return the port
      std::map<uint32_t, struct Flowlet *>::iterator flowletItr = m_flowletTable.find (flowId);
      if (flowletItr != m_flowletTable.end ())
      {
        flowlet = flowletItr->second;
        if (flowlet != NULL && // Impossible in normal cases
            now - flowlet->activeTime <= m_flowletTimeout)
        {
          // Do not forget to update the flowlet active time
          flowlet->activeTime = now;

          // Return the port information used for routing routine to select the port
          selectedPort = flowlet->port;

          // Construct Conga Header for the packet
          ipv4CongaTag.SetLbTag (selectedPort);
          ipv4CongaTag.SetCe (0);

          // Piggyback the feedback information
          ipv4CongaTag.SetFbLbTag (fbLbTag);
          ipv4CongaTag.SetFbMetric (fbMetric);
          packet->AddPacketTag(ipv4CongaTag);

          // Update local dre
          Ipv4CongaRouting::UpdateLocalDre (header, packet, selectedPort);

          Ptr<Ipv4Route> route = Ipv4CongaRouting::ConstructIpv4Route (selectedPort, destAddress);
          ucb (route, packet, header);

          NS_LOG_LOGIC (this << " Sending Conga on leaf switch (flowlet hit): " << m_leafId << " - LbTag: " << selectedPort << ", CE: " << 0 << ", FbLbTag: " << fbLbTag << ", FbMetric: " << fbMetric);

          return true;
        }
      }

      NS_LOG_LOGIC (this << " Flowlet expires, calculate the new port");
      // Not hit. Determine the port

      // 1. Select port congestion information based on dest leaf switch id
      std::map<uint32_t, std::map<uint32_t, std::pair<Time, uint32_t> > >::iterator
          congaToLeafItr = m_congaToLeafTable.find (destLeafId);

      // 2. Prepare the candidate port
      // For a new flowlet, we pick the uplink port that minimizes the maximum of the local metric (from the local DREs)
      // and the remote metric (from the Congestion-To-Leaf Table).
      uint32_t minPortCongestion = (std::numeric_limits<uint32_t>::max)();

      std::vector<uint32_t> portCandidates;
      std::vector<CongaRouteEntry>::iterator routeEntryItr = routeEntries.begin ();

      for ( ; routeEntryItr != routeEntries.end (); ++routeEntryItr)
      {
        uint32_t port = (*routeEntryItr).port;
        uint32_t localCongestion = 0;
        uint32_t remoteCongestion = 0;

        std::map<uint32_t, uint32_t>::iterator localCongestionItr = m_XMap.find (port);
        if (localCongestionItr != m_XMap.end ())
        {
          localCongestion = Ipv4CongaRouting::QuantizingX (port, localCongestionItr->second);
        }

        std::map<uint32_t, std::pair<Time, uint32_t> >::iterator remoteCongestionItr =
            (congaToLeafItr->second).find (port);
        if (remoteCongestionItr != (congaToLeafItr->second).end ())
        {
          remoteCongestion = (remoteCongestionItr->second).second;
        }

        uint32_t congestionDegree = std::max (localCongestion, remoteCongestion);

        if (congestionDegree < minPortCongestion)
        {
          // Strictly better port
          minPortCongestion = congestionDegree;
          portCandidates.clear();
          portCandidates.push_back(port);
        }
        if (congestionDegree == minPortCongestion)
        {
          // Equally good port
          portCandidates.push_back(port);
        }
      }

      // 3. Select one port from all those candidate ports
      if (flowlet != NULL &&
            std::find(portCandidates.begin (), portCandidates.end (), flowlet->port) != portCandidates.end ())
      {
        // Prefer the port cached in flowlet table
        selectedPort = flowlet->port;
        // Activate the flowlet entry again
        flowlet->activeTime = now;
      }
      else
      {
        // If there are no cached ports, we randomly choose a good port
        selectedPort = portCandidates[rand() % portCandidates.size ()];
        if (flowlet == NULL)
        {
          struct Flowlet *newFlowlet = new Flowlet;
          newFlowlet->port = selectedPort;
          newFlowlet->activeTime = now;
          m_flowletTable[flowId] = newFlowlet;
        }
        else
        {
          flowlet->port = selectedPort;
          flowlet->activeTime = now;
        }
      }

      // 4. Construct Conga Header for the packet
      ipv4CongaTag.SetLbTag (selectedPort);
      ipv4CongaTag.SetCe (0);

      // Piggyback the feedback information
      ipv4CongaTag.SetFbLbTag (fbLbTag);
      ipv4CongaTag.SetFbMetric (fbMetric);
      packet->AddPacketTag(ipv4CongaTag);

      // Update local dre
      Ipv4CongaRouting::UpdateLocalDre (header, packet, selectedPort);

      Ptr<Ipv4Route> route = Ipv4CongaRouting::ConstructIpv4Route (selectedPort, destAddress);
      ucb (route, packet, header);

      NS_LOG_LOGIC (this << " Sending Conga on leaf switch: " << m_leafId << " - LbTag: " << selectedPort << ", CE: " << 0 << ", FbLbTag: " << fbLbTag << ", FbMetric: " << fbMetric);

      return true;
    }
    else
    {
      NS_LOG_LOGIC (this << " Receiving Conga - LbTag: " << ipv4CongaTag.GetLbTag ()
              << ", CE: " << ipv4CongaTag.GetCe ()
              << ", FbLbTag: " << ipv4CongaTag.GetFbLbTag ()
              << ", FbMetric: " << ipv4CongaTag.GetFbMetric ());

      // Forwarding the packet to destination

      // Determine the source switch leaf id
      std::map<Ipv4Address, uint32_t>::iterator itr = m_ipLeafIdMap.find(header.GetSource ());
      if (itr == m_ipLeafIdMap.end ())
      {
        NS_LOG_ERROR (this << " Conga routing cannot find leaf switch id");
        ecb (packet, header, Socket::ERROR_NOROUTETOHOST);
        return false;
      }
      uint32_t sourceLeafId = itr->second;

      // 1. Update the CongaFromLeafTable
      std::map<uint32_t, std::map<uint32_t, FeedbackInfo> >::iterator fromLeafItr = m_congaFromLeafTable.find (sourceLeafId);

      if (fromLeafItr == m_congaFromLeafTable.end ())
      {
        std::map<uint32_t, FeedbackInfo> newMap;
        FeedbackInfo feedbackInfo;
        feedbackInfo.ce = ipv4CongaTag.GetCe ();
        feedbackInfo.change = true;
        feedbackInfo.updateTime = Simulator::Now ();
        newMap[ipv4CongaTag.GetLbTag ()] = feedbackInfo;
        m_congaFromLeafTable[sourceLeafId] = newMap;
      }
      else
      {
        std::map<uint32_t, FeedbackInfo>::iterator innerItr = (fromLeafItr->second).find (ipv4CongaTag.GetLbTag ());
        if (innerItr == (fromLeafItr->second).end ())
        {
          FeedbackInfo feedbackInfo;
          feedbackInfo.ce = ipv4CongaTag.GetCe ();
          feedbackInfo.change = true;
          feedbackInfo.updateTime = Simulator::Now ();
          (fromLeafItr->second)[ipv4CongaTag.GetLbTag ()] = feedbackInfo;
        }
        else
        {
          (innerItr->second).ce = ipv4CongaTag.GetCe ();
          (innerItr->second).change = true;
          (innerItr->second).updateTime = Simulator::Now ();
        }
      }

      // 2. Update the CongaToLeafTable
      if (ipv4CongaTag.GetFbLbTag () != LOOPBACK_PORT)
      {
        std::map<uint32_t, std::map<uint32_t, std::pair<Time, uint32_t> > >::iterator toLeafItr =
            m_congaToLeafTable.find(sourceLeafId);
        if (toLeafItr != m_congaToLeafTable.end ())
        {
          (toLeafItr->second)[ipv4CongaTag.GetFbLbTag ()] =
              std::make_pair(Simulator::Now (), ipv4CongaTag.GetFbMetric ());
        }
        else
        {
          std::map<uint32_t, std::pair<Time, uint32_t> > newMap;
          newMap[ipv4CongaTag.GetFbLbTag ()] =
              std::make_pair(Simulator::Now (), ipv4CongaTag.GetFbMetric ());
          m_congaToLeafTable[sourceLeafId] = newMap;
        }
      }

      // Not necessary
      // Remove the Conga Header
      packet->RemovePacketTag (ipv4CongaTag);

      // Pick port using standard ECMP
      uint32_t selectedPort = routeEntries[flowId % routeEntries.size ()].port;

      Ipv4CongaRouting::UpdateLocalDre (header, packet, selectedPort);

      Ptr<Ipv4Route> route = Ipv4CongaRouting::ConstructIpv4Route (selectedPort, destAddress);
      ucb (route, packet, header);

      Ipv4CongaRouting::PrintDreTable ();
      Ipv4CongaRouting::PrintCongaToLeafTable ();
      Ipv4CongaRouting::PrintCongaFromLeafTable ();

      return true;
    }
  }
  else
  {
    // If the switch is spine switch
    // Extract Conga Header
    Ipv4CongaTag ipv4CongaTag;
    bool found = packet->PeekPacketTag(ipv4CongaTag);
    if (!found)
    {
      NS_LOG_ERROR (this<< "Conga routing cannot extract Conga Header in spine switch");
      ecb (p, header, Socket::ERROR_NOROUTETOHOST);
      return false;
    }

    // Determine the port using standard ECMP
    uint32_t selectedPort = routeEntries[flowId % routeEntries.size ()].port;

    // Update local dre
    uint32_t X = Ipv4CongaRouting::UpdateLocalDre (header, packet, selectedPort);

    NS_LOG_LOGIC (this << " Forwarding Conga packet, Quantized X on port: " << selectedPort
            << " is: " << Ipv4CongaRouting::QuantizingX (selectedPort, X)
            << ", LbTag in Conga header is: " << ipv4CongaTag.GetLbTag ()
            << ", CE in Conga header is: " << ipv4CongaTag.GetCe ()
            << ", packet size is: " << packet->GetSize ());

    uint32_t quantizingX = Ipv4CongaRouting::QuantizingX (selectedPort, X);

    // Compare the X with that in the Conga Header
    if (quantizingX > ipv4CongaTag.GetCe()) {
      ipv4CongaTag.SetCe(quantizingX);
      packet->ReplacePacketTag(ipv4CongaTag);
    }

    Ptr<Ipv4Route> route = Ipv4CongaRouting::ConstructIpv4Route (selectedPort, destAddress);
    ucb (route, packet, header);

    return true;
  }
}

void
Ipv4CongaRouting::NotifyInterfaceUp (uint32_t interface)
{
}

void
Ipv4CongaRouting::NotifyInterfaceDown (uint32_t interface)
{
}

void
Ipv4CongaRouting::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address)
{
}

void
Ipv4CongaRouting::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address)
{
}

void
Ipv4CongaRouting::SetIpv4 (Ptr<Ipv4> ipv4)
{
  NS_LOG_LOGIC (this << "Setting up Ipv4: " << ipv4);
  NS_ASSERT (m_ipv4 == 0 && ipv4 != 0);
  m_ipv4 = ipv4;
}

void
Ipv4CongaRouting::PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const
{
}


void
Ipv4CongaRouting::DoDispose (void)
{
  std::map<uint32_t, Flowlet *>::iterator itr = m_flowletTable.begin ();
  for ( ; itr != m_flowletTable.end (); ++itr )
  {
    delete (itr->second);
  }
  m_dreEvent.Cancel ();
  m_agingEvent.Cancel ();
  m_ipv4=0;
  Ipv4RoutingProtocol::DoDispose ();
}

uint32_t
Ipv4CongaRouting::UpdateLocalDre (const Ipv4Header &header, Ptr<Packet> packet, uint32_t port)
{
  uint32_t X = 0;
  std::map<uint32_t, uint32_t>::iterator XItr = m_XMap.find(port);
  if (XItr != m_XMap.end ())
  {
    X = XItr->second;
  }
  uint32_t newX = X + packet->GetSize () + header.GetSerializedSize ();
  NS_LOG_LOGIC (this << " Update local dre, new X: " << newX);
  m_XMap[port] = newX;
  return newX;
}

void
Ipv4CongaRouting::DreEvent ()
{
  bool moveToIdleStatus = true;

  std::map<uint32_t, uint32_t>::iterator itr = m_XMap.begin ();
  for ( ; itr != m_XMap.end (); ++itr )
  {
    uint32_t newX = itr->second * (1 - m_alpha);
    itr->second = newX;
    if (newX != 0)
    {
      moveToIdleStatus = false;
    }
  }

  NS_LOG_LOGIC (this << " Dre event finished, the dre table is now: ");
  Ipv4CongaRouting::PrintDreTable ();

  if (!moveToIdleStatus)
  {
    m_dreEvent = Simulator::Schedule(m_tdre, &Ipv4CongaRouting::DreEvent, this);
  }
  else
  {
    NS_LOG_LOGIC (this << " Dre event goes into idle status");
  }
}

void
Ipv4CongaRouting::AgingEvent ()
{
    bool moveToIdleStatus = true;
    std::map<uint32_t, std::map<uint32_t, std::pair<Time, uint32_t> > >::iterator itr =
        m_congaToLeafTable.begin ();
    for ( ; itr != m_congaToLeafTable.end (); ++itr)
    {
      std::map<uint32_t, std::pair<Time, uint32_t> >::iterator innerItr =
        (itr->second).begin ();
      for (; innerItr != (itr->second).end (); ++innerItr)
      {
        if (Simulator::Now () - (innerItr->second).first > m_agingTime)
        {
          (innerItr->second).second = 0;
        }
        else
        {
          moveToIdleStatus = false;
        }
      }
    }
    std::map<uint32_t, std::map<uint32_t, FeedbackInfo> >::iterator itr2 =
        m_congaFromLeafTable.begin ();
    for ( ; itr2 != m_congaFromLeafTable.end (); ++itr2 )
    {
        std::map<uint32_t, FeedbackInfo>::iterator innerItr2 =
          (itr2->second).begin ();
        for ( ; innerItr2 != (itr2->second).end (); ++innerItr2)
        {
          if (Simulator::Now () - (innerItr2->second).updateTime > m_agingTime)
          {
            (itr2->second).erase (innerItr2);
            if ((itr2->second).empty ())
            {
                m_congaFromLeafTable.erase (itr2);
            }
          }
          else
          {
            moveToIdleStatus = false;
          }
        }
    }


    if (!moveToIdleStatus)
    {
      m_agingEvent = Simulator::Schedule(m_agingTime / 4, &Ipv4CongaRouting::AgingEvent, this);
    }
    else
    {
      NS_LOG_LOGIC (this << " Aging event goes into idle status");
    }
}

uint32_t
Ipv4CongaRouting::QuantizingX (uint32_t interface, uint32_t X)
{
  DataRate c = m_C;
  std::map<uint32_t, DataRate>::iterator itr = m_Cs.find (interface);
  if (itr != m_Cs.end ())
  {
    c = itr->second;
  }
  double ratio = static_cast<double> (X * 8) / (c.GetBitRate () * m_tdre.GetSeconds () / m_alpha);
  NS_LOG_LOGIC ("ratio: " << ratio);
  return static_cast<uint32_t>(ratio * std::pow(2, m_Q));
}

void
Ipv4CongaRouting::PrintCongaToLeafTable ()
{
/*
  std::ostringstream oss;
  oss << "===== CongaToLeafTable For Leaf: " << m_leafId <<"=====" << std::endl;
  std::map<uint32_t, std::map<uint32_t, uint32_t> >::iterator itr = m_congaToLeafTable.begin ();
  for ( ; itr != m_congaToLeafTable.end (); ++itr )
  {
    oss << "Leaf ID: " << itr->first << std::endl<<"\t";
    std::map<uint32_t, uint32_t>::iterator innerItr = (itr->second).begin ();
    for ( ; innerItr != (itr->second).end (); ++innerItr)
    {
      oss << "{ port: "
          << innerItr->first << ", ce: "  << (innerItr->second)
          << " } ";
    }
    oss << std::endl;
  }
  oss << "============================";
  NS_LOG_LOGIC (oss.str ());
*/
}

void
Ipv4CongaRouting::PrintCongaFromLeafTable ()
{
/*
  std::ostringstream oss;
  oss << "===== CongaFromLeafTable For Leaf: " << m_leafId << "=====" <<std::endl;
  std::map<uint32_t, std::map<uint32_t, FeedbackInfo> >::iterator itr = m_congaFromLeafTable.begin ();
  for ( ; itr != m_congaFromLeafTable.end (); ++itr )
  {
    oss << "Leaf ID: " << itr->first << std::endl << "\t";
    std::map<uint32_t, FeedbackInfo>::iterator innerItr = (itr->second).begin ();
    for ( ; innerItr != (itr->second).end (); ++innerItr)
    {
      oss << "{ port: "
          << innerItr->first << ", ce: "  << (innerItr->second).ce
          << ", change: " << (innerItr->second).change
          << " } ";
    }
    oss << std::endl;
  }
  oss << "==============================";
  NS_LOG_LOGIC (oss.str ());
*/
}

void
Ipv4CongaRouting::PrintFlowletTable ()
{
/*
  std::ostringstream oss;
  oss << "===== Flowlet For Leaf: " << m_leafId << "=====" << std::endl;
  std::map<uint32_t, Flowlet*>::iterator itr = m_flowletTable.begin ();
  for ( ; itr != m_flowletTable.end(); ++itr )
  {
    oss << "flowId: " << itr->first << std::endl << "\t"
        << "port: " << (itr->second)->port << "\t"
        << "activeTime" << (itr->second)->activeTime << std::endl;
  }
  oss << "===================";
  NS_LOG_LOGIC (oss.str ());
*/
}

void
Ipv4CongaRouting::PrintDreTable ()
{
/*
  std::ostringstream oss;
  std::string switchType = m_isLeaf == true ? "leaf switch" : "spine switch";
  oss << "==== Local Dre for " << switchType << " ====" <<std::endl;
  std::map<uint32_t, uint32_t>::iterator itr = m_XMap.begin ();
  for ( ; itr != m_XMap.end (); ++itr)
  {
    oss << "port: " << itr->first <<
      ", X: " << itr->second <<
      ", Quantized X: " << Ipv4CongaRouting::QuantizingX (itr->second) <<std::endl;
  }
  oss << "=================================";
  NS_LOG_LOGIC (oss.str ());
*/
}


}

