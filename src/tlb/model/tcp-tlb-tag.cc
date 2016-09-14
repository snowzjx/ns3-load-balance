#include "ns3/tcp-tlb-tag.h"

namespace ns3 {

TypeId
TcpTLBTag::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::TcpTLBTag")
        .SetParent<Tag> ()
        .SetGroupName ("TLB")
        .AddConstructor<TcpTLBTag> ();

    return tid;

}

TcpTLBTag::TcpTLBTag ()
{

}

uint32_t
TcpTLBTag::GetPath (void) const
{
    return m_path;
}

void
TcpTLBTag::SetPath (uint32_t path)
{
    m_path = path;
}

Time
TcpTLBTag::GetTime (void) const
{
    return m_time;
}

void
TcpTLBTag::SetTime (Time time)
{
    m_time = time;
}

TypeId
TcpTLBTag::GetInstanceTypeId (void) const
{
    return TcpTLBTag::GetTypeId ();
}

uint32_t
TcpTLBTag::GetSerializedSize (void) const
{
    return sizeof (uint32_t)
         + sizeof (double);
}

void
TcpTLBTag::Serialize (TagBuffer i) const
{
    i.WriteU32 (m_path);
    i.WriteDouble (m_time.GetSeconds ());
}

void
TcpTLBTag::Deserialize (TagBuffer i)
{
    m_path = i.ReadU32 ();
    m_time = Time::FromDouble (i.ReadDouble (), Time::S);
}

void
TcpTLBTag::Print (std::ostream &os) const
{
    os << " path: " << m_path
        <<" time: " << m_time;
}

}
