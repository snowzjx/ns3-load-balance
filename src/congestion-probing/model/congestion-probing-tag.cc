#include "congestion-probing-tag.h"

namespace ns3 {

TypeId
CongestionProbingTag::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::CongestionProbingTag")
        .SetParent<Tag> ()
        .SetGroupName ("Internet")
        .AddConstructor<CongestionProbingTag> ();

    return tid;
}

CongestionProbingTag::CongestionProbingTag ()
{

}

uint32_t
CongestionProbingTag::GetId (void) const
{
    return m_id;
}

void
CongestionProbingTag::SetId (uint32_t id)
{
    m_id = id;
}

uint8_t
CongestionProbingTag::GetIsReply (void) const
{
    return m_isReply;
}

void
CongestionProbingTag::SetIsReply (uint8_t isReply)
{
    m_isReply = isReply;
}

Time
CongestionProbingTag::GetTime (void) const
{
    return m_time;
}

void
CongestionProbingTag::SetTime (Time time)
{
    m_time = time;
}

uint8_t
CongestionProbingTag::GetIsCE (void) const
{
    return m_isCE;
}

void
CongestionProbingTag::SetIsCE (uint8_t ce)
{
    m_isCE = ce;
}

TypeId
CongestionProbingTag::GetInstanceTypeId (void) const
{
    return GetTypeId ();
}

uint32_t
CongestionProbingTag::GetSerializedSize (void) const
{
    return sizeof (uint32_t)
         + sizeof (uint8_t)
         + sizeof (double)
         + sizeof (uint8_t);
}

void
CongestionProbingTag::Serialize (TagBuffer i) const
{
    i.WriteU32 (m_id);
    i.WriteU8 (m_isReply);
    i.WriteDouble (m_time.GetSeconds ());
    i.WriteU8 (m_isCE);
}

void
CongestionProbingTag::Deserialize (TagBuffer i)
{
    m_id = i.ReadU32 ();
    m_isReply = i.ReadU8 ();
    m_time = Time::FromDouble (i.ReadDouble (), Time::S);
    m_isCE = i.ReadU8 ();
}

void
CongestionProbingTag::Print (std::ostream &os) const
{
    os << "id: " << m_id;
    os << "Is Reply: " << m_isReply;
    os << "Time: " << m_time;
    os << "Is CE: " << m_isCE;
}

}


