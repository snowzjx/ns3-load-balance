/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ipv4-queue-probe.h"

#include "ns3/log.h"

#include "ipv4-link-probe.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv4QueueProbe");

NS_OBJECT_ENSURE_REGISTERED (Ipv4QueueProbe);

TypeId
Ipv4QueueProbe::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Ipv4QueueProbe")
            .SetParent<Object> ()
            .SetGroupName ("LinkMonitor")
            .AddConstructor<Ipv4QueueProbe> ();

  return tid;
}

Ipv4QueueProbe::Ipv4QueueProbe ()
{
  NS_LOG_FUNCTION (this);
}

void
Ipv4QueueProbe::SetIpv4LinkProbe (Ptr<Ipv4LinkProbe> linkProbe)
{
    m_ipv4LinkProbe = linkProbe;
}

void
Ipv4QueueProbe::SetInterfaceId (uint32_t interfaceId)
{
  m_interfaceId = interfaceId;
}

void
Ipv4QueueProbe::DequeueLogger (Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this);
  m_ipv4LinkProbe->DequeueLogger (packet, m_interfaceId);
}

void
Ipv4QueueProbe::PacketsInQueueLogger (uint32_t oldValue, uint32_t newValue)
{
  NS_LOG_FUNCTION (this);
  m_ipv4LinkProbe->PacketsInQueueLogger (newValue, m_interfaceId);
}

void
Ipv4QueueProbe::BytesInQueueLogger (uint32_t oldValue, uint32_t newValue)
{
  NS_LOG_FUNCTION (this);
  m_ipv4LinkProbe->BytesInQueueLogger (newValue, m_interfaceId);
}

void
Ipv4QueueProbe::PacketsInQueueDiscLogger (uint32_t oldValue, uint32_t newValue)
{
  NS_LOG_FUNCTION (this);
  m_ipv4LinkProbe->PacketsInQueueDiscLogger (newValue, m_interfaceId);
}

void
Ipv4QueueProbe::BytesInQueueDiscLogger (uint32_t oldValue, uint32_t newValue)
{
  NS_LOG_FUNCTION (this);
  m_ipv4LinkProbe->BytesInQueueDiscLogger (newValue, m_interfaceId);
}

}
