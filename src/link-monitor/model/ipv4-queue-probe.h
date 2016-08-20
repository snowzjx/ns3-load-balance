/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef IPV4_QUEUE_PROBE_H
#define IPV4_QUEUE_PROBE_H

#include "ns3/object.h"
#include "ns3/packet.h"

namespace ns3 {

class Ipv4LinkProbe;

class Ipv4QueueProbe : public Object
{
public:

  static TypeId GetTypeId (void);

  Ipv4QueueProbe ();

  void SetInterfaceId (uint32_t interfaceId);

  void SetIpv4LinkProbe (Ptr<Ipv4LinkProbe> linkProbe);

  void DequeueLogger (Ptr<const Packet> packet);

  void PacketsInQueueLogger (uint32_t oldValue, uint32_t newValue);

  void BytesInQueueLogger (uint32_t oldValue, uint32_t newValue);

  void PacketsInQueueDiscLogger (uint32_t oldValue, uint32_t newValue);

  void BytesInQueueDiscLogger (uint32_t oldValue, uint32_t newValue);

private:

  uint32_t m_interfaceId;

  Ptr<Ipv4LinkProbe> m_ipv4LinkProbe;
};

}

#endif /* IPV4_QUEUE_PROBE_H */
