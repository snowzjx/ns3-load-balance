/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ipv4-drill-routing.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/net-device.h"
#include "ns3/channel.h"
#include "ns3/node.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/traffic-control-layer.h"
#include "ns3/point-to-point-net-device.h"

#include <algorithm>
#include <limits>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv4DrillRouting");

NS_OBJECT_ENSURE_REGISTERED (Ipv4DrillRouting);

TypeId
Ipv4DrillRouting::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Ipv4DrillRouting")
      .SetParent<Object> ()
      .SetGroupName ("DrillRouting")
      .AddConstructor<Ipv4DrillRouting> ()
      .AddAttribute ("d", "Sample d random outputs queue",
                     UintegerValue (2),
                     MakeUintegerAccessor (&Ipv4DrillRouting::m_d),
                     MakeUintegerChecker<uint32_t> ())
  ;

  return tid;
}

Ipv4DrillRouting::Ipv4DrillRouting ()
    : m_d (2)
{
  NS_LOG_FUNCTION (this);
}

Ipv4DrillRouting::~Ipv4DrillRouting ()
{
  NS_LOG_FUNCTION (this);
}

void
Ipv4DrillRouting::AddRoute (Ipv4Address network, Ipv4Mask networkMask, uint32_t port)
{
  NS_LOG_LOGIC (this << " Add Drill routing entry: " << network << "/" << networkMask << " would go through port: " << port);
  DrillRouteEntry drillRouteEntry;
  drillRouteEntry.network = network;
  drillRouteEntry.networkMask = networkMask;
  drillRouteEntry.port = port;
  m_routeEntryList.push_back (drillRouteEntry);
}

std::vector<DrillRouteEntry>
Ipv4DrillRouting::LookupDrillRouteEntries (Ipv4Address dest)
{
  std::vector<DrillRouteEntry> drillRouteEntries;
  std::vector<DrillRouteEntry>::iterator itr = m_routeEntryList.begin ();
  for ( ; itr != m_routeEntryList.end (); ++itr)
  {
    if((*itr).networkMask.IsMatch(dest, (*itr).network))
    {
      drillRouteEntries.push_back (*itr);
    }
  }
  return drillRouteEntries;
}

uint32_t
Ipv4DrillRouting::CalculateQueueLength (uint32_t interface)
{
  Ptr<Ipv4L3Protocol> ipv4L3Protocol = DynamicCast<Ipv4L3Protocol> (m_ipv4);
  if (!ipv4L3Protocol)
  {
    NS_LOG_ERROR (this << " Drill routing cannot work other than Ipv4L3Protocol");
    return 0;
  }

  uint32_t totalLength = 0;

  const Ptr<NetDevice> netDevice = this->m_ipv4->GetNetDevice (interface);

  if (netDevice->IsPointToPoint ())
  {
    Ptr<PointToPointNetDevice> p2pNetDevice = DynamicCast<PointToPointNetDevice> (netDevice);
    if (p2pNetDevice)
    {
      totalLength += p2pNetDevice->GetQueue ()->GetNBytes ();
    }
  }

  Ptr<TrafficControlLayer> tc = ipv4L3Protocol->GetNode ()->GetObject<TrafficControlLayer> ();

  if (!tc)
  {
    return totalLength;
  }

  Ptr<QueueDisc> queueDisc = tc->GetRootQueueDiscOnDevice (netDevice);
  if (queueDisc)
  {
    totalLength += queueDisc->GetNBytes ();
  }

  return totalLength;
}

Ptr<Ipv4Route>
Ipv4DrillRouting::ConstructIpv4Route (uint32_t port, Ipv4Address destAddress)
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


/* Inherit From Ipv4RoutingProtocol */
Ptr<Ipv4Route>
Ipv4DrillRouting::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{
  NS_LOG_ERROR (this << " Drill routing is not support for local routing output");
  return 0;
}

bool
Ipv4DrillRouting::RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                            UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                            LocalDeliverCallback lcb, ErrorCallback ecb)
{
  NS_LOG_LOGIC (this << " RouteInput: " << p << "Ip header: " << header);

  NS_ASSERT (m_ipv4->GetInterfaceForDevice (idev) >= 0);

  Ptr<Packet> packet = ConstCast<Packet> (p);

  Ipv4Address destAddress = header.GetDestination();

  // Conga routing only supports unicast
  if (destAddress.IsMulticast() || destAddress.IsBroadcast()) {
    NS_LOG_ERROR (this << " Drill routing only supports unicast");
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

  std::vector<DrillRouteEntry> allPorts = Ipv4DrillRouting::LookupDrillRouteEntries (destAddress);

  if (allPorts.empty ())
  {
    NS_LOG_ERROR (this << " Drill routing cannot find routing entry");
    ecb (packet, header, Socket::ERROR_NOROUTETOHOST);
    return false;
  }

  uint32_t leastLoadInterface = 0;
  uint32_t leastLoad = std::numeric_limits<uint32_t>::max ();

  std::random_shuffle (allPorts.begin (), allPorts.end ());

  std::map<Ipv4Address, uint32_t>::iterator itr = m_previousBestQueueMap.find (destAddress);

  if (itr != m_previousBestQueueMap.end ())
  {
    leastLoadInterface = itr->second;
    leastLoad = CalculateQueueLength (itr->second);
  }

  uint32_t sampleNum = m_d < allPorts.size () ? m_d : allPorts.size ();

  for (uint32_t samplePort = 0; samplePort < sampleNum; samplePort ++)
  {
    uint32_t sampleLoad = Ipv4DrillRouting::CalculateQueueLength (allPorts[samplePort].port);
    if (sampleLoad < leastLoad)
    {
      leastLoad = sampleLoad;
      leastLoadInterface = allPorts[samplePort].port;
    }
  }

  NS_LOG_INFO (this << " Drill routing chooses interface: " << leastLoadInterface << ", since its load is: " << leastLoad);

  m_previousBestQueueMap[destAddress] = leastLoadInterface;

  Ptr<Ipv4Route> route = Ipv4DrillRouting::ConstructIpv4Route (leastLoadInterface, destAddress);
  ucb (route, packet, header);

  return true;
}

void
Ipv4DrillRouting::NotifyInterfaceUp (uint32_t interface)
{
}

void
Ipv4DrillRouting::NotifyInterfaceDown (uint32_t interface)
{
}

void
Ipv4DrillRouting::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address)
{
}

void
Ipv4DrillRouting::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address)
{
}

void
Ipv4DrillRouting::SetIpv4 (Ptr<Ipv4> ipv4)
{
  NS_LOG_LOGIC (this << "Setting up Ipv4: " << ipv4);
  NS_ASSERT (m_ipv4 == 0 && ipv4 != 0);
  m_ipv4 = ipv4;
}

void
Ipv4DrillRouting::PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const
{
}

void
Ipv4DrillRouting::DoDispose (void)
{
}
}

