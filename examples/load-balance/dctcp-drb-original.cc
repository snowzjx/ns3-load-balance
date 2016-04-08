#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/ipv4-drb-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DcTcpDrbOriginal");

class MyApp : public Application
{
public:

  MyApp ();
  virtual ~MyApp();

  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp()
{
  m_socket = 0;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

static void
CwndChange (uint32_t oldCwnd, uint32_t newCwnd)
{
    NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
}

static void
CongChange (TcpSocketState::TcpCongState_t oldCong, TcpSocketState::TcpCongState_t newCong)
{
    NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << TcpSocketState::TcpCongStateName[newCong]);
}

uint32_t checkTimes;
double avgQueueSize;

std::stringstream filePlotQueue;
std::stringstream filePlotQueueAvg;

void
CheckQueueSize (Ptr<QueueDisc> queue)
{
  uint32_t qSize = StaticCast<RedQueueDisc> (queue)->GetQueueSize ();

  avgQueueSize += qSize;
  checkTimes++;

  // check queue size every 1/100 of a second
  Simulator::Schedule (Seconds (0.01), &CheckQueueSize, queue);

  std::ofstream fPlotQueue (filePlotQueue.str ().c_str (), std::ios::out|std::ios::app);
  fPlotQueue << Simulator::Now ().GetSeconds () << " " << qSize << std::endl;
  fPlotQueue.close ();

  std::ofstream fPlotQueueAvg (filePlotQueueAvg.str ().c_str (), std::ios::out|std::ios::app);
  fPlotQueueAvg << Simulator::Now ().GetSeconds () << " " << avgQueueSize / checkTimes << std::endl;
  fPlotQueueAvg.close ();
}


int main (int argc, char *argv[])
{
#if 1
    LogComponentEnable ("DcTcpDrbOriginal", LOG_LEVEL_INFO);
#endif

    CommandLine cmd;
    cmd.Parse (argc, argv);

    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpDCTCP::GetTypeId ()));

    NS_LOG_INFO ("Create nodes.");
    NodeContainer c;
    c.Create (6);

    NodeContainer n0n1 = NodeContainer (c.Get (0), c.Get(1));
    NodeContainer n1n2 = NodeContainer (c.Get (1), c.Get(2));
    NodeContainer n1n3 = NodeContainer (c.Get (1), c.Get(3));
    NodeContainer n2n4 = NodeContainer (c.Get (2), c.Get(4));
    NodeContainer n3n4 = NodeContainer (c.Get (3), c.Get(4));
    NodeContainer n4n5 = NodeContainer (c.Get (4), c.Get(5));

    NS_LOG_INFO ("Install Internet stack");
    InternetStackHelper internet;
    internet.Install (c.Get (0));
    internet.Install (c.Get (2));
    internet.Install (c.Get (3));
    internet.Install (c.Get (5));
    internet.SetDrb (true);
    internet.Install (c.Get (1));
    internet.Install (c.Get (4));

    NS_LOG_INFO ("Install channel");
    PointToPointHelper p2p;
    p2p.SetQueue ("ns3::DropTailQueue");
    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
    p2p.SetChannelAttribute ("Delay", TimeValue (MicroSeconds(100)));

    NetDeviceContainer d0d1 = p2p.Install (n0n1);

    Config::SetDefault ("ns3::RedQueueDisc::Mode", StringValue ("QUEUE_MODE_PACKETS"));
    Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (1040));
    Config::SetDefault ("ns3::RedQueueDisc::QueueLimit", UintegerValue (1000));

    TrafficControlHelper tchRed;
    tchRed.SetRootQueueDisc ("ns3::RedQueueDisc", "LinkBandwidth", StringValue ("10Gbps"),
                                                  "LinkDelay", TimeValue (MicroSeconds (100)),
                                                  "MinTh", DoubleValue (65),
                                                  "MaxTh", DoubleValue (65));
    NetDeviceContainer d1d2 = p2p.Install (n1n2);
    QueueDiscContainer queueDisc12 = tchRed.Install (d1d2);
    NetDeviceContainer d2d4 = p2p.Install (n2n4);
    tchRed.Install (d2d4);
    NetDeviceContainer d4d5 = p2p.Install (n4n5);
    tchRed.Install (d4d5);

    p2p.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));

    TrafficControlHelper tchRed2;
    tchRed2.SetRootQueueDisc ("ns3::RedQueueDisc", "LinkBandwidth", StringValue ("1Gbps"),
                                                   "LinkDelay", TimeValue (MicroSeconds (100)),
                                                   "MinTh", DoubleValue (10),
                                                   "MaxTh", DoubleValue (10));

    NetDeviceContainer d1d3 = p2p.Install (n1n3);
    tchRed2.Install (d1d3);
    NetDeviceContainer d3d4 = p2p.Install (n3n4);
    tchRed2.Install (d3d4);

    NS_LOG_INFO ("Assign IP address");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i0i1 = ipv4.Assign (d0d1);
    ipv4.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer i1i2 = ipv4.Assign (d1d2);
    ipv4.SetBase ("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer i2i4 = ipv4.Assign (d2d4);
    ipv4.SetBase ("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer i1i3 = ipv4.Assign (d1d3);
    ipv4.SetBase ("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer i3i4 = ipv4.Assign (d3d4);
    ipv4.SetBase ("10.1.6.0", "255.255.255.0");
    Ipv4InterfaceContainer i4i5 = ipv4.Assign (d4d5);

    NS_LOG_INFO ("Enable DRB");
    Ipv4DrbHelper drb;
    Ptr<Ipv4> ip1 = c.Get(1)->GetObject<Ipv4>();
    Ptr<Ipv4Drb> ipv4Drb1 = drb.GetIpv4Drb(ip1);
    ipv4Drb1->AddCoreSwitchAddress(i1i2.GetAddress (1));
    ipv4Drb1->AddCoreSwitchAddress(i1i3.GetAddress (1));

    Ptr<Ipv4> ip4 = c.Get(4)->GetObject<Ipv4>();
    Ptr<Ipv4Drb> ipv4Drb4 = drb.GetIpv4Drb(ip4);
    ipv4Drb4->AddCoreSwitchAddress(i2i4.GetAddress (0));
    ipv4Drb4->AddCoreSwitchAddress(i3i4.GetAddress (0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    NS_LOG_INFO ("Install TCP based application");
    uint16_t port = 8080;
    Address sinkAddress (InetSocketAddress (i4i5.GetAddress(1), port));
    Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (c.Get(0), TcpSocketFactory::GetTypeId ());
    ns3TcpSocket->TraceConnectWithoutContext("CongestionWindow", MakeCallback (&CwndChange));
    ns3TcpSocket->TraceConnectWithoutContext("CongState", MakeCallback (&CongChange));

    Ptr<MyApp> app = CreateObject<MyApp> ();
    app->Setup (ns3TcpSocket, sinkAddress, 1040, 1000000, DataRate ("10Gbps"));
    c.Get(0)->AddApplication (app);
    app->SetStartTime (Seconds (1.0));
    app->SetStopTime (Seconds (10.0));

    PacketSinkHelper sink ("ns3::TcpSocketFactory",
            InetSocketAddress (Ipv4Address::GetAny (), port));
    ApplicationContainer sinkApp = sink.Install (c.Get(5));
    sinkApp.Start (Seconds (0.1));
    sinkApp.Stop (Seconds (10.0));

    filePlotQueue << "red-queue.plotme";
    filePlotQueueAvg << "red-queue-avg.plotme";
    Ptr<QueueDisc> queue = queueDisc12.Get (0);
    Simulator::ScheduleNow (&CheckQueueSize, queue);

    Simulator::Run ();
    Simulator::Destroy ();

    return 0;
}
