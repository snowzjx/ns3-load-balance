#include "ipv4-drb-helper.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-list-routing.h"

namespace ns3 {

Ipv4DrbHelper::Ipv4DrbHelper ()
{

}

Ipv4DrbHelper::~Ipv4DrbHelper ()
{

}

Ipv4DrbHelper*
Ipv4DrbHelper::Copy (void) const
{
  return new Ipv4DrbHelper (*this);
}

Ptr<Ipv4Drb>
Ipv4DrbHelper::Create (Ptr<Node> node) const
{
  return CreateObject<Ipv4Drb> ();
}

Ptr<Ipv4Drb>
Ipv4DrbHelper::GetIpv4Drb (Ptr<Ipv4> ipv4) const
{
  Ptr<Ipv4RoutingProtocol> ipv4rp = ipv4->GetRoutingProtocol ();
  if (DynamicCast<Ipv4ListRouting>(ipv4rp))
  {
    return (DynamicCast<Ipv4ListRouting>(ipv4rp))->GetDrb();
  }
  return 0;
}

}
