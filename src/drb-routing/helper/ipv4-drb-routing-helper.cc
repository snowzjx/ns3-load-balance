/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ipv4-drb-routing-helper.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv4DrbRoutingHelper");

Ipv4DrbRoutingHelper::Ipv4DrbRoutingHelper ()
{

}

Ipv4DrbRoutingHelper::Ipv4DrbRoutingHelper (const Ipv4DrbRoutingHelper&)
{

}

Ipv4DrbRoutingHelper*
Ipv4DrbRoutingHelper::Copy (void) const
{
  return new Ipv4DrbRoutingHelper (*this);
}

Ptr<Ipv4RoutingProtocol>
Ipv4DrbRoutingHelper::Create (Ptr<Node> node) const
{
  Ptr<Ipv4DrbRouting> drbRouting = CreateObject<Ipv4DrbRouting> ();
  return drbRouting;
}

Ptr<Ipv4DrbRouting>
Ipv4DrbRoutingHelper::GetDrbRouting (Ptr<Ipv4> ipv4) const
{
  Ptr<Ipv4RoutingProtocol> ipv4rp = ipv4->GetRoutingProtocol ();
  if (DynamicCast<Ipv4DrbRouting> (ipv4rp))
  {
    return DynamicCast<Ipv4DrbRouting> (ipv4rp);
  }
  if (DynamicCast<Ipv4ListRouting> (ipv4rp))
  {
    Ptr<Ipv4ListRouting> lrp = DynamicCast<Ipv4ListRouting> (ipv4rp);
    int16_t priority;
    for (uint32_t i = 0; i < lrp->GetNRoutingProtocols ();  i++)
    {
      Ptr<Ipv4RoutingProtocol> temp = lrp->GetRoutingProtocol (i, priority);
      if (DynamicCast<Ipv4DrbRouting> (temp))
      {
        return DynamicCast<Ipv4DrbRouting> (temp);
      }
    }
  }

  return 0;
}

}

