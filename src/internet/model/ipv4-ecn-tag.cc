#include "ipv4-ecn-tag.h"

namespace ns3
{

Ipv4EcnTag::Ipv4EcnTag () {}

void
Ipv4EcnTag::SetEcn (Ipv4Header::EcnType ecn)
{
  m_ipv4Ecn = ecn;
}

Ipv4Header::EcnType
Ipv4EcnTag::GetEcn (void) const
{
  return Ipv4Header::EcnType (m_ipv4Ecn);
}

TypeId
Ipv4EcnTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Ipv4EcnTag")
    .SetParent<Tag> ()
    .SetGroupName ("Internet")
    .AddConstructor<Ipv4EcnTag> ();
  return tid;
}

TypeId
Ipv4EcnTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
Ipv4EcnTag::GetSerializedSize (void) const
{
  return sizeof (uint8_t);
}

void
Ipv4EcnTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_ipv4Ecn);
}

void
Ipv4EcnTag::Deserialize (TagBuffer i)
{
  m_ipv4Ecn = i.ReadU8();
}
void
Ipv4EcnTag::Print (std::ostream &os) const
{
  os << "IP_ECN = " << m_ipv4Ecn;
}
}
