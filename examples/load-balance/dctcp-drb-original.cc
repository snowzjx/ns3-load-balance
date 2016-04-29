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

double lastCwndCheckTime;

std::stringstream cwndPlot;
std::stringstream congPlot;
std::stringstream queueDiscPlot;
std::stringstream queuePlot;
std::stringstream throughputPlot;

static void
CwndChange (uint32_t oldCwnd, uint32_t newCwnd)
{
    if (Simulator::Now().GetSeconds () - lastCwndCheckTime > 0.01)
    {
        NS_LOG_UNCOND ("Cwnd: " << Simulator::Now ().GetSeconds () << "\t" << newCwnd);
        lastCwndCheckTime = Simulator::Now ().GetSeconds ();
        std::ofstream fCwndPlot (cwndPlot.str ().c_str (), std::ios::out|std::ios::app);
        fCwndPlot << Simulator::Now ().GetSeconds () << " " << newCwnd << std::endl;
    }
}

static void
CongChange (TcpSocketState::TcpCongState_t oldCong, TcpSocketState::TcpCongState_t newCong)
{
    NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << TcpSocketState::TcpCongStateName[newCong]);
    std::ofstream fCongPlot (congPlot.str ().c_str (), std::ios::out|std::ios::app);
    fCongPlot << Simulator::Now ().GetSeconds () << " " << TcpSocketState::TcpCongStateName[newCong] << std::endl;
}

static void
TraceCwnd ()
{
    Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
            MakeCallback (&CwndChange));
}

static void
TraceCong ()
{
    Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongState",
            MakeCallback (&CongChange));
}

double avgQueueSize;

void
CheckQueueDiscSize (Ptr<QueueDisc> queue)
{
  uint32_t qSize = StaticCast<RedQueueDisc> (queue)->GetQueueSize ();

  Simulator::Schedule (Seconds (0.01), &CheckQueueDiscSize, queue);

  NS_LOG_UNCOND ("Queue disc size: " << qSize);

  std::ofstream fQueueDiscPlot (queueDiscPlot.str ().c_str (), std::ios::out|std::ios::app);
  fQueueDiscPlot << Simulator::Now ().GetSeconds () << " " << qSize << std::endl;
}

void
CheckQueueSize (Ptr<Queue> queue)
{
  uint32_t qSize = queue->GetNPackets ();

  Simulator::Schedule (Seconds (0.01), &CheckQueueSize, queue);

  NS_LOG_UNCOND ("Queue size: " << qSize);

  std::ofstream fQueuePlot (queuePlot.str ().c_str (), std::ios::out|std::ios::app);
  fQueuePlot << Simulator::Now ().GetSeconds () << " " << qSize << std::endl;

}

uint32_t accumRecvBytes;

void
CheckThroughput (Ptr<PacketSink> sink)
{
  uint32_t totalRecvBytes = sink->GetTotalRx ();
  uint32_t currentPeriodRecvBytes = totalRecvBytes - accumRecvBytes;

  accumRecvBytes = totalRecvBytes;

  Simulator::Schedule (Seconds (0.01), &CheckThroughput, sink);

  NS_LOG_UNCOND ("Throughput: " << currentPeriodRecvBytes * 8 / 0.01 << "bps");

  std::ofstream fThroughputPlot (throughputPlot.str ().c_str (), std::ios::out|std::ios::app);
  fThroughputPlot << Simulator::Now ().GetSeconds () << " " << currentPeriodRecvBytes * 8 / 0.01 <<std::endl;
}


int main (int argc, char *argv[])
{
#if 1
    LogComponentEnable ("DcTcpDrbOriginal", LOG_LEVEL_INFO);
#endif

    CommandLine cmd;
    cmd.Parse (argc, argv);

    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpDCTCP::GetTypeId ()));
    //Config::SetDefault ("ns3::TcpSocketBase::ECN", BooleanValue (false));

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

    Config::SetDefault ("ns3::RedQueueDisc::Mode", StringValue ("QUEUE_MODE_PACKETS"));
    Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (1040));
    Config::SetDefault ("ns3::RedQueueDisc::QueueLimit", UintegerValue (100));

    TrafficControlHelper tchRed;
    tchRed.SetRootQueueDisc ("ns3::RedQueueDisc", "MinTh", DoubleValue (8),
                                                  "MaxTh", DoubleValue (8));

    //p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (100));
    p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (10));
    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
    p2p.SetChannelAttribute ("Delay", TimeValue (MicroSeconds(100)));

    NetDeviceContainer d0d1 = p2p.Install (n0n1);
    QueueDiscContainer qd0d1 = tchRed.Install (d0d1);

    //p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (100));
    p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (10));
    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));

    NetDeviceContainer d1d2 = p2p.Install (n1n2);
    tchRed.Install (d1d2);

    NetDeviceContainer d2d4 = p2p.Install (n2n4);
    tchRed.Install (d2d4);

    NetDeviceContainer d4d5 = p2p.Install (n4n5);
    tchRed.Install (d4d5);

    p2p.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));

    TrafficControlHelper tchRed2;
    tchRed2.SetRootQueueDisc ("ns3::RedQueueDisc", "MinTh", DoubleValue (2),
                                                   "MaxTh", DoubleValue (2));

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

    // Uninstall the queue disc in sender
    /*
    TrafficControlHelper tc;
    tc.Uninstall(d0d1);
    tc.Uninstall(d1d2);
    tc.Uninstall(d1d3);
    tc.Uninstall(d2d4);
    tc.Uninstall(d3d4);
    tc.Uninstall(d4d5);
    */

    NS_LOG_INFO ("Enable k DRB");
    Ipv4DrbHelper drb;
    Ptr<Ipv4> ip1 = c.Get(1)->GetObject<Ipv4>();
    Ptr<Ipv4Drb> ipv4Drb1 = drb.GetIpv4Drb(ip1);
    ipv4Drb1->AddCoreSwitchAddress(200, i1i2.GetAddress (1));
    ipv4Drb1->AddCoreSwitchAddress(200, i1i3.GetAddress (1));

    Ptr<Ipv4> ip4 = c.Get(4)->GetObject<Ipv4>();
    Ptr<Ipv4Drb> ipv4Drb4 = drb.GetIpv4Drb(ip4);
    ipv4Drb4->AddCoreSwitchAddress(200, i2i4.GetAddress (0));
    ipv4Drb4->AddCoreSwitchAddress(200, i3i4.GetAddress (0));

    /*
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
    */

    NS_LOG_INFO ("Setting up routing table");

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    NS_LOG_INFO ("Install TCP based application");

    uint16_t port = 8080;

    BulkSendHelper source ("ns3::TcpSocketFactory",
            InetSocketAddress (i4i5.GetAddress(1), port));
    source.SetAttribute ("MaxBytes", UintegerValue (0));
    ApplicationContainer sourceApps = source.Install (c.Get (0));
    sourceApps.Start (Seconds (0.0));
    sourceApps.Stop (Seconds (20.0));

    PacketSinkHelper sink ("ns3::TcpSocketFactory",
            InetSocketAddress (Ipv4Address::GetAny (), port));
    ApplicationContainer sinkApp = sink.Install (c.Get(5));
    sinkApp.Start (Seconds (0.0));
    sinkApp.Stop (Seconds (20.0));

    cwndPlot << "cwnd.plotme";
    congPlot << "cong.plotme";
    queueDiscPlot << "queue_disc.plotme";
    queuePlot << "queue.plotme";
    throughputPlot << "throughput.plotme";

    remove(cwndPlot.str ().c_str ());
    remove(congPlot.str ().c_str ());
    remove(queueDiscPlot.str ().c_str ());
    remove(queuePlot.str ().c_str ());
    remove(throughputPlot.str ().c_str ());

    Simulator::Schedule (Seconds (0.00001), &TraceCwnd);
    Simulator::Schedule (Seconds (0.00001), &TraceCong);

    Ptr<QueueDisc> queueDisc = qd0d1.Get (0);
    Simulator::ScheduleNow (&CheckQueueDiscSize, queueDisc);

    Ptr<NetDevice> nd = d0d1.Get (0);
    Ptr<Queue> queue = DynamicCast<PointToPointNetDevice>(nd)->GetQueue ();
    Simulator::ScheduleNow (&CheckQueueSize, queue);

    Ptr<PacketSink> pktSink = sinkApp.Get (0)->GetObject<PacketSink> ();
    Simulator::ScheduleNow (&CheckThroughput, pktSink);

    NS_LOG_INFO ("Run Simulations");

    Simulator::Stop (Seconds (20.0));
    Simulator::Run ();
    Simulator::Destroy ();

    return 0;
}
