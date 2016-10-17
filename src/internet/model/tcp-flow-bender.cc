#include "tcp-flow-bender.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"

NS_LOG_COMPONENT_DEFINE ("TcpFlowBender");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpFlowBender);

TypeId
TcpFlowBender::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::TcpFlowBender")
        .SetParent<Object> ()
        .SetGroupName ("Internent")
        .AddConstructor<TcpFlowBender> ()
         .AddAttribute ("T", "The congestion degree within one RTT",
            DoubleValue (0.05),
            MakeDoubleAccessor (&TcpFlowBender::m_T),
            MakeDoubleChecker<double> (0))
        .AddAttribute ("N", "The number of allowed congestion RTT",
            UintegerValue (1),
            MakeUintegerAccessor (&TcpFlowBender::m_N),
            MakeUintegerChecker<uint32_t> ());

    return tid;
}

TcpFlowBender::TcpFlowBender ()
    :Object (),
     m_totalBytes (0),
     m_markedBytes (0),
     m_numCongestionRtt (0),
     m_V (1),
     m_highTxMark (0),
     m_T (0.05),
     m_N (1)
{
    NS_LOG_FUNCTION (this);
}

TcpFlowBender::TcpFlowBender (const TcpFlowBender &other)
     :Object (),
     m_totalBytes (0),
     m_markedBytes (0),
     m_numCongestionRtt (0),
     m_V (1),
     m_highTxMark (0),
     m_T (other.m_T),
     m_N (other.m_N)
{
    NS_LOG_FUNCTION (this);
}

TcpFlowBender::~TcpFlowBender ()
{
    NS_LOG_FUNCTION (this);
}

void
TcpFlowBender::DoDispose (void)
{
    NS_LOG_FUNCTION (this);
}

void
TcpFlowBender::ReceivedPacket (SequenceNumber32 highTxhMark, SequenceNumber32 ackNumber,
        uint32_t ackedBytes, bool withECE)
{
    NS_LOG_INFO (this << " High TX Mark: " << m_highTxMark << ", ACK Number: " << ackNumber);
    m_totalBytes += ackedBytes;
    if (withECE)
    {
        m_markedBytes += ackedBytes;
    }
    if (ackNumber >= m_highTxMark)
    {
        m_highTxMark = highTxhMark;
        TcpFlowBender::CheckCongestion ();
    }
}


uint32_t
TcpFlowBender::GetV ()
{
    NS_LOG_INFO ( this << " retuning V as " << m_V );
    return m_V;
}

Time
TcpFlowBender::GetPauseTime ()
{
    return MicroSeconds (80);
}

void
TcpFlowBender::CheckCongestion ()
{
    double f = static_cast<double> (m_markedBytes) / m_totalBytes;
    NS_LOG_LOGIC (this << "\tMarked packet: " << m_markedBytes
                       << "\tTotal packet: " << m_totalBytes
                       << "\tf: " << f);
    if (f > m_T)
    {
        m_numCongestionRtt ++;
        if (m_numCongestionRtt >= m_N)
        {
            m_numCongestionRtt = 0;
            // XXX Do we need to clear the congestion state
            // m_V = m_V + 1 + (rand() % 10);
            m_V ++;
        }
    }
    else
    {
        m_numCongestionRtt = 0;
    }

    m_markedBytes = 0;
    m_totalBytes = 0;
}

}
