#ifndef IPV4_CONGA_HELPER
#define IPV4_CONGA_HELPER

#include "ns3/ipv4.h"
#include "ns3/ipv4-conga.h"

namespace ns3 {

class Ipv4CongaHelper
{
public:
  Ptr<Ipv4Conga> GetIpv4Conga(Ptr<Ipv4> ipv4) const;
};

}

#endif
