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
CongestionProbingTag::GetV (void) const
{
    return m_V;
}

void
CongestionProbingTag::SetV (uint32_t v)
{
    m_V = v;
}

uint32_t
CongestionProbingTag::GetIsReply (void) const
{
    return m_isReply;
}

void
CongestionProbingTag::SetIsReply (uint32_t isReply)
{
    m_isReply = isReply;
}

Time
CongestionProbingTag::GetSendTime (void) const
{
    return m_sendTime;
}

void
CongestionProbingTag::SetSendTime (Time sendTime)
{
    m_sendTime = sendTime;
}

uint32_t
CongestionProbingTag::GetIsCE (void) const
{
    return m_isCE;
}

void
CongestionProbingTag::SetIsCE (uint32_t ce)
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
         + sizeof (uint32_t)
         + sizeof (double)
         + sizeof (uint32_t);
}

void
CongestionProbingTag::Serialize (TagBuffer i) const
{
    i.WriteU32 (m_V);
    i.WriteU32 (m_isReply);
    i.WriteDouble (m_sendTime.GetSeconds ());
    i.WriteU32 (m_isCE);
}

void
CongestionProbingTag::Deserialize (TagBuffer i)
{
    m_V = i.ReadU32 ();
    m_isReply = i.ReadU32 ();
    m_sendTime = Time::FromDouble (i.ReadDouble (), Time::S);
    m_isCE = i.ReadU32 ();
}

void
CongestionProbingTag::Print (std::ostream &os) const
{
    os << "V : " << m_V;
    os << "Is Reply: " << m_isReply;
    os << "Send Time: " << m_sendTime;
    os << "Is CE: " << m_isCE;
}

}


