/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ipv4-xpath-routing-helper.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv4XPathRoutingHelper");

Ipv4XPathRoutingHelper::Ipv4XPathRoutingHelper ()
{

}

Ipv4XPathRoutingHelper::Ipv4XPathRoutingHelper (const Ipv4XPathRoutingHelper&)
{

}

Ipv4XPathRoutingHelper*
Ipv4XPathRoutingHelper::Copy (void) const
{
  return new Ipv4XPathRoutingHelper (*this);
}

Ptr<Ipv4RoutingProtocol>
Ipv4XPathRoutingHelper::Create (Ptr<Node> node) const
{
  Ptr<Ipv4XPathRouting> xPathRouting = CreateObject<Ipv4XPathRouting> ();
  return xPathRouting;
}

Ptr<Ipv4XPathRouting>
Ipv4XPathRoutingHelper::GetXPathRouting (Ptr<Ipv4> ipv4) const
{
  Ptr<Ipv4RoutingProtocol> ipv4rp = ipv4->GetRoutingProtocol ();
  if (DynamicCast<Ipv4XPathRouting> (ipv4rp))
  {
    return DynamicCast<Ipv4XPathRouting> (ipv4rp);
  }
  return 0;
}

}

