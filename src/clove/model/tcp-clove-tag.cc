/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ns3/tcp-clove-tag.h"

namespace ns3 {

TypeId
TcpCloveTag::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::TcpCloveTag")
        .SetParent<Tag> ()
        .SetGroupName ("Clove")
        .AddConstructor<TcpCloveTag> ()
    ;

    return tid;
}

TcpCloveTag::TcpCloveTag ()
{

}

uint32_t
TcpCloveTag::GetPath (void) const
{
    return m_path;
}

void
TcpCloveTag::SetPath (uint32_t path)
{
    m_path = path;
}

TypeId
TcpCloveTag::GetInstanceTypeId (void) const
{
    return TcpCloveTag::GetTypeId ();
}

uint32_t
TcpCloveTag::GetSerializedSize (void) const
{
    return sizeof (uint32_t);
}

void
TcpCloveTag::Serialize (TagBuffer i) const
{
    i.WriteU32 (m_path);
}

void
TcpCloveTag::Deserialize (TagBuffer i)
{
    m_path = i.ReadU32 ();
}

void
TcpCloveTag::Print (std::ostream &os) const
{
    os << " path: " << m_path;
}

}
