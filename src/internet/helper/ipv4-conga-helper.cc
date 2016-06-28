#include "ipv4-conga-helper.h"

#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/ipv4-global-routing.h"

namespace ns3 {

Ptr<Ipv4Conga>
Ipv4CongaHelper::GetIpv4Conga (Ptr<Ipv4> ipv4) const
{
  Ptr<Ipv4RoutingProtocol> ipv4rp = ipv4->GetRoutingProtocol ();
  if (DynamicCast<Ipv4ListRouting>(ipv4rp))
  {
    int16_t priority;
    // The index for global routing is 0 while the static routing is 1
    // TODO changed due to the current conga implementation has to work together with global routing
    Ptr<Ipv4RoutingProtocol> ipv4Grp = DynamicCast<Ipv4ListRouting>(ipv4rp)->GetRoutingProtocol (0, priority);

    if (DynamicCast<Ipv4GlobalRouting>(ipv4Grp))
    {
      return DynamicCast<Ipv4GlobalRouting>(ipv4Grp)->GetIpv4Conga ();
    }
  }

  return NULL;
}

}
