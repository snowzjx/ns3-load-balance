#ifndef IPV4_DRB_HELPER
#define IPV4_DRB_HELPER

#include "ns3/node.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-drb.h"

namespace ns3 {

class Ipv4DrbHelper
{
public:
  Ipv4DrbHelper ();
  virtual ~Ipv4DrbHelper ();

  Ipv4DrbHelper *Copy (void) const;
  virtual Ptr<Ipv4Drb> Create (Ptr<Node> node) const;
  Ptr<Ipv4Drb> GetIpv4Drb(Ptr<Ipv4> ipv4) const;
};

}

#endif
