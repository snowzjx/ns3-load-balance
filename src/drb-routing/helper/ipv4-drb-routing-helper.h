/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef IPV4_DRB_ROUTING_HELPER_H
#define IPV4_DRB_ROUTING_HELPER_H

#include "ns3/ipv4-drb-routing.h"
#include "ns3/ipv4-routing-helper.h"

namespace ns3 {

class Ipv4DrbRoutingHelper : public Ipv4RoutingHelper
{
public:
    Ipv4DrbRoutingHelper ();
    Ipv4DrbRoutingHelper (const Ipv4DrbRoutingHelper&);

    Ipv4DrbRoutingHelper *Copy (void) const;

    virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;

    Ptr<Ipv4DrbRouting> GetDrbRouting (Ptr<Ipv4> ipv4) const;
};

}

#endif /* IPV4_DRB_ROUTING_HELPER_H */

