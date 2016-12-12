/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef IPV4_DRILL_ROUTING_H
#define IPV4_DRILL_ROUTING_H

#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-route.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-address.h"

#include <vector>
#include <map>

namespace ns3 {

struct DrillRouteEntry {
  Ipv4Address network;
  Ipv4Mask networkMask;
  uint32_t port;
};


class Ipv4DrillRouting : public Ipv4RoutingProtocol {

public:
  Ipv4DrillRouting ();
  ~Ipv4DrillRouting ();

  static TypeId GetTypeId (void);

  void AddRoute (Ipv4Address network, Ipv4Mask networkMask, uint32_t port);
  std::vector<DrillRouteEntry> LookupDrillRouteEntries (Ipv4Address dest);

  uint32_t CalculateQueueLength (uint32_t interface);
  Ptr<Ipv4Route> ConstructIpv4Route (uint32_t port, Ipv4Address destAddress);


  /* Inherit From Ipv4RoutingProtocol */
  virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
  virtual bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                           UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                           LocalDeliverCallback lcb, ErrorCallback ecb);
  virtual void NotifyInterfaceUp (uint32_t interface);
  virtual void NotifyInterfaceDown (uint32_t interface);
  virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void SetIpv4 (Ptr<Ipv4> ipv4);
  virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const;

  virtual void DoDispose (void);

private:
  uint32_t m_d;
  std::map<Ipv4Address, uint32_t> m_previousBestQueueMap;

  Ptr<Ipv4> m_ipv4;
  std::vector<DrillRouteEntry> m_routeEntryList;
};

}

#endif /* IPV4_DRILL_ROUTING_H */

