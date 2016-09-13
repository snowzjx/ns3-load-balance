#include "ipv4-tlb-probing-tag.h"

namespace ns3 {

TypeId
Ipv4TLBProbingTag::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::Ipv4TLBProbingTag")
        .SetParent<Tag> ()
        .SetGroupName ("TLB")
        .AddConstructor<Ipv4TLBProbingTag> ();

    return tid;
}

Ipv4TLBProbingTag::Ipv4TLBProbingTag ()
{

}

uint32_t
Ipv4TLBProbingTag::GetId (void) const
{
    return m_id;
}

void
Ipv4TLBProbingTag::SetId (uint32_t id)
{
    m_id = id;
}

uint32_t
Ipv4TLBProbingTag::GetPath (void) const
{
    return m_path;
}

void
Ipv4TLBProbingTag::SetPath (uint32_t path)
{
    m_path = path;
}

uint8_t
Ipv4TLBProbingTag::GetIsReply (void) const
{
    return m_isReply;
}

void
Ipv4TLBProbingTag::SetIsReply (uint8_t isReply)
{
    m_isReply = isReply;
}

Time
Ipv4TLBProbingTag::GetTime (void) const
{
    return m_time;
}

void
Ipv4TLBProbingTag::SetTime (Time time)
{
    m_time = time;
}

uint8_t
Ipv4TLBProbingTag::GetIsCE (void) const
{
    return m_isCE;
}

void
Ipv4TLBProbingTag::SetIsCE (uint8_t ce)
{
    m_isCE = ce;
}

TypeId
Ipv4TLBProbingTag::GetInstanceTypeId (void) const
{
    return GetTypeId ();
}

uint32_t
Ipv4TLBProbingTag::GetSerializedSize (void) const
{
    return sizeof (uint32_t)
         + sizeof (uint32_t)
         + sizeof (uint8_t)
         + sizeof (double)
         + sizeof (uint8_t);
}

void
Ipv4TLBProbingTag::Serialize (TagBuffer i) const
{
    i.WriteU32 (m_id);
    i.WriteU32 (m_path);
    i.WriteU8 (m_isReply);
    i.WriteDouble (m_time.GetSeconds ());
    i.WriteU8 (m_isCE);
}

void
Ipv4TLBProbingTag::Deserialize (TagBuffer i)
{
    m_id = i.ReadU32 ();
    m_path = i.ReadU32 ();
    m_isReply = i.ReadU8 ();
    m_time = Time::FromDouble (i.ReadDouble (), Time::S);
    m_isCE = i.ReadU8 ();
}

void
Ipv4TLBProbingTag::Print (std::ostream &os) const
{
    os << "id: " << m_id;
    os << "path: " << m_path;
    os << "Is Reply: " << m_isReply;
    os << "Time: " << m_time;
    os << "Is CE: " << m_isCE;
}

}


