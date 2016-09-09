/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef IPV4_XPATH_ROUTING_H
#define IPV4_XPATH_ROUTING_H

#include "ns3/ipv4-routing-protocol.h"

#include <map>

namespace ns3 {

class Ipv4XPathRouting : public Ipv4RoutingProtocol
{

public:
  Ipv4XPathRouting ();
  ~Ipv4XPathRouting ();

  static TypeId GetTypeId (void);

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

  Ptr<Ipv4> m_ipv4;
};

}

#endif /* XPATH_ROUTING_H */

