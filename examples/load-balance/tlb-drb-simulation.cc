#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/ipv4-drb-helper.h"
#include "ns3/ipv4-xpath-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/ipv4-tlb.h"
#include "ns3/ipv4-tlb-probing.h"

#include "ns3/gnuplot.h"

#include <map>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TLBDRBSimulation");

double lastCwndCheckTime;

std::string cwndPlot = "cwnd.plotme";
std::string cwndPlot2 = "cwnd_2.plotme";
std::string congPlot = "cong.plotme";
std::string congPlot2 = "cong_2.plotme";

std::string queueDiscPlot = "queue_disc.plotme";
std::string queuePlot = "queue.plotme";
std::string queueDiscPlot3 = "queue_disc_3.plotme";
std::string queuePlot3 = "queue_3.plotme";
std::string queueDiscPlot2 = "queue_disc_2.plotme";
std::string queuePlot2 = "queue_2.plotme";
std::string queuePlot4 = "queue_4.plotme";
std::string queueDiscPlot4 = "queue_disc_4.plotme";

std::string throughputPlot = "throughput.plotme";

Gnuplot2dDataset cwndDataset;
Gnuplot2dDataset cwnd2Dataset;
Gnuplot2dDataset queueDiscDataset;
Gnuplot2dDataset queueDataset;
Gnuplot2dDataset queueDisc2Dataset;
Gnuplot2dDataset queue2Dataset;
Gnuplot2dDataset queueDisc3Dataset;
Gnuplot2dDataset queue3Dataset;
Gnuplot2dDataset queueDisc4Dataset;
Gnuplot2dDataset queue4Dataset;
Gnuplot2dDataset throughputDataset;

void
DoGnuPlot (std::string transportProt) {
    Gnuplot cwndGnuPlot ("cwnd.png");
    cwndGnuPlot.SetTitle("Cwnd");
    cwndGnuPlot.SetTerminal("png");
    cwndGnuPlot.AddDataset (cwndDataset);
    std::ofstream cwndPlotFile ("cwnd.plt");
    cwndGnuPlot.GenerateOutput (cwndPlotFile);
    cwndPlotFile.close();

    Gnuplot cwnd2GnuPlot ("cwnd_2.png");
    cwndGnuPlot.SetTitle("Cwnd2");
    cwndGnuPlot.SetTerminal("png");
    cwndGnuPlot.AddDataset (cwnd2Dataset);
    std::ofstream cwnd2PlotFile ("cwnd_2.plt");
    cwndGnuPlot.GenerateOutput (cwnd2PlotFile);
    cwndPlotFile.close();


    Gnuplot queueGnuPlot ("queue.png");
    queueGnuPlot.SetTitle("Queue");
    queueGnuPlot.SetTerminal("png");
    queueGnuPlot.AddDataset (queueDataset);
    std::ofstream queuePlotFile ("queue.plt");
    queueGnuPlot.GenerateOutput (queuePlotFile);
    queuePlotFile.close();

    Gnuplot queue2GnuPlot ("queue_2.png");
    queue2GnuPlot.SetTitle("Queue");
    queue2GnuPlot.SetTerminal("png");
    queue2GnuPlot.AddDataset (queue2Dataset);
    std::ofstream queue2PlotFile ("queue_2.plt");
    queue2GnuPlot.GenerateOutput (queue2PlotFile);
    queue2PlotFile.close();

    Gnuplot queue3GnuPlot ("queue_3.png");
    queue3GnuPlot.SetTitle("Queue 3");
    queue3GnuPlot.SetTerminal("png");
    queue3GnuPlot.AddDataset (queue3Dataset);
    std::ofstream queue3PlotFile ("queue_3.plt");
    queue3GnuPlot.GenerateOutput (queue3PlotFile);
    queue3PlotFile.close();

    Gnuplot queue4GnuPlot ("queue_4.png");
    queue4GnuPlot.SetTitle("Queue 4");
    queue4GnuPlot.SetTerminal("png");
    queue4GnuPlot.AddDataset (queue4Dataset);
    std::ofstream queue4PlotFile ("queue_4.plt");
    queue4GnuPlot.GenerateOutput (queue4PlotFile);
    queue4PlotFile.close();

    if (transportProt.compare ("DcTcp") == 0)
    {
        Gnuplot queueDiscGnuPlot ("queue_disc.png");
        queueDiscGnuPlot.SetTitle("Queue Disc");
        queueDiscGnuPlot.SetTerminal("png");
        queueDiscGnuPlot.AddDataset (queueDiscDataset);
        std::ofstream queueDiscPlotFile ("queue_disc.plt");
        queueDiscGnuPlot.GenerateOutput (queueDiscPlotFile);
        queueDiscPlotFile.close();

        Gnuplot queueDisc2GnuPlot ("queue_disc_2.png");
        queueDisc2GnuPlot.SetTitle("Queue Disc 2");
        queueDisc2GnuPlot.SetTerminal("png");
        queueDisc2GnuPlot.AddDataset (queueDisc2Dataset);
        std::ofstream queueDisc2PlotFile ("queue_disc_2.plt");
        queueDisc2GnuPlot.GenerateOutput (queueDisc2PlotFile);
        queueDisc2PlotFile.close();

        Gnuplot queueDisc3GnuPlot ("queue_disc_3.png");
        queueDisc3GnuPlot.SetTitle("Queue Disc 3");
        queueDisc3GnuPlot.SetTerminal("png");
        queueDisc3GnuPlot.AddDataset (queueDisc3Dataset);
        std::ofstream queueDisc3PlotFile ("queue_disc_3.plt");
        queueDisc3GnuPlot.GenerateOutput (queueDisc3PlotFile);
        queueDisc3PlotFile.close();

        Gnuplot queueDisc4GnuPlot ("queue_disc_4.png");
        queueDisc4GnuPlot.SetTitle("Queue Disc 4");
        queueDisc4GnuPlot.SetTerminal("png");
        queueDisc4GnuPlot.AddDataset (queueDisc4Dataset);
        std::ofstream queueDisc4PlotFile ("queue_disc_4.plt");
        queueDisc4GnuPlot.GenerateOutput (queueDisc4PlotFile);
        queueDisc4PlotFile.close();
    }

    Gnuplot throughputGnuPlot ("throughput.png");
    throughputGnuPlot.SetTitle("Throughtput");
    throughputGnuPlot.SetTerminal("png");
    throughputGnuPlot.AddDataset (throughputDataset);
    std::ofstream throughputPlotFile ("throughput.plt");
    throughputGnuPlot.GenerateOutput (throughputPlotFile);
    throughputPlotFile.close();
}

static void
CwndChange (uint32_t oldCwnd, uint32_t newCwnd)
{
    if (Simulator::Now().GetSeconds () - lastCwndCheckTime > 0.001)
    {
        //NS_LOG_UNCOND ("Cwnd: " << Simulator::Now ().GetSeconds () << "\t" << newCwnd);
        lastCwndCheckTime = Simulator::Now ().GetSeconds ();
        std::ofstream fCwndPlot (cwndPlot.c_str (), std::ios::out|std::ios::app);
        fCwndPlot << Simulator::Now ().GetSeconds () << " " << newCwnd << std::endl;

        cwndDataset.Add(Simulator::Now ().GetSeconds (), newCwnd);
    }
}

static void
CongChange (TcpSocketState::TcpCongState_t oldCong, TcpSocketState::TcpCongState_t newCong)
{
    //NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << TcpSocketState::TcpCongStateName[newCong]);
    std::ofstream fCongPlot (congPlot.c_str (), std::ios::out|std::ios::app);
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
CheckQueueDiscSize (Ptr<QueueDisc> queue, std::string plot, Gnuplot2dDataset *dataset)
{
  uint32_t qSize = StaticCast<RedQueueDisc> (queue)->GetQueueSize ();

  Simulator::Schedule (Seconds (0.00001), &CheckQueueDiscSize, queue, plot, dataset);

  //NS_LOG_UNCOND ("Queue disc size: " << qSize);

  std::ofstream fQueueDiscPlot (plot.c_str (), std::ios::out|std::ios::app);
  fQueueDiscPlot << Simulator::Now ().GetSeconds () << " " << qSize << std::endl;

  dataset->Add (Simulator::Now ().GetSeconds (), qSize);
}

void
CheckQueueSize (Ptr<Queue> queue, std::string plot, Gnuplot2dDataset *dataset)
{
  uint32_t qSize = queue->GetNPackets ();

  Simulator::Schedule (Seconds (0.001), &CheckQueueSize, queue, plot, dataset);

  //NS_LOG_UNCOND ("Queue size: " << qSize);

  std::ofstream fQueuePlot (plot.c_str (), std::ios::out|std::ios::app);
  fQueuePlot << Simulator::Now ().GetSeconds () << " " << qSize << std::endl;

  dataset->Add (Simulator::Now ().GetSeconds (), qSize);
}

uint32_t accumRecvBytes;

std::map<std::string, uint32_t> accumBytes;

void
CheckThroughput (Ptr<PacketSink> sink, std::string plot)
{
  uint32_t totalRecvBytes = sink->GetTotalRx ();

  uint32_t currentPeriodRecvBytes = totalRecvBytes - accumBytes[plot];
  accumBytes[plot] = totalRecvBytes;

  NS_LOG_UNCOND ("Throughput: " << currentPeriodRecvBytes * 8 / 0.001 << "bps");

  std::ofstream fThroughputPlot (plot.c_str (), std::ios::out|std::ios::app);
  fThroughputPlot << Simulator::Now ().GetSeconds () << "\t" << currentPeriodRecvBytes * 8 / 0.001 <<std::endl;

  Simulator::Schedule (Seconds (0.001), &CheckThroughput, sink, plot);
  //throughputDataset.Add (Simulator::Now().GetSeconds (), currentPeriodRecvBytes * 8 / 0.001);
}


int main (int argc, char *argv[])
{
#if 1
    LogComponentEnable ("TLBDRBSimulation", LOG_LEVEL_INFO);
#endif

	Config::SetDefault	("ns3::TcpSocket::SegmentSize",	UintegerValue(1400));
	Config::SetDefault	("ns3::TcpSocket::DelAckCount",	UintegerValue	(0));
	Config::SetDefault	("ns3::TcpSocket::ConnTimeout",	TimeValue	(MilliSeconds	(5)));
	Config::SetDefault	("ns3::TcpSocket::InitialCwnd",	UintegerValue	(10));
	Config::SetDefault	("ns3::TcpSocket::SndBufSize",	UintegerValue	(160000000));
	Config::SetDefault	("ns3::TcpSocket::RcvBufSize",	UintegerValue	(160000000));
	Config::SetDefault	("ns3::TcpSocketBase::MinRto",	TimeValue	(MilliSeconds	(5)));
	Config::SetDefault	("ns3::TcpSocketBase::ClockGranularity",	TimeValue	(MicroSeconds	(100)));
	Config::SetDefault	("ns3::RttEstimator::InitialEstimation",	TimeValue	(MicroSeconds	(80)));
    Config::SetDefault	("ns3::TcpSocketBase::ReTxThreshold",	UintegerValue	(1000));

    std::string transportProt = "DcTcp";

    CommandLine cmd;
    cmd.AddValue ("transportProt", "Transport protocol to use: Tcp, DcTcp", transportProt);

    cmd.Parse (argc, argv);

    if (transportProt.compare ("Tcp") == 0)
    {
        Config::SetDefault ("ns3::TcpSocketBase::ECN", BooleanValue (false));
    }
    else if (transportProt.compare ("DcTcp") == 0)
    {
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpDCTCP::GetTypeId ()));
    }
    else
    {
        return 0;
    }

    NS_LOG_INFO ("Create nodes.");
    NodeContainer c;
    c.Create (9);

    NodeContainer n0n1 = NodeContainer (c.Get (0), c.Get(1));
    NodeContainer n1n2 = NodeContainer (c.Get (1), c.Get(2));
    NodeContainer n1n3 = NodeContainer (c.Get (1), c.Get(3));
    NodeContainer n2n4 = NodeContainer (c.Get (2), c.Get(4));
    NodeContainer n3n4 = NodeContainer (c.Get (3), c.Get(4));
    NodeContainer n4n5 = NodeContainer (c.Get (4), c.Get(5));
    NodeContainer n7n8 = NodeContainer (c.Get (7), c.Get(8));
    NodeContainer n8n3 = NodeContainer (c.Get (8), c.Get(3));
    NodeContainer n4n6 = NodeContainer (c.Get (4), c.Get(6));

    NS_LOG_INFO ("Install Internet stack");
    InternetStackHelper internet;

    internet.Install (c.Get (0));
    internet.Install (c.Get (5));
    internet.Install (c.Get (6));
    internet.Install (c.Get (7));
    internet.Install (c.Get (8));
    internet.Install (c.Get (2));
    internet.Install (c.Get (3));
    internet.SetDrb (true);
    internet.Install (c.Get (4));
    internet.Install (c.Get (1));


    NS_LOG_INFO ("Install channel");
    PointToPointHelper p2p;

	Config::SetDefault	("ns3::RedQueueDisc::Mode",	StringValue	("QUEUE_MODE_BYTES"));
	Config::SetDefault	("ns3::RedQueueDisc::MeanPktSize",	UintegerValue	(1400));
    Config::SetDefault	("ns3::RedQueueDisc::QueueLimit",	UintegerValue	(550 * 1400));
    Config::SetDefault	("ns3::RedQueueDisc::Gentle",	BooleanValue	(false));

    TrafficControlHelper tc;
    tc.SetRootQueueDisc ("ns3::RedQueueDisc", "MinTh", DoubleValue (65 * 1400),
                                              "MaxTh", DoubleValue (65 * 1400));

    if (transportProt.compare ("Tcp") == 0)
    {
        p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (100));
    }
    else
    {
        p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (10));
    }

    p2p.SetDeviceAttribute ("DataRate", StringValue ("100Gbps"));
    p2p.SetChannelAttribute ("Delay", TimeValue (MicroSeconds(10)));

    NetDeviceContainer d0d1 = p2p.Install (n0n1);
    QueueDiscContainer qd0d1 = tc.Install (d0d1);
    NS_LOG_INFO ("Node 0 is connected to node 1 with port " << d0d1.Get (0)->GetIfIndex () << "<->" << d0d1.Get (1)->GetIfIndex ());

    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));

    NetDeviceContainer d1d2 = p2p.Install (n1n2);
    NS_LOG_INFO ("Node 1 is connected to node 2 with port " << d1d2.Get (0)->GetIfIndex () << "<->" << d1d2.Get (1)->GetIfIndex ());
    QueueDiscContainer qd1d2 = tc.Install (d1d2);

    NetDeviceContainer d2d4 = p2p.Install (n2n4);
    NS_LOG_INFO ("Node 2 is connected to node 4 with port " << d2d4.Get (0)->GetIfIndex () << "<->" << d2d4.Get (1)->GetIfIndex ());
    QueueDiscContainer qd2d4 = tc.Install (d2d4);

    p2p.SetDeviceAttribute ("DataRate", StringValue ("100Gbps"));

    NetDeviceContainer d4d5 = p2p.Install (n4n5);
    NS_LOG_INFO ("Node 4 is connected to node 5 with port " << d4d5.Get (0)->GetIfIndex () << "<->" << d4d5.Get (1)->GetIfIndex ());
    QueueDiscContainer qd4d5 = tc.Install (d4d5);

    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));

    NetDeviceContainer d7d8 = p2p.Install (n7n8);
    NS_LOG_INFO ("Node 7 is connected to node 8 with port " << d7d8.Get (0)->GetIfIndex () << "<->" << d7d8.Get (1)->GetIfIndex ());
    QueueDiscContainer qd7d8 = tc.Install (d7d8);

    NetDeviceContainer d8d3 = p2p.Install (n8n3);
    NS_LOG_INFO ("Node 8 is connected to node 3 with port " << d8d3.Get (0)->GetIfIndex () << "<->" << d8d3.Get (1)->GetIfIndex ());
    QueueDiscContainer qd8d3 = tc.Install (d8d3);

    NetDeviceContainer d4d6 = p2p.Install (n4n6);
    NS_LOG_INFO ("Node 4 is connected to node 6 with port " << d4d6.Get (0)->GetIfIndex () << "<->" << d4d6.Get (1)->GetIfIndex ());
    QueueDiscContainer qd4d6 = tc.Install (d4d6);

    NetDeviceContainer d1d3 = p2p.Install (n1n3);
    NS_LOG_INFO ("Node 1 is connected to node 3 with port " << d1d3.Get (0)->GetIfIndex () << "<->" << d1d3.Get (1)->GetIfIndex ());
    QueueDiscContainer qd1d3 = tc.Install (d1d3);

    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));

    Config::SetDefault ("ns3::RedQueueDisc::DropRate", DoubleValue (0.0));

    NetDeviceContainer d3d4 = p2p.Install (n3n4);
    NS_LOG_INFO ("Node 3 is connected to node 4 with port " << d3d4.Get (0)->GetIfIndex () << "<->" << d3d4.Get (1)->GetIfIndex ());
    QueueDiscContainer qd3d4 = tc.Install (d3d4);

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
    ipv4.SetBase ("10.1.7.0", "255.255.255.0");
    Ipv4InterfaceContainer i7i8 = ipv4.Assign (d7d8);
    ipv4.SetBase ("10.1.8.0", "255.255.255.0");
    Ipv4InterfaceContainer i8i3 = ipv4.Assign (d8d3);
    ipv4.SetBase ("10.1.9.0", "255.255.255.0");
    Ipv4InterfaceContainer i4i6 = ipv4.Assign (d4d6);

    // Uninstall the queue disc in sender
    if (transportProt.compare ("Tcp") == 0)
    {
        TrafficControlHelper tc;
        tc.Uninstall(d0d1);
        tc.Uninstall(d1d2);
        tc.Uninstall(d1d3);
        tc.Uninstall(d2d4);
        tc.Uninstall(d3d4);
        tc.Uninstall(d4d5);
        tc.Uninstall(d7d8);
        tc.Uninstall(d8d3);
        tc.Uninstall(d4d6);
    }

    NS_LOG_INFO ("Setting up DRB");
    Ipv4DrbHelper drb;
    Ptr<Ipv4> ip1 = c.Get(1)->GetObject<Ipv4>();
    Ptr<Ipv4Drb> ipv4Drb1 = drb.GetIpv4Drb(ip1);
    ipv4Drb1->AddCoreSwitchAddress(64, i1i2.GetAddress (1));
    ipv4Drb1->AddCoreSwitchAddress(64, i1i3.GetAddress (1));

    Ptr<Ipv4> ip4 = c.Get(4)->GetObject<Ipv4>();
    Ptr<Ipv4Drb> ipv4Drb4 = drb.GetIpv4Drb(ip4);
    ipv4Drb4->AddCoreSwitchAddress(64, i2i4.GetAddress (0));
    ipv4Drb4->AddCoreSwitchAddress(64, i3i4.GetAddress (0));


    NS_LOG_INFO ("Setting up routing table");

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    NS_LOG_INFO ("Install TCP based application");

    uint16_t port = 8080;

    for (int i = 0; i < 6; i++)
    {
    BulkSendHelper source ("ns3::TcpSocketFactory",
            InetSocketAddress (i4i5.GetAddress(1), port + i));
    source.SetAttribute ("MaxBytes", UintegerValue (10e6));
    ApplicationContainer sourceApps = source.Install (c.Get (0));
    sourceApps.Start (Seconds (0.0 + 0.001 * i));
    sourceApps.Stop (Seconds (1.0));
    }


    /*
    OnOffHelper sourceC ("ns3::UdpSocketFactory", InetSocketAddress (i4i6.GetAddress(1), 9999));
    sourceC.SetAttribute ("MaxBytes", UintegerValue(0));
    sourceC.SetAttribute ("DataRate", DataRateValue (DataRate (9e9)));
    sourceC.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    sourceC.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer sourceApps2 = sourceC.Install (c.Get (7));
    sourceApps2.Start (Seconds (0.0));
    sourceApps2.Stop (Seconds (1.0));
    */

    for (int i = 0; i < 6; i++)
    {
    PacketSinkHelper sink ("ns3::TcpSocketFactory",
            InetSocketAddress (i4i5.GetAddress(1), port + i));
    ApplicationContainer sinkApp = (sink.Install (c.Get(5)));

    sinkApp.Start (Seconds (0.0));
    sinkApp.Stop (Seconds (1.0));

    Ptr<PacketSink> pktSink = sinkApp.Get (0)->GetObject<PacketSink> ();
    std::stringstream oss;
    oss << "throughtput_" << i << ".txt";
    Simulator::ScheduleNow (&CheckThroughput, pktSink, oss.str ());
    }

    /*
    PacketSinkHelper sink2 ("ns3::UdpSocketFactory",
            InetSocketAddress (i4i6.GetAddress(1), 9999));
    ApplicationContainer sinkApp2 = sink2.Install (c.Get(6));
    sinkApp2.Start (Seconds (0.0));
    sinkApp2.Stop (Seconds (1.0));
    */

    // Gnuplot Settings
    cwndDataset.SetTitle("Cwnd");
    cwndDataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);

    cwnd2Dataset.SetTitle("Cwnd 2");
    cwnd2Dataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);

    queueDataset.SetTitle("Queue");
    queueDataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);

    queueDiscDataset.SetTitle("Queue Disc");
    queueDiscDataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);

    queue2Dataset.SetTitle("Queue 2");
    queue2Dataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);

    queueDisc2Dataset.SetTitle("Queue Disc 2");
    queueDisc2Dataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);


    queue3Dataset.SetTitle("Queue 3");
    queue3Dataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);

    queueDisc3Dataset.SetTitle("Queue Disc 3");
    queueDisc3Dataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);

    queue4Dataset.SetTitle("Queue 4");
    queue4Dataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);

    queueDisc4Dataset.SetTitle("Queue Disc 4");
    queueDisc4Dataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);

    throughputDataset.SetTitle("Throughput");
    throughputDataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);

    Simulator::Schedule (Seconds (0.00001), &TraceCwnd);
    Simulator::Schedule (Seconds (0.00001), &TraceCong);

    if (transportProt.compare ("DcTcp") == 0)
    {
        Ptr<QueueDisc> queueDisc = qd1d3.Get (0);
        Simulator::ScheduleNow (&CheckQueueDiscSize, queueDisc, queueDiscPlot, &queueDiscDataset);

        Ptr<QueueDisc> queueDisc3 = qd3d4.Get (0);
        Simulator::ScheduleNow (&CheckQueueDiscSize, queueDisc3, queueDiscPlot3, &queueDisc3Dataset);

        Ptr<QueueDisc> queueDisc2 = qd1d2.Get (0);
        Simulator::ScheduleNow (&CheckQueueDiscSize, queueDisc2, queueDiscPlot2, &queueDisc2Dataset);

        Ptr<QueueDisc> queueDisc4 = qd4d5.Get(0);
        Simulator::ScheduleNow (&CheckQueueDiscSize, queueDisc4, queueDiscPlot4, &queueDisc4Dataset);
    }

    Ptr<NetDevice> nd = d0d1.Get (0);
    Ptr<Queue> queue = DynamicCast<PointToPointNetDevice>(nd)->GetQueue ();
    Simulator::ScheduleNow (&CheckQueueSize, queue, queuePlot, &queueDataset);

    Ptr<NetDevice> nd3 = d3d4.Get (0);
    Ptr<Queue> queue3 = DynamicCast<PointToPointNetDevice>(nd3)->GetQueue ();
    Simulator::ScheduleNow (&CheckQueueSize, queue3, queuePlot3, &queue3Dataset);

    Ptr<NetDevice> nd2 = d1d2.Get (0);
    Ptr<Queue> queue2 = DynamicCast<PointToPointNetDevice>(nd2)->GetQueue ();
    Simulator::ScheduleNow (&CheckQueueSize, queue2, queuePlot2, &queue2Dataset);

    Ptr<NetDevice> nd4 = d4d5.Get (0);
    Ptr<Queue> queue4 = DynamicCast<PointToPointNetDevice>(nd4)->GetQueue ();
    Simulator::ScheduleNow (&CheckQueueSize, queue4, queuePlot4, &queue4Dataset);

    p2p.EnablePcapAll ("load-balance");

    NS_LOG_INFO ("Run Simulations");

    Simulator::Stop (Seconds (0.6));
    Simulator::Run ();
    Simulator::Destroy ();

    DoGnuPlot (transportProt);

    return 0;
}
