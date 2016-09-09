#include "ipv4-xpath-tag.h"

namespace ns3 {

Ipv4XPathTag::Ipv4XPathTag () {}

TypeId
Ipv4XPathTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Ipv4XPathTag")
    .SetParent<Tag> ()
    .SetGroupName ("Internet")
    .AddConstructor<Ipv4XPathTag> ();

  return tid;
}

uint32_t
Ipv4XPathTag::GetPathId (void)
{
  return m_pathId;
}

void
Ipv4XPathTag::SetPathId (uint32_t pathId)
{
  m_pathId = pathId;
}

TypeId
Ipv4XPathTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
Ipv4XPathTag::GetSerializedSize (void) const
{
  return sizeof (uint32_t);
}

void
Ipv4XPathTag::Serialize (TagBuffer i) const
{
  i.WriteU32 (m_pathId);
}

void
Ipv4XPathTag::Deserialize (TagBuffer i)
{
  m_pathId = i.ReadU32 ();
}

void
Ipv4XPathTag::Print (std::ostream &os) const
{
  os << "Path Id = " << m_pathId;
}

}
