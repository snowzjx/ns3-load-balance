/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef XPATH_ROUTING_HELPER_H
#define XPATH_ROUTING_HELPER_H

#include "ns3/ipv4-xpath-routing.h"
#include "ns3/ipv4-routing-helper.h"

namespace ns3 {

class Ipv4XPathRoutingHelper : public Ipv4RoutingHelper
{
public:
    Ipv4XPathRoutingHelper ();
    Ipv4XPathRoutingHelper (const Ipv4XPathRoutingHelper &);

    Ipv4XPathRoutingHelper* Copy (void) const;

    virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;

    Ptr<Ipv4XPathRouting> GetXPathRouting (Ptr<Ipv4> ipv4) const;
};

}

#endif /* XPATH_ROUTING_HELPER_H */

