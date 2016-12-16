/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef IPV4_LETFLOW_ROUTING_HELPER_H
#define IPV4_LETFLOW_ROUTING_HELPER_H

#include "ns3/ipv4-letflow-routing.h"
#include "ns3/ipv4-routing-helper.h"

namespace ns3 {

class Ipv4LetFlowRoutingHelper : public Ipv4RoutingHelper
{
public:
    Ipv4LetFlowRoutingHelper ();
    Ipv4LetFlowRoutingHelper (const Ipv4LetFlowRoutingHelper&);

    Ipv4LetFlowRoutingHelper *Copy (void) const;

    virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;

    Ptr<Ipv4LetFlowRouting> GetLetFlowRouting (Ptr<Ipv4> ipv4) const;
};

}

#endif /* LETFLOW_ROUTING_HELPER_H */

