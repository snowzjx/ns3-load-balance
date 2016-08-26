/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "congestion-probing.h"

#include "congestion-probing-tag.h"

#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/packet.h"
#include "ns3/socket.h"
#include "ns3/ipv4-raw-socket-factory.h"
#include "ns3/ipv4-header.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/flow-id-tag.h"

#include <sys/socket.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CongestionProbing");

NS_OBJECT_ENSURE_REGISTERED (CongestionProbing);

TypeId
CongestionProbing::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::CongestionProbing")
        .SetParent<Object> ()
        .SetGroupName ("Internet")
        .AddConstructor<CongestionProbing> ()
        .AddAttribute ("MaxV",
                "The max V used in probing",
                UintegerValue (8),
                MakeUintegerAccessor (&CongestionProbing::m_maxV),
                MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("ProbingInterval",
                "Time interval between probings",
                TimeValue (MicroSeconds (100)),
                MakeTimeAccessor (&CongestionProbing::m_probeInterval),
                MakeTimeChecker ())
        .AddTraceSource ("Probing",
                "The probing event callback",
                MakeTraceSourceAccessor (&CongestionProbing::m_probingCallback),
                "ns3::CongestionProbing::ProbingCallback");

    return tid;
}

TypeId
CongestionProbing::GetInstanceTypeId () const
{
    return CongestionProbing::GetTypeId ();
}

CongestionProbing::CongestionProbing ()
    : m_V (0),
      m_probeEvent (),
      m_probeAddress (Ipv4Address ("127.0.0.1")),
      m_flowId (0),
      m_node (),
      m_maxV (8),
      m_probeInterval (MicroSeconds (100))
{
    NS_LOG_FUNCTION (this);
}

CongestionProbing::CongestionProbing (const CongestionProbing &other)
    : m_V (0),
      m_probeEvent (),
      m_probeAddress (other.m_probeAddress),
      m_flowId (other.m_flowId),
      m_node (),
      m_maxV (other.m_maxV),
      m_probeInterval (other.m_probeInterval),
      m_probingCallback (other.m_probingCallback)
{
    NS_LOG_FUNCTION (this);
}

CongestionProbing::~CongestionProbing ()
{
    NS_LOG_FUNCTION (this);
}

void
CongestionProbing::DoDispose ()
{
    m_probeEvent.Cancel ();
}

void
CongestionProbing::SetSourceAddress (Ipv4Address address)
{
    m_sourceAddress = address;
}

void
CongestionProbing::SetProbeAddress (Ipv4Address address)
{
    m_probeAddress = address;
}

void
CongestionProbing::SetFlowId (uint32_t flowId)
{
    m_flowId = flowId;
}

void
CongestionProbing::SetNode (Ptr<Node> node)
{
    m_node = node;
}

void
CongestionProbing::StartProbe ()
{
    Simulator::ScheduleNow (&CongestionProbing::DoStartProbe, this);
}

void
CongestionProbing::DoStartProbe ()
{
    NS_LOG_LOGIC (this << "Start probing with interval: " << m_probeInterval);

    m_socket = m_node->GetObject<Ipv4RawSocketFactory> ()->CreateSocket ();
    m_socket->SetRecvCallback (MakeCallback (&CongestionProbing::ReceivePacket, this));
    m_socket->Bind (InetSocketAddress (Ipv4Address ("0.0.0.0"), 0));
    m_socket->SetAttribute ("IpHeaderInclude", BooleanValue (true));

    m_probeEvent = Simulator::ScheduleNow (&CongestionProbing::ProbeEvent, this);
}

void
CongestionProbing::DoStopProbe ()
{
    m_probeEvent.Cancel ();
}

void
CongestionProbing::StopProbe (Time time)
{
    Simulator::Schedule (time, &CongestionProbing::DoStopProbe, this);
}

void
CongestionProbing::ProbeEvent ()
{
    Address to = InetSocketAddress (m_probeAddress, 0);

    Ptr<Packet> packet = Create<Packet> (0);
    Ipv4Header newHeader;
    newHeader.SetSource (m_sourceAddress);
    newHeader.SetDestination (m_probeAddress);
    newHeader.SetProtocol (0);
    newHeader.SetPayloadSize (packet->GetSize ());
    newHeader.SetEcn (Ipv4Header::ECN_ECT1);
    newHeader.SetTtl (255);
    packet->AddHeader (newHeader);

    // Flow Id tag
    FlowIdTag flowIdTag (m_flowId ^ m_V); // When flowId ^ V, the ECMP path is changed
    packet->AddPacketTag (flowIdTag);

    // Probing tag
    CongestionProbingTag probingTag;
    probingTag.SetV (m_V);
    probingTag.SetIsReply (0);
    probingTag.SetSendTime (Simulator::Now ());
    probingTag.SetIsCE (0);
    packet->AddPacketTag (probingTag);

    m_socket->SendTo (packet, 0, to);

    m_V = (m_V + 1) % m_maxV;
    m_probeEvent = Simulator::Schedule (m_probeInterval / m_maxV, &CongestionProbing::ProbeEvent, this);
}

void
CongestionProbing::ReceivePacket (Ptr<Socket> socket)
{
    Ptr<Packet> packet = m_socket->Recv (std::numeric_limits<uint32_t>::max (), MSG_PEEK);

    CongestionProbingTag probingTag;
    bool found = packet->RemovePacketTag (probingTag);

    if (!found)
    {
        return;
    }

    Ipv4Header ipv4Header;
    found = packet->RemoveHeader (ipv4Header);

    /*
    NS_LOG_LOGIC (this << "PacketTag: " << probingTag.GetV ()
            << "/" << probingTag.GetIsReply ()
            << "/" << probingTag.GetSendTime ());

    */

    NS_LOG_LOGIC (this << "Ipv4 Header: " << ipv4Header);

    if (probingTag.GetIsReply () == 0)
    {
        // Reply to this packet
        Ptr<Packet> newPacket = Create<Packet> (0);
        Ipv4Header newHeader;
        newHeader.SetSource (m_sourceAddress);
        newHeader.SetDestination (ipv4Header.GetSource ());
        newHeader.SetProtocol (0);
        newHeader.SetPayloadSize (packet->GetSize ());
        newHeader.SetTtl (255);
        newPacket->AddHeader (newHeader);

        CongestionProbingTag replyProbingTag;
        replyProbingTag.SetV (probingTag.GetV ());
        replyProbingTag.SetIsReply (1);
        replyProbingTag.SetSendTime (probingTag.GetSendTime ());
        if (ipv4Header.GetEcn () == Ipv4Header::ECN_CE)
        {
            replyProbingTag.SetIsCE (1);
        }
        else
        {
            replyProbingTag.SetIsCE (0);
        }
        newPacket->AddPacketTag (replyProbingTag);

        Address to = InetSocketAddress (ipv4Header.GetSource (), 0);

        m_socket->SendTo (newPacket, 0, to);
    }
    else
    {
        // Raise an event
        uint32_t v = probingTag.GetV ();
        Time rtt = Simulator::Now () - probingTag.GetSendTime ();
        bool isCE = probingTag.GetIsCE () == 1 ? true : false;
        m_probingCallback (v, packet, ipv4Header, rtt, isCE);
    }
}

}

