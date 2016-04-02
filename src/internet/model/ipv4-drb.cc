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

Ipv4Drb::Ipv4Drb ():m_index(0)
{
  NS_LOG_FUNCTION (this);
}

Ipv4Drb::~Ipv4Drb ()
{
  NS_LOG_FUNCTION (this);
}

Ipv4Address
Ipv4Drb::GetCoreSwitchAddress ()
{
  uint32_t listSize = m_coreSwitchAddressList.size();
  if (listSize == 0)
  {
    return Ipv4Address ();
  }
  Ipv4Address addr = m_coreSwitchAddressList[m_index % listSize];
  m_index ++;
  NS_LOG_DEBUG (this << " The index: " << m_index);
  return addr;
}

void
Ipv4Drb::AddCoreSwitchAddress (Ipv4Address addr)
{
  m_coreSwitchAddressList.push_back (addr);
}

}
