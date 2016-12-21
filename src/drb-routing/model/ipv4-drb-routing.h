/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef IPV4_DRB_ROUTING_H
#define IPV4_DRB_ROUTING_H

#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-route.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-address.h"

#include <set>

namespace ns3 {

enum DrbRoutingMode
{
    PER_DEST = 0,
    PER_FLOW
};

class Ipv4DrbRouting : public Ipv4RoutingProtocol
{

public:
  Ipv4DrbRouting ();
  ~Ipv4DrbRouting ();

  static TypeId GetTypeId (void);
  bool AddPath (uint32_t path);
  bool AddPath (uint32_t weight, uint32_t path);

  // TODO ugly code
  // Patch for Weighted Presto
  bool AddWeightedPath (uint32_t weight, uint32_t path,
          const std::set<Ipv4Address>& exclusiveIPs = std::set<Ipv4Address> ());
  bool AddWeightedPath (Ipv4Address destAddr, uint32_t weight, uint32_t path);

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
  std::vector<uint32_t> m_paths;
  std::map<Ipv4Address, std::vector<uint32_t> > m_extraPaths;
  std::map<uint32_t, uint32_t> m_indexMap;
  enum DrbRoutingMode m_mode;

  Ptr<Ipv4> m_ipv4;
};

}

#endif /* IPV4_DRB_ROUTING_H */

