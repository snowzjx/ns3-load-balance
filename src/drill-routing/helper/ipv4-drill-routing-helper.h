/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef IPV4_DRILL_ROUTING_HELPER_H
#define IPV4_DRILL_ROUTING_HELPER_H

#include "ns3/ipv4-drill-routing.h"
#include "ns3/ipv4-routing-helper.h"

namespace ns3 {

class Ipv4DrillRoutingHelper : public Ipv4RoutingHelper
{
public:
    Ipv4DrillRoutingHelper ();
    Ipv4DrillRoutingHelper (const Ipv4DrillRoutingHelper&);

    Ipv4DrillRoutingHelper *Copy (void) const;

    virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;

    Ptr<Ipv4DrillRouting> GetDrillRouting (Ptr<Ipv4> ipv4) const;
};

}

#endif /* DRILL_ROUTING_HELPER_H */

