#ifndef TCP_RESEQUENCE_BUFFER_H
#define TCP_RESEQUENCE_BUFFER_H

#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/sequence-number.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"

#include <vector>
#include <queue>

namespace ns3
{

class TcpSocketBase;

class TcpResequenceBufferElement
{

public:
  SequenceNumber32 m_seq;

  bool m_isSyn;

  bool m_isFin;

  uint32_t m_dataSize; // In bytes

  Ptr<Packet> m_packet;

  Address m_fromAddress;

  Address m_toAddress;

  friend inline bool operator > (const TcpResequenceBufferElement &l, const TcpResequenceBufferElement &r)
  {
    return l.m_seq > r.m_seq;
  }
};

class TcpResequenceBuffer : public Object
{

public:

  static TypeId GetTypeId (void);

  TcpResequenceBuffer ();
  ~TcpResequenceBuffer ();

  virtual void DoDispose (void);

  void BufferPacket (Ptr<Packet> packet, const Address& fromAddress, const Address& toAddress);

  void SetTcp (TcpSocketBase *tcp);

  void Stop (void);

private:

  bool PutInTheInOrderQueue (const TcpResequenceBufferElement &element);
  SequenceNumber32 CalculateNextSeq (const TcpResequenceBufferElement &element);

  void PeriodicalCheck ();

  void FlushOneElement (const TcpResequenceBufferElement &element);
  void FlushInOrderQueue ();
  void FlushOutOrderQueue ();

  // Parameters
  uint32_t m_sizeLimit;

  Time m_inOrderQueueTimerLimit;
  Time m_outOrderQueueTimerLimit;

  Time m_periodicalCheckTime;

  // Variables
  uint32_t m_size;
  Time m_inOrderQueueTimer;
  Time m_outOrderQueueTimer;

  EventId m_checkEvent;

  SequenceNumber32 m_firstSeq;
  SequenceNumber32 m_nextSeq;

  std::vector<TcpResequenceBufferElement> m_inOrderQueue;

  std::priority_queue<TcpResequenceBufferElement,
      std::vector<TcpResequenceBufferElement>,
      std::greater<TcpResequenceBufferElement> > m_outOrderQueue;

  TcpSocketBase *m_tcp;
};

}

#endif
