#include "ns3/log.h"
#include "ipv4-drb.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Ipv4Drb");

NS_OBJECT_ENSURE_REGISTERED (Ipv4Drb);

TypeId
Ipv4Drb::GetTypeId (void)
{
  static TypeId tid = TypeId("ns3::Ipv4Drb")
      .SetParent<Object>()
      .SetGroupName ("Internet")
      .AddConstructor<Ipv4Drb> ();

  return tid;
}

Ipv4Drb::Ipv4Drb ()
{
  NS_LOG_FUNCTION (this);
}

Ipv4Drb::~Ipv4Drb ()
{
  NS_LOG_FUNCTION (this);
}

Ipv4Address
Ipv4Drb::GetCoreSwitchAddress (uint32_t flowId)
{
  NS_LOG_FUNCTION (this);

  uint32_t listSize = m_coreSwitchAddressList.size();

  if (listSize == 0)
  {
    return Ipv4Address ();
  }

  uint32_t index = rand () % listSize;

  std::map<uint32_t, uint32_t>::iterator itr = m_indexMap.find (flowId);

  if (itr != m_indexMap.end ())
  {
    index = itr->second;
  }
  m_indexMap[flowId] = ((index + 1) % listSize);

  Ipv4Address addr = m_coreSwitchAddressList[index];

  NS_LOG_DEBUG (this << " The index for flow: " << flowId << " is : " << index);
  return addr;
}

void
Ipv4Drb::AddCoreSwitchAddress (Ipv4Address addr)
{
  NS_LOG_FUNCTION (this << addr);
  m_coreSwitchAddressList.push_back (addr);
}

void
Ipv4Drb::AddCoreSwitchAddress (uint32_t k, Ipv4Address addr)
{
  for (uint32_t i = 0; i < k; i++)
  {
    Ipv4Drb::AddCoreSwitchAddress(addr);
  }
}

}
