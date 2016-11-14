#ifndef TCP_RESEQUENCE_BUFFER_H
#define TCP_RESEQUENCE_BUFFER_H

#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/sequence-number.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "ns3/callback.h"
#include "ns3/traced-value.h"

#include <vector>
#include <queue>
#include <set>

namespace ns3
{

enum TcpRBPopReason
{
  IN_ORDER_FULL = 0,
  IN_ORDER_TIMEOUT,
  OUT_ORDER_TIMEOUT,
  RE_TRANS
};

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
  //TcpResequenceBuffer (const TcpResequenceBuffer &);
  ~TcpResequenceBuffer ();

  virtual void DoDispose (void);

  void BufferPacket (Ptr<Packet> packet, const Address& fromAddress, const Address& toAddress);

  void SetTcp (TcpSocketBase *tcp);

  void Stop (void);

  TracedCallback <uint32_t, Time, SequenceNumber32, SequenceNumber32> m_tcpRBBuffer;
  TracedCallback <uint32_t, Time, SequenceNumber32, uint32_t, uint32_t, TcpRBPopReason> m_tcpRBFlush;

private:

  bool PutInTheInOrderQueue (const TcpResequenceBufferElement &element);
  SequenceNumber32 CalculateNextSeq (const TcpResequenceBufferElement &element);

  void PeriodicalCheck ();

  void FlushOneElement (const TcpResequenceBufferElement &element, TcpRBPopReason reason);
  void FlushInOrderQueue (TcpRBPopReason reason);
  void FlushOutOrderQueue (TcpRBPopReason reason);

  // Parameters
  uint32_t m_sizeLimit;

  Time m_inOrderQueueTimerLimit;
  Time m_outOrderQueueTimerLimit;

  Time m_periodicalCheckTime;

  uint32_t m_traceFlowId;

  // Variables
  uint32_t m_size;
  Time m_inOrderQueueTimer;
  Time m_outOrderQueueTimer;

  EventId m_checkEvent;
  bool m_hasStopped;

  SequenceNumber32 m_firstSeq;
  SequenceNumber32 m_nextSeq;

  std::vector<TcpResequenceBufferElement> m_inOrderQueue;

  std::priority_queue<TcpResequenceBufferElement,
      std::vector<TcpResequenceBufferElement>,
      std::greater<TcpResequenceBufferElement> > m_outOrderQueue;

  std::set<SequenceNumber32> m_outOrderSeqSet;

  TcpSocketBase *m_tcp;

  typedef void (* TcpRBBuffer) (uint32_t flowId, Time time, SequenceNumber32 recSeq,
          SequenceNumber32 nextSeq);
  typedef void (* TcpRBFlush) (uint32_t flowId, Time time, SequenceNumber32 popSeq,
          uint32_t inOrderLength, uint32_t outOrderLength, TcpRBPopReason reason);
};

}

#endif
