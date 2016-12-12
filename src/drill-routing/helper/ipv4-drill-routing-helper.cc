/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ipv4-drill-routing-helper.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv4DrillRoutingHelper");

Ipv4DrillRoutingHelper::Ipv4DrillRoutingHelper ()
{

}

Ipv4DrillRoutingHelper::Ipv4DrillRoutingHelper (const Ipv4DrillRoutingHelper&)
{

}

Ipv4DrillRoutingHelper*
Ipv4DrillRoutingHelper::Copy (void) const
{
  return new Ipv4DrillRoutingHelper (*this);
}

Ptr<Ipv4RoutingProtocol>
Ipv4DrillRoutingHelper::Create (Ptr<Node> node) const
{
  Ptr<Ipv4DrillRouting> drillRouting = CreateObject<Ipv4DrillRouting> ();
  return drillRouting;
}

Ptr<Ipv4DrillRouting>
Ipv4DrillRoutingHelper::GetDrillRouting (Ptr<Ipv4> ipv4) const
{
  Ptr<Ipv4RoutingProtocol> ipv4rp = ipv4->GetRoutingProtocol ();
  if (DynamicCast<Ipv4DrillRouting> (ipv4rp))
  {
    return DynamicCast<Ipv4DrillRouting> (ipv4rp);
  }
  if (DynamicCast<Ipv4ListRouting> (ipv4rp))
  {
    Ptr<Ipv4ListRouting> lrp = DynamicCast<Ipv4ListRouting> (ipv4rp);
    int16_t priority;
    for (uint32_t i = 0; i < lrp->GetNRoutingProtocols ();  i++)
    {
      Ptr<Ipv4RoutingProtocol> temp = lrp->GetRoutingProtocol (i, priority);
      if (DynamicCast<Ipv4DrillRouting> (temp))
      {
        return DynamicCast<Ipv4DrillRouting> (temp);
      }
    }
  }

  return 0;
}

}

