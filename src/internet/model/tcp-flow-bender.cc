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
        .AddAttribute ("RTT", "RTT of the network",
            TimeValue (MicroSeconds (100)),
            MakeTimeAccessor (&TcpFlowBender::m_rtt),
            MakeTimeChecker ())
        .AddAttribute ("T", "The congestion degree within one RTT",
            DoubleValue (0.05),
            MakeDoubleAccessor (&TcpFlowBender::m_T),
            MakeDoubleChecker<double> (0))
        .AddAttribute ("N", "The number of allowed congestion RTT",
            UintegerValue (5),
            MakeUintegerAccessor (&TcpFlowBender::m_N),
            MakeUintegerChecker<uint32_t> ());

    return tid;
}

TcpFlowBender::TcpFlowBender ()
    :Object (),
     m_totalPackets (0),
     m_markedPackets (0),
     m_numCongestionRtt (0),
     m_V (0),
     m_checkEvent(),
     m_rtt(MicroSeconds (100)),
     m_T (0.05),
     m_N (5),
     m_totalPacketsStatis (0),
     m_markedPacketsStatis (0)
{
    NS_LOG_FUNCTION (this);
}

TcpFlowBender::TcpFlowBender (const TcpFlowBender &other)
    :Object (),
     m_totalPackets (other.m_totalPackets),
     m_markedPackets (other.m_markedPackets),
     m_numCongestionRtt (other.m_numCongestionRtt),
     m_V (other.m_V),
     m_checkEvent (),
     m_rtt (other.m_rtt),
     m_T (other.m_T),
     m_N (other.m_N),
     m_totalPacketsStatis (0),
     m_markedPacketsStatis (0)
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
    m_checkEvent.Cancel ();
    NS_LOG_INFO (this <<"Statistics: " << static_cast<double>(m_markedPacketsStatis) / m_totalPacketsStatis);
}

void
TcpFlowBender::ReceivedPacket ()
{
    if (!m_checkEvent.IsRunning ())
    {
        m_checkEvent = Simulator::Schedule (m_rtt, &TcpFlowBender::CheckEvent, this);
    }
    m_totalPackets++;
    m_totalPacketsStatis++;
}

void
TcpFlowBender::ReceivedMarkedPacket ()
{
    if (!m_checkEvent.IsRunning ())
    {
        m_checkEvent = Simulator::Schedule (m_rtt, &TcpFlowBender::CheckEvent, this);
    }
    m_totalPackets++;
    m_markedPackets++;
    m_totalPacketsStatis++;
    m_markedPacketsStatis++;
}

uint32_t
TcpFlowBender::GetV ()
{
    NS_LOG_INFO ("retuning V as " << m_V );
    return m_V;
}

void
TcpFlowBender::CheckEvent ()
{
    double f = static_cast<double> (m_markedPackets) / m_totalPackets;
    NS_LOG_LOGIC (this << "\tMarked packet: " << m_markedPackets
            << "\tTotal packet: " << m_totalPackets
            << "\tf: " << f);
    if (f > m_T)
    {
        m_numCongestionRtt ++;
        if (m_numCongestionRtt >= m_N)
        {
            m_numCongestionRtt = 0;
            // XXX Do we need to clear the congestion state
            m_V ++;
        }
    }
    else
    {
        m_numCongestionRtt = 0;
    }

    m_markedPackets = 0;
    m_totalPackets = 0;

    m_checkEvent = Simulator::Schedule (m_rtt, &TcpFlowBender::CheckEvent, this);
}

}
