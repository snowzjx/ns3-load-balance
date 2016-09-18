#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/ipv4-drb-helper.h"
#include "ns3/congestion-probing-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/gnuplot.h"
#include "ns3/ipv4-conga-routing-helper.h"
#include "ns3/ipv4-static-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("examples2");

/* === Link Rate Check === */

/* Check link rate for d0d1 */
#define SPINE_LEAF_CAPACITY  10000000000          // 10Gbps
std::string d0d1Rate = "d0d1.out";
uint32_t d0d1TotalBytes = 0;
double lastd0d1CheckTime;

void Linkd0d1SendPacket (Ptr<const Packet> packet)
{
  //NS_LOG_UNCOND ("Sending packet: " << packet);

  double now = Simulator::Now().GetSeconds();

  d0d1TotalBytes += packet->GetSize ();

  if (now - lastd0d1CheckTime > 0.0001)
  {
    std::ofstream out (d0d1Rate.c_str (), std::ios::out|std::ios::app);
    out << Simulator::Now ().GetSeconds () << " " << d0d1TotalBytes * 8 / (now - lastd0d1CheckTime) << std::endl;
    lastd0d1CheckTime = now;
    d0d1TotalBytes = 0;
  }

}
/* ======================= */


double lastCwndCheckTime;

std::string cwndPlot = "cwnd.plotme";
std::string congPlot = "cong.plotme";
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
    //if (Simulator::Now().GetSeconds () - lastCwndCheckTime > 0.0001)
    //{
        //NS_LOG_UNCOND ("Cwnd: " << Simulator::Now ().GetSeconds () << "\t" << newCwnd);
        lastCwndCheckTime = Simulator::Now ().GetSeconds ();
        std::ofstream fCwndPlot (cwndPlot.c_str (), std::ios::out|std::ios::app);
        fCwndPlot << Simulator::Now ().GetSeconds () << " " << newCwnd << std::endl;

        cwndDataset.Add(Simulator::Now ().GetSeconds (), newCwnd);
    //}
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

  Simulator::Schedule (Seconds (0.0001), &CheckQueueSize, queue, plot, dataset);

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

void
CheckThroughput (Ptr<PacketSink> sink)
{
  uint32_t totalRecvBytes = sink->GetTotalRx ();
  uint32_t currentPeriodRecvBytes = totalRecvBytes - accumRecvBytes;

  accumRecvBytes = totalRecvBytes;

  Simulator::Schedule (Seconds (0.001), &CheckThroughput, sink);

  //NS_LOG_UNCOND ("Throughput: " << currentPeriodRecvBytes * 8 / 0.001 << "bps");

  std::ofstream fThroughputPlot (throughputPlot.c_str (), std::ios::out|std::ios::app);
  fThroughputPlot << Simulator::Now ().GetSeconds () << " " << currentPeriodRecvBytes * 8 / 0.001 <<std::endl;

  throughputDataset.Add (Simulator::Now().GetSeconds (), currentPeriodRecvBytes * 8 / 0.001);
}

uint32_t v0PacketTotal = 0;
uint32_t v0PacketMarked = 0;
uint32_t v1PacketTotal = 0;
uint32_t v1PacketMarked = 0;

void ProbingEvent (uint32_t v, Ptr<Packet> packet, Ipv4Header header, Time rtt, bool isCE)
{
    if (v == 0)
    {
        v0PacketTotal ++;
        if (isCE)
        {
            v0PacketMarked ++;
        }
    }
    else
    {
        v1PacketTotal ++;
        if (isCE)
        {
            v1PacketMarked ++;
        }
    }
}

int main (int argc, char *argv[])
{
#if 1
    LogComponentEnable ("examples2", LOG_LEVEL_INFO);
#endif

    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue(1400));
    Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (0));
    Config::SetDefault ("ns3::TcpSocket::ConnTimeout", TimeValue (MilliSeconds (5)));
    Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (10));
    Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (16000000));
    Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (16000000));
    Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue (MilliSeconds (5)));
    Config::SetDefault ("ns3::TcpSocketBase::ClockGranularity", TimeValue (MicroSeconds (100)));
    Config::SetDefault ("ns3::RttEstimator::InitialEstimation", TimeValue (MicroSeconds (80)));
    //Config::SetDefault ("ns3::TcpSocketBase::ReTxThreshold", UintegerValue (1000));


    std::string transportProt = "DcTcp";
    uint32_t drbCount1 = 0;
    uint32_t drbCount2 = 0;
    bool enableResequenceBuffer = false;
    bool enableFlowBender = false;
    double flowBenderT = 0.05;
    uint32_t flowBenderN = 1;
	bool enableConga = true;


    CommandLine cmd;
    cmd.AddValue ("transportProt", "Transport protocol to use: Tcp, DcTcp", transportProt);
    cmd.AddValue ("drbCount1", "DRB count for path 1, 0 means disable, 1 means DRB, n means Presto with n packets", drbCount1);
    cmd.AddValue ("drbCount2", "DRB count for path 1, 0 means disable, 1 means DRB, n means Presto with n packets", drbCount2);
    cmd.AddValue ("resequenceBuffer", "Enable resequence buffer", enableResequenceBuffer);
    cmd.AddValue ("flowBender", "Enable flow bender", enableFlowBender);
    cmd.AddValue ("flowBenderT", "T in the flow bender", flowBenderT);
    cmd.AddValue ("flowBenderN", "N in the flow bender", flowBenderN);
    cmd.AddValue ("Conga", "Enable conga", enableConga);

    cmd.Parse (argc, argv);

    if (transportProt.compare ("Tcp") == 0)
    {
        Config::SetDefault ("ns3::TcpSocketBase::ECN", BooleanValue (false));
    }
    else if (transportProt.compare ("DcTcp") == 0)
    {
        NS_LOG_INFO ("Enabling DcTcp");
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpDCTCP::GetTypeId ()));
    }
    else
    {
        return 0;
    }

    if (enableResequenceBuffer)
    {
        NS_LOG_INFO ("Enabling Resequence Buffer");
	    Config::SetDefault ("ns3::TcpSocketBase::ResequenceBuffer", BooleanValue (true));
        Config::SetDefault ("ns3::TcpResequenceBuffer::InOrderQueueTimerLimit", TimeValue (MicroSeconds (15)));
        Config::SetDefault ("ns3::TcpResequenceBuffer::SizeLimit", UintegerValue (100));
        Config::SetDefault ("ns3::TcpResequenceBuffer::OutOrderQueueTimerLimit", TimeValue (MicroSeconds (500)));

    }


    Config::SetDefault ("ns3::Ipv4GlobalRouting::PerflowEcmpRouting", BooleanValue (true));

    if (enableFlowBender)
    {
        NS_LOG_INFO ("Enabling Flow Bender");
        Config::SetDefault ("ns3::TcpSocketBase::FlowBender", BooleanValue (true));
        Config::SetDefault ("ns3::TcpFlowBender::T", DoubleValue (flowBenderT));
        Config::SetDefault ("ns3::TcpFlowBender::N", UintegerValue (flowBenderN));
    }

    NS_LOG_INFO ("Create nodes.");
    NodeContainer c;
    c.Create (6);

    NodeContainer n0n1 = NodeContainer (c.Get (0), c.Get(1));
    NodeContainer n1n2 = NodeContainer (c.Get (1), c.Get(2));
    NodeContainer n1n3 = NodeContainer (c.Get (1), c.Get(3));
    NodeContainer n2n4 = NodeContainer (c.Get (2), c.Get(4));
    NodeContainer n3n4 = NodeContainer (c.Get (3), c.Get(4));
    NodeContainer n4n5 = NodeContainer (c.Get (4), c.Get(5));

    InternetStackHelper internet;
    Ipv4StaticRoutingHelper staticRoutingHelper;
    Ipv4CongaRoutingHelper congaRoutingHelper;

    NS_LOG_INFO ("Install Internet stack");
	if (enableConga)
	{


	internet.SetRoutingHelper (staticRoutingHelper);
    internet.Install (c.Get(0));
    internet.Install (c.Get(5));

    internet.SetRoutingHelper (congaRoutingHelper);
    internet.Install (c.Get(1));
    internet.Install (c.Get(2));
	internet.Install (c.Get(3));
	internet.Install (c.Get(4));



	}
	else
	{

    internet.Install (c.Get (0));
    internet.Install (c.Get (2));
    internet.Install (c.Get (3));
    internet.Install (c.Get (5));
    //internet.Install (c.Get (4));
       if (drbCount1 != 0 || drbCount2 != 0) {
           internet.SetDrb (true);
       }
    internet.Install (c.Get (1));
    internet.Install (c.Get (4));
    }


    NS_LOG_INFO ("Install channel");
    PointToPointHelper p2p;

    Config::SetDefault ("ns3::RedQueueDisc::Mode", StringValue ("QUEUE_MODE_BYTES"));
    Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (1400));
    Config::SetDefault ("ns3::RedQueueDisc::QueueLimit", UintegerValue (550 * 1400));
    Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (false));

    TrafficControlHelper tc;
    tc.SetRootQueueDisc ("ns3::RedQueueDisc", "MinTh", DoubleValue (65 * 1400),
                                              "MaxTh", DoubleValue (65 * 1400));

    if (transportProt.compare ("Tcp") == 0)
    {
        p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (250));
    }
    else
    {
        p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (10));
    }

    p2p.SetDeviceAttribute ("DataRate", StringValue ("100Gbps")); // link rate of 0-1
    p2p.SetChannelAttribute ("Delay", TimeValue (MicroSeconds(10)));


    remove (d0d1Rate.c_str ());

    NetDeviceContainer d0d1 = p2p.Install (n0n1);

    Ptr<PointToPointNetDevice> pD0 = DynamicCast<PointToPointNetDevice>(d0d1.Get(0));
    pD0->TraceConnectWithoutContext("PhyTxBegin", MakeCallback(&Linkd0d1SendPacket));

    QueueDiscContainer qd0d1 = tc.Install (d0d1);

    NetDeviceContainer d1d2 = p2p.Install (n1n2);
    QueueDiscContainer qd1d2 = tc.Install (d1d2);

    NetDeviceContainer d1d3 = p2p.Install (n1n3);
    QueueDiscContainer qd1d3 = tc.Install (d1d3);

    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Gbps")); // link rate of 1-2 2-4


    NetDeviceContainer d2d4 = p2p.Install (n2n4);
    QueueDiscContainer qd2d4 = tc.Install (d2d4);


    //Config::SetDefault ("ns3::RedQueueDisc::QueueLimit", UintegerValue (100));
    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));                  // link rate of 1-3 3-4
    //p2p.SetChannelAttribute ("Delay", TimeValue (MicroSeconds(100)));

    TrafficControlHelper tc2;
    tc2.SetRootQueueDisc ("ns3::RedQueueDisc", "MinTh", DoubleValue (65 * 1400),
                                               "MaxTh", DoubleValue (65 * 1400));

    Config::SetDefault ("ns3::RedQueueDisc::DropRate", DoubleValue (0.005));

    NetDeviceContainer d3d4 = p2p.Install (n3n4);
    QueueDiscContainer qd3d4 = tc2.Install (d3d4);

    Config::SetDefault ("ns3::RedQueueDisc::DropRate", DoubleValue (0.0));

    p2p.SetDeviceAttribute ("DataRate", StringValue ("100Gbps"));  // link rate of 4-5

    NetDeviceContainer d4d5 = p2p.Install (n4n5);
    QueueDiscContainer qd4d5 = tc.Install (d4d5);

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
    if (transportProt.compare ("Tcp") == 0)
    {
        TrafficControlHelper tc;
        tc.Uninstall(d0d1);
        tc.Uninstall(d1d2);
        tc.Uninstall(d1d3);
        tc.Uninstall(d2d4);
        tc.Uninstall(d3d4);
        tc.Uninstall(d4d5);
    }

    if (drbCount1 != 0 || drbCount2 != 0) {
        NS_LOG_INFO ("Enable " << drbCount1 << ":" << drbCount2 <<" DRB");
        Ipv4DrbHelper drb;
        Ptr<Ipv4> ip1 = c.Get(1)->GetObject<Ipv4>();
        Ptr<Ipv4Drb> ipv4Drb1 = drb.GetIpv4Drb(ip1);
        ipv4Drb1->AddCoreSwitchAddress(drbCount1, i1i2.GetAddress (1));
        ipv4Drb1->AddCoreSwitchAddress(drbCount2, i1i3.GetAddress (1));

        Ptr<Ipv4> ip4 = c.Get(4)->GetObject<Ipv4>();
        Ptr<Ipv4Drb> ipv4Drb4 = drb.GetIpv4Drb(ip4);
        ipv4Drb4->AddCoreSwitchAddress(drbCount1, i2i4.GetAddress (0));
        ipv4Drb4->AddCoreSwitchAddress(drbCount2, i3i4.GetAddress (0));
    }

    NS_LOG_INFO ("Setting up routing table");

   // Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    NS_LOG_INFO ("Install TCP based application");

    uint16_t port = 8080;

	   //conga routing configure
	if (enableConga)
	{




	 // configure endhosts
	  staticRoutingHelper.GetStaticRouting (c.Get (0)->GetObject<Ipv4> ())->
			        AddNetworkRouteTo (Ipv4Address ("0.0.0.0"),
					                Ipv4Mask ("0.0.0.0"),
                                    d0d1.Get (0)->GetIfIndex ());


	staticRoutingHelper.GetStaticRouting (c.Get (5)->GetObject<Ipv4> ())->
			        AddNetworkRouteTo (Ipv4Address ("0.0.0.0"),
					                Ipv4Mask ("0.0.0.0"),
                                    d4d5.Get (1)->GetIfIndex ());

		    // Conga leaf switches forward the packet to the correct servers
            congaRoutingHelper.GetCongaRouting (c.Get (1)->GetObject<Ipv4> ())->
			        AddRoute (i4i5.GetAddress (1),
                            Ipv4Mask("255.255.255.255"),
                            d1d2.Get (0)->GetIfIndex ());

		    congaRoutingHelper.GetCongaRouting (c.Get (1)->GetObject<Ipv4> ())->
			        AddRoute (i4i5.GetAddress (1),
                            Ipv4Mask("255.255.255.255"),
                            d1d3.Get (0)->GetIfIndex ());

            congaRoutingHelper.GetCongaRouting (c.Get (1)->GetObject<Ipv4> ())->
			        AddRoute (i0i1.GetAddress (0),
                            Ipv4Mask("255.255.255.255"),
                            d0d1.Get (1)->GetIfIndex ());


            congaRoutingHelper.GetCongaRouting (c.Get (4)->GetObject<Ipv4> ())->
			        AddRoute (i0i1.GetAddress (0),
                            Ipv4Mask("255.255.255.255"),
                            d2d4.Get (1)->GetIfIndex ());

		    congaRoutingHelper.GetCongaRouting (c.Get (4)->GetObject<Ipv4> ())->
			        AddRoute (i0i1.GetAddress (0),
                            Ipv4Mask("255.255.255.255"),
                            d3d4.Get (1)->GetIfIndex ());

            congaRoutingHelper.GetCongaRouting (c.Get (4)->GetObject<Ipv4> ())->
			        AddRoute (i4i5.GetAddress (1),
                            Ipv4Mask("255.255.255.255"),
                            d4d5.Get (0)->GetIfIndex ());

            congaRoutingHelper.GetCongaRouting (c.Get (2)->GetObject<Ipv4> ())->
			        AddRoute (i0i1.GetAddress (0),
                            Ipv4Mask("255.255.255.255"),
                            d1d2.Get (1)->GetIfIndex ());

            congaRoutingHelper.GetCongaRouting (c.Get (2)->GetObject<Ipv4> ())->
			        AddRoute (i4i5.GetAddress (1),
                            Ipv4Mask("255.255.255.255"),
                            d2d4.Get (0)->GetIfIndex ());

          congaRoutingHelper.GetCongaRouting (c.Get (3)->GetObject<Ipv4> ())->
			        AddRoute (i0i1.GetAddress (0),
                            Ipv4Mask("255.255.255.255"),
                            d1d3.Get (1)->GetIfIndex ());

            congaRoutingHelper.GetCongaRouting (c.Get (3)->GetObject<Ipv4> ())->
			        AddRoute (i4i5.GetAddress (1),
                            Ipv4Mask("255.255.255.255"),
                            d3d4.Get (0)->GetIfIndex ());




			// configur switch parameter
	    Ptr<Ipv4CongaRouting> congaLeaf = congaRoutingHelper.GetCongaRouting (c.Get (1)->GetObject<Ipv4> ());
                congaLeaf->SetLeafId (1);
	    congaLeaf->SetTDre (MicroSeconds (100));
	    congaLeaf->SetAlpha (0.2);
	    congaLeaf->SetLinkCapacity(DataRate(SPINE_LEAF_CAPACITY));
	    congaLeaf->SetFlowletTimeout (MicroSeconds (200));

	    Ptr<Ipv4CongaRouting> congaLeaf2 = congaRoutingHelper.GetCongaRouting (c.Get (4)->GetObject<Ipv4> ());
                congaLeaf2->SetLeafId (2);
	    congaLeaf2->SetTDre (MicroSeconds (100));
	    congaLeaf2->SetAlpha (0.2);
	    congaLeaf2->SetLinkCapacity(DataRate(SPINE_LEAF_CAPACITY));
	    congaLeaf2->SetFlowletTimeout (MicroSeconds (200));

		Ptr<Ipv4CongaRouting> congaSpine = congaRoutingHelper.GetCongaRouting (c.Get (2)->GetObject<Ipv4> ());
		    congaSpine->SetTDre (MicroSeconds (100));
		    congaSpine->SetAlpha (0.2);
		    congaSpine->SetLinkCapacity(DataRate(SPINE_LEAF_CAPACITY));


        Ptr<Ipv4CongaRouting> congaSpine2 = congaRoutingHelper.GetCongaRouting (c.Get (3)->GetObject<Ipv4> ());
		    congaSpine2->SetTDre (MicroSeconds (100));
		    congaSpine2->SetAlpha (0.2);
		    congaSpine2->SetLinkCapacity(DataRate(SPINE_LEAF_CAPACITY));



            // ???
                congaRoutingHelper.GetCongaRouting (c.Get (1)->GetObject<Ipv4> ())->
			    AddAddressToLeafIdMap (i0i1.GetAddress (0), 1);

				congaRoutingHelper.GetCongaRouting (c.Get (1)->GetObject<Ipv4> ())->
			    AddAddressToLeafIdMap (i4i5.GetAddress (1), 2);

	            congaRoutingHelper.GetCongaRouting (c.Get (4)->GetObject<Ipv4> ())->
			    AddAddressToLeafIdMap (i4i5.GetAddress (1), 2);

	            congaRoutingHelper.GetCongaRouting (c.Get (4)->GetObject<Ipv4> ())->
				 AddAddressToLeafIdMap (i0i1.GetAddress (0), 1);

	}

    for (int i = 0; i < 6; i++)
    {
    BulkSendHelper source ("ns3::TcpSocketFactory",
            InetSocketAddress (i4i5.GetAddress(1), port + i));
    source.SetAttribute ("MaxBytes", UintegerValue (10e6));
    ApplicationContainer sourceApps = source.Install (c.Get (0));
    sourceApps.Start (Seconds (0.0 + 0.001 * i));
    sourceApps.Stop (Seconds (1.5));
    }

    for (int i = 0; i < 6; i++)
    {
    PacketSinkHelper sink ("ns3::TcpSocketFactory",
            InetSocketAddress (i4i5.GetAddress(1), port + i));
    ApplicationContainer sinkApp = (sink.Install (c.Get(5)));

    sinkApp.Start (Seconds (0.0));
    sinkApp.Stop (Seconds (1.5));

    Ptr<PacketSink> pktSink = sinkApp.Get (0)->GetObject<PacketSink> ();
    std::stringstream oss;
    oss << "throughtput_" << i << ".txt";
    Simulator::ScheduleNow (&CheckThroughput, pktSink, oss.str ());
    }


        // Gnuplot Settings
    cwndDataset.SetTitle("Cwnd");
    cwndDataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);

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
        Ptr<QueueDisc> queueDisc = qd0d1.Get (0);
        Simulator::ScheduleNow (&CheckQueueDiscSize, queueDisc, queueDiscPlot, &queueDiscDataset);

        Ptr<QueueDisc> queueDisc3 = qd3d4.Get (0);
        Simulator::ScheduleNow (&CheckQueueDiscSize, queueDisc3, queueDiscPlot3, &queueDisc3Dataset);

        Ptr<QueueDisc> queueDisc2 = qd2d4.Get (0);
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

    Ptr<NetDevice> nd2 = d2d4.Get (0);
    Ptr<Queue> queue2 = DynamicCast<PointToPointNetDevice>(nd2)->GetQueue ();
    Simulator::ScheduleNow (&CheckQueueSize, queue2, queuePlot2, &queue2Dataset);

    Ptr<NetDevice> nd4 = d4d5.Get (0);
    Ptr<Queue> queue4 = DynamicCast<PointToPointNetDevice>(nd4)->GetQueue ();
    Simulator::ScheduleNow (&CheckQueueSize, queue4, queuePlot4, &queue4Dataset);

    //Ptr<PacketSink> pktSink = sinkApp.Get (0)->GetObject<PacketSink> ();
    //Simulator::ScheduleNow (&CheckThroughput, pktSink);


    p2p.EnablePcapAll ("load-balance");

    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    NS_LOG_INFO ("Run Simulations");

    Simulator::Stop (Seconds (0.2));
    Simulator::Run ();

    flowMonitor->SerializeToXmlFile("flow-monitor.xml", true, true);

    Simulator::Destroy ();

    NS_LOG_INFO ("Probing in V0: " << static_cast<double>(v0PacketMarked) / v0PacketTotal);
    NS_LOG_INFO ("Probing in V1: " << static_cast<double>(v1PacketMarked) / v1PacketTotal);

    DoGnuPlot (transportProt);

    return 0;
}
