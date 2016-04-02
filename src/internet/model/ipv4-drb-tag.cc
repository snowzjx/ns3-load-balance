#include "ipv4-drb-tag.h"

namespace ns3
{

Ipv4DrbTag::Ipv4DrbTag () {}

void
Ipv4DrbTag::SetOriginalDestAddr (Ipv4Address addr)
{
  m_addr = addr;
}

Ipv4Address
Ipv4DrbTag::GetOriginalDestAddr (void) const
{
  return m_addr;
}

TypeId
Ipv4DrbTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Ipv4DrbTag")
    .SetParent<Tag> ()
    .SetGroupName ("Internet")
    .AddConstructor<Ipv4DrbTag> ();
  return tid;
}

TypeId
Ipv4DrbTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
Ipv4DrbTag::GetSerializedSize (void) const
{
  return sizeof (uint32_t);
}

void
Ipv4DrbTag::Serialize (TagBuffer i) const
{
  i.WriteU32(m_addr.Get ());
}

void
Ipv4DrbTag::Deserialize (TagBuffer i)
{
  m_addr.Set (i.ReadU32 ());
}

void
Ipv4DrbTag::Print (std::ostream &os) const
{
  os << "IP_Drb_original_dest_addr = " << m_addr;
}
}
