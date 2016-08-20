/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef IPV4_LINK_PROBE_H
#define IPV4_LINK_PROBE_H

#include "link-probe.h"

#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/event-id.h"
#include "ns3/data-rate.h"

#include "ipv4-queue-probe.h"

namespace ns3 {

class Ipv4LinkProbe : public LinkProbe
{
public:

  static TypeId GetTypeId (void);

  Ipv4LinkProbe (Ptr<Node> node, Ptr<LinkMonitor> linkMonitor);

  void SetDataRateAll (DataRate dataRate);

  void SetCheckTime (Time checkTime);

  void TxLogger (Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t interface);

  void DequeueLogger (Ptr<const Packet> packet, uint32_t interface);

  void PacketsInQueueLogger (uint32_t NPackets, uint32_t interface);

  void BytesInQueueLogger (uint32_t NBytes, uint32_t interface);

  void PacketsInQueueDiscLogger (uint32_t NPackets, uint32_t interface);

  void BytesInQueueDiscLogger (uint32_t NBytes, uint32_t interface);

  void CheckCurrentStatus ();

  void Start ();
  void Stop ();

private:

  double GetLinkUtility (uint32_t interface, uint64_t bytes, Time time);

  Time m_checkTime;

  EventId m_checkEvent;

  std::map<uint32_t, Ptr<Ipv4QueueProbe> > m_queueProbe;

  std::map<uint32_t, uint64_t> m_accumulatedTxBytes;
  std::map<uint32_t, uint64_t> m_accumulatedDequeueBytes;

  std::map<uint32_t, uint32_t> m_NPacketsInQueue;
  std::map<uint32_t, uint32_t> m_NBytesInQueue;

  std::map<uint32_t, uint32_t> m_NPacketsInQueueDisc;
  std::map<uint32_t, uint32_t> m_NBytesInQueueDisc;

  std::map<uint32_t, DataRate> m_dataRate;

  Ptr<Ipv4L3Protocol> m_ipv4;
};

}

#endif /* IPV4_LINK_PROBE_H */

