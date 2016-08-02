#include "ipv4-conga-tag.h"

namespace ns3
{

Ipv4CongaTag::Ipv4CongaTag () {}

TypeId
Ipv4CongaTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Ipv4CongaTag")
    .SetParent<Tag> ()
    .SetGroupName ("Internet")
    .AddConstructor<Ipv4CongaTag> ();
  return tid;
}

void
Ipv4CongaTag::SetLbTag (uint32_t lbTag)
{
  m_lbTag = lbTag;
}

uint32_t
Ipv4CongaTag::GetLbTag (void) const
{
  return m_lbTag;
}

void
Ipv4CongaTag::SetCe (uint32_t ce)
{
  m_ce = ce;
}

uint32_t
Ipv4CongaTag::GetCe (void) const
{
  return m_ce;
}

void
Ipv4CongaTag::SetFbLbTag (uint32_t fbLbTag)
{
  m_fbLbTag = fbLbTag;
}

uint32_t
Ipv4CongaTag::GetFbLbTag (void) const
{
  return m_fbLbTag;
}

void
Ipv4CongaTag::SetFbMetric (uint32_t fbMetric)
{
  m_fbMetric = fbMetric;
}

uint32_t
Ipv4CongaTag::GetFbMetric (void) const
{
  return m_fbMetric;
}

TypeId
Ipv4CongaTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
Ipv4CongaTag::GetSerializedSize (void) const
{
  return sizeof (uint32_t) +
         sizeof (uint32_t) +
         sizeof (uint32_t) +
         sizeof (uint32_t);
}

void
Ipv4CongaTag::Serialize (TagBuffer i) const
{
  i.WriteU32(m_lbTag);
  i.WriteU32(m_ce);
  i.WriteU32(m_fbLbTag);
  i.WriteU32(m_fbMetric);
}

void
Ipv4CongaTag::Deserialize (TagBuffer i)
{
  m_lbTag = i.ReadU32 ();
  m_ce = i.ReadU32 ();
  m_fbLbTag = i.ReadU32 ();
  m_fbMetric = i.ReadU32 ();

}

void
Ipv4CongaTag::Print (std::ostream &os) const
{
  os << "Lb Tag = " << m_lbTag;
  os << "CE  = " << m_ce;
  os << "Feedback Lb Tag = " << m_fbLbTag;
  os << "Feedback Metric = " << m_fbMetric;
}

}
