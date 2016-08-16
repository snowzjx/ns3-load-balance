#include "tcp-resequence-buffer.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/tcp-header.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("TcpResequenceBuffer");

NS_OBJECT_ENSURE_REGISTERED (TcpResequenceBuffer);

TypeId
TcpResequenceBuffer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpResequenceBuffer")
    .SetParent<Object> ()
    .SetGroupName ("Internet")
    .AddConstructor<TcpResequenceBuffer> ()
    .AddAttribute ("SizeLimit",
                   "In order queue max size",
                   UintegerValue (64000),
                   MakeUintegerAccessor (&TcpResequenceBuffer::m_sizeLimit),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("InOrderQueueTimerLimit",
                   "In order queue timer limit",
                   TimeValue (MicroSeconds (20)),
                   MakeTimeAccessor (&TcpResequenceBuffer::m_inOrderQueueTimerLimit),
                   MakeTimeChecker  ())
    .AddAttribute ("OutOrderQueueTimerLimit",
                   "Out order queue timer limit",
                   TimeValue (MicroSeconds (50)),
                   MakeTimeAccessor (&TcpResequenceBuffer::m_outOrderQueueTimerLimit),
                   MakeTimeChecker  ())
    .AddAttribute ("PeriodicalCheckTime",
                   "Periodical check time",
                   TimeValue (MicroSeconds (10)),
                   MakeTimeAccessor (&TcpResequenceBuffer::m_periodicalCheckTime),
                   MakeTimeChecker  ());

  return tid;
}

TcpResequenceBuffer::TcpResequenceBuffer ():
	// Parameters
    m_sizeLimit (64000),
    m_inOrderQueueTimerLimit (MicroSeconds (20)),
    m_outOrderQueueTimerLimit (MicroSeconds (50)),
    m_periodicalCheckTime (MicroSeconds (10)),
 	// Variables
    m_size (0),
    m_inOrderQueueTimer (Simulator::Now ()),
    m_outOrderQueueTimer (Simulator::Now ()),
    m_checkEvent (),
    m_firstSeq (SequenceNumber32 (0)),
    m_nextSeq (SequenceNumber32 (0))
{
  NS_LOG_FUNCTION (this);
}

TcpResequenceBuffer::~TcpResequenceBuffer ()
{
  NS_LOG_FUNCTION (this);
}

void
TcpResequenceBuffer::DoDispose (void)
{
  m_inOrderQueue.clear ();
  while (!m_outOrderQueue.empty ())
  {
    m_outOrderQueue.pop ();
  }
  if (m_checkEvent.IsRunning ())
  {
    m_checkEvent.Cancel ();
  }
}

void
TcpResequenceBuffer::BufferPacket (Ptr<Packet> packet,
        const Address& fromAddress, const Address& toAddress)
{
  NS_LOG_FUNCTION (this << "Buffering the packet: " << packet);

  // Turn on the periodical check and reset the timer
  if (!m_checkEvent.IsRunning ())
  {
    NS_LOG_LOGIC ("Turn on periodical check");
    m_checkEvent = Simulator::Schedule (m_periodicalCheckTime, &TcpResequenceBuffer::PeriodicalCheck, this);
    m_inOrderQueueTimer = Simulator::Now ();
    m_outOrderQueueTimer = Simulator::Now ();
  }

  // Extract the Tcp header
  TcpHeader tcpHeader;
  uint32_t bytesPeeked = packet->PeekHeader (tcpHeader);

  if (bytesPeeked == 0)
  {
    NS_LOG_ERROR ("Cannot peek the TCP header");
    return;
  }

  TcpResequenceBufferElement element;

  // Extract the seq number
  element.m_seq = tcpHeader.GetSequenceNumber ();
  element.m_isSyn = (tcpHeader.GetFlags () & TcpHeader::SYN) == TcpHeader::SYN ? true: false;
  element.m_isFin = (tcpHeader.GetFlags () & TcpHeader::FIN) == TcpHeader::FIN ? true: false;
  element.m_dataSize = packet->GetSize () - tcpHeader.GetLength () * 4;
  element.m_packet = packet;
  element.m_fromAddress = fromAddress;
  element.m_toAddress = toAddress;

  NS_LOG_INFO ("\tThe packet seq is: " << element.m_seq
    << " and the expected next seq is: " << TcpResequenceBuffer::CalculateNextSeq (element));

  // If the seq < first seq, retransmission may occur
  if (element.m_seq < m_firstSeq)
  {
    NS_LOG_LOGIC ("Receive retransmission packet with seq:" << element.m_seq
				<< ", while the firstSeq is: " << m_firstSeq);
    TcpResequenceBuffer::FlushInOrderQueue ();
    // Two situation
    // Case 1. The packet is exactly the previous one
    // We just continue to expect packets
    // Case 2. The packet is not exactly the previous one
    // We need to expect the next one of this packet
    if (TcpResequenceBuffer::CalculateNextSeq (element) != m_firstSeq)
    {
      m_nextSeq = TcpResequenceBuffer::CalculateNextSeq (element);
    }
    TcpResequenceBuffer::FlushOneElement (element);

    // After flush, the first and next seq would be the same
    m_firstSeq = m_nextSeq;
  }
  // If the first seq <= seq < next seq
  // TODO check, we simply ignore/drop this packet
  else if (m_firstSeq <= element.m_seq && element.m_seq < m_nextSeq)
  {
    NS_LOG_LOGIC ("Ignore packet with seq: " << element.m_seq
            << ", while the firstSeq is: " << m_firstSeq
            << " and the nextSeq is: " << m_nextSeq);
  }
  // If the seq == next seq
  else if (TcpResequenceBuffer::PutInTheInOrderQueue (element))
  {
    // Try to fill the in order queue from the out order queue
    while (!m_outOrderQueue.empty ())
    {
      if (TcpResequenceBuffer::PutInTheInOrderQueue (m_outOrderQueue.top ()))
      {
        m_outOrderQueue.pop ();
      }
      else
      {
        break;
      }
    }
    // If the size exceeds the limit
    if (m_size >= m_sizeLimit)
    {
		NS_LOG_LOGIC ("In order queue size exceeds the size limit");
        TcpResequenceBuffer::FlushInOrderQueue ();
        m_firstSeq = m_nextSeq;
    }
  }
  // If the seq > next seq
  else
  {
    m_outOrderQueue.push (element);
  }
}

void
TcpResequenceBuffer::SetTcpForwardUpCallback (Callback<void, Ptr<Packet>, const Address& , const Address&> callback)
{
  m_tcpForwardUp = callback;
}

bool
TcpResequenceBuffer::PutInTheInOrderQueue (const TcpResequenceBufferElement &element)
{
  if (m_nextSeq == SequenceNumber32 (0) // For the fist packet
        || m_nextSeq == element.m_seq)
  {
    m_inOrderQueue.push_back (element);
    m_size += element.m_dataSize;
    m_nextSeq = TcpResequenceBuffer::CalculateNextSeq (element);
    if (m_nextSeq == SequenceNumber32 (0))
    {
      m_firstSeq = element.m_seq;
    }
    return true;
  }
  else
  {
    return false;
  }
}

SequenceNumber32
TcpResequenceBuffer::CalculateNextSeq (const TcpResequenceBufferElement &element)
{
  SequenceNumber32 newSeq = element.m_seq + SequenceNumber32 (element.m_dataSize);
  if (element.m_isSyn || element.m_isFin)
  {
    newSeq = newSeq + SequenceNumber32 (1);
  }
  return newSeq;
}

void
TcpResequenceBuffer::PeriodicalCheck ()
{
  if (Simulator::Now () - m_inOrderQueueTimer > m_inOrderQueueTimerLimit)
  {
    FlushInOrderQueue ();
    m_firstSeq = m_nextSeq;
  }

  if (Simulator::Now () - m_outOrderQueueTimer > m_outOrderQueueTimerLimit)
  {
    FlushInOrderQueue ();
    FlushOutOrderQueue ();
    m_firstSeq = m_nextSeq;
  }

  if (!m_inOrderQueue.empty () || !m_outOrderQueue.empty ())
  {
    m_checkEvent = Simulator::Schedule (m_periodicalCheckTime, &TcpResequenceBuffer::PeriodicalCheck, this);
  }
  else
  {
	NS_LOG_LOGIC ("Turn the periodical check into idle status");
  }
}

void
TcpResequenceBuffer::FlushOneElement (const TcpResequenceBufferElement &element)
{
  NS_LOG_INFO ("Flush packet: " << element.m_packet);
  m_tcpForwardUp (element.m_packet, element.m_fromAddress, element.m_toAddress);
}

void
TcpResequenceBuffer::FlushInOrderQueue ()
{
  NS_LOG_FUNCTION (this);
  // Flush the data
  std::vector<TcpResequenceBufferElement>::iterator itr = m_inOrderQueue.begin ();
  for (; itr != m_inOrderQueue.end (); ++itr)
  {
    TcpResequenceBuffer::FlushOneElement (*itr);
  }
  m_inOrderQueue.clear ();

  // Reset variables
  m_size = 0;

  // Reset timer
  m_inOrderQueueTimer = Simulator::Now ();

}

void
TcpResequenceBuffer::FlushOutOrderQueue ()
{
  NS_LOG_FUNCTION (this);
  // Flush the data
  while (!m_outOrderQueue.empty ())
  {
    TcpResequenceBuffer::FlushOneElement (m_outOrderQueue.top ());
    m_outOrderQueue.pop ();
  }

  // Reset the timer
  m_outOrderQueueTimer = Simulator::Now ();
}

}
