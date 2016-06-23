#include "ns3/log.h"
#include "ipv4-conga.h"
#include "ipv4-conga-tag.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv4Conga");

NS_OBJECT_ENSURE_REGISTERED (Ipv4Conga);

TypeId
Ipv4Conga::GetTypeId (void)
{
  static TypeId tid = TypeId("ns3::Ipv4Conga")
      .SetParent<Object>()
      .SetGroupName ("Internet")
      .AddConstructor<Ipv4Conga> ();

  return tid;
}

Ipv4Conga::Ipv4Conga ():
    m_isLeaf (false), m_leafId (0)
{
  NS_LOG_FUNCTION (this);
}

Ipv4Conga::~Ipv4Conga ()
{
  NS_LOG_FUNCTION (this);
}

void
Ipv4Conga::SetLeafId (uint32_t leafId)
{
  m_isLeaf = true;
  m_leafId = leafId;
}

void
Ipv4Conga::AddAddressToLeafIdMap (Ipv4Address addr, uint32_t leafId)
{
  m_ipLeafIdMap[addr] = leafId;
}

uint32_t
Ipv4Conga::ProcessPacket (Ptr<Packet> packet)
{
  // First, check if this switch if leaf switch
  if (m_isLeaf)
  {
    // If the switch is leaf switch, two possible situations
    // 1. The sender is connected to this leaf switch
    // 2. The receiver is connected to this leaf switch
    // We can distinguish it by checking whether the packet has CongaTag
    Ipv4CongaTag ipv4CongTag;
    bool found = packet->PeekPacketTag(ipv4CongTag);

    if (!found)
    {
      // Sending a new packet
    }
    else
    {
      // Forwarding the packet to dest
    }

  }
  else
  {
  }
  return 0;
}

}
