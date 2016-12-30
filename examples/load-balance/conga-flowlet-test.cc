#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-conga-routing-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/link-monitor-module.h"
#include "ns3/traffic-control-module.h"

#include "ns3/gnuplot.h"

#include <vector>
#include <utility>
#include <string>

// There are 1 server connecting to each leaf switch
#define SERVER_COUNT 1
#define SPINE_COUNT 2
#define LEAF_COUNT 2

#define SPINE_LEAF_CAPACITY  10000000000          // 10Gbps
#define LEAF_SERVER_CAPACITY 100000000000         // 100Gbps
#define LINK_LATENCY MicroSeconds(20)             // 10 MicroSeconds
#define BUFFER_SIZE 600                           // 600 Packets

#define RED_QUEUE_MARKING 65 			  // 65 Packets (available only in DcTcp)

// The simulation starting and ending time
#define START_TIME 0.0
#define END_TIME 0.05

// The flow port range, each flow will be assigned a random port number within this range
#define FLOW_A_PORT 5000
#define FLOW_B_PORT 6000

// Adopted from the simulation from WANG PENG
// Acknowledged to https://williamcityu@bitbucket.org/williamcityu/2016-socc-simulation.git
#define PACKET_SIZE 1400

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CongaFlowletTest");

double lastCwndCheckTime;

Gnuplot2dDataset throughputDatasetA;
Gnuplot2dDataset throughputDatasetB;

uint64_t flowARecvBytes = 0;
uint64_t flowBRecvBytes = 0;

std::string
ToString (uint32_t value)
{
  std::stringstream ss;
  ss << value;
  return ss.str();
}

std::string
StringCombine (std::string A, std::string B, std::string C)
{
  std::stringstream ss;
  ss << A << B << C;
  return ss.str();
}

void
CheckFlowAThroughput (Ptr<PacketSink> sink)
{
  uint32_t totalRecvBytes = sink->GetTotalRx ();
  uint32_t currentPeriodRecvBytes = totalRecvBytes - flowARecvBytes;

  flowARecvBytes = totalRecvBytes;

  Simulator::Schedule (Seconds (0.0001), &CheckFlowAThroughput, sink);

  NS_LOG_UNCOND ("Throughput of Flow A: " << currentPeriodRecvBytes * 8 / 0.0001 << "bps");

  throughputDatasetA.Add (Simulator::Now().GetSeconds (), currentPeriodRecvBytes * 8 / 0.0001);
}

void
CheckFlowBThroughput (Ptr<PacketSink> sink)
{
  uint32_t totalRecvBytes = sink->GetTotalRx ();
  uint32_t currentPeriodRecvBytes = totalRecvBytes - flowBRecvBytes;

  flowBRecvBytes = totalRecvBytes;

  Simulator::Schedule (Seconds (0.0001), &CheckFlowBThroughput, sink);

  NS_LOG_UNCOND ("Throughput of Flow B: " << currentPeriodRecvBytes * 8 / 0.0001 << "bps");

  throughputDatasetB.Add (Simulator::Now().GetSeconds (), currentPeriodRecvBytes * 8 / 0.0001);
}

void
DoGnuPlot (uint32_t flowletTimeout)
{
    Gnuplot flowAThroughputPlot (StringCombine ("flow_A_", ToString (flowletTimeout), "_throughput.png"));
    flowAThroughputPlot.SetTitle ("Flow A Throughput");
    flowAThroughputPlot.SetTerminal ("png");
    flowAThroughputPlot.AddDataset (throughputDatasetA);
    std::ofstream flowAThroughputFile (StringCombine ("flow_A_", ToString (flowletTimeout), "_throughput.plt").c_str ());
    flowAThroughputPlot.GenerateOutput (flowAThroughputFile);
    flowAThroughputFile.close ();

    Gnuplot flowBThroughputPlot (StringCombine ("flow_B_", ToString (flowletTimeout), "_throughput.png"));
    flowBThroughputPlot.SetTitle ("Flow B Throughput");
    flowBThroughputPlot.SetTerminal ("png");
    flowBThroughputPlot.AddDataset (throughputDatasetB);
    std::ofstream flowBThroughputFile (StringCombine ("flow_B_", ToString (flowletTimeout), "_throughput.plt").c_str ());
    flowBThroughputPlot.GenerateOutput (flowBThroughputFile);
    flowBThroughputFile.close ();
}

std::string
DefaultFormat (struct LinkProbe::LinkStats stat)
{
  std::ostringstream oss;
  //oss << stat.txLinkUtility << "/"
      //<< stat.packetsInQueue << "/"
      //<< stat.bytesInQueue << "/"
      //<< stat.packetsInQueueDisc << "/"
      //<< stat.bytesInQueueDisc;
  /*oss << stat.packetsInQueue;*/
  oss << stat.txLinkUtility << ",";
  oss << stat.packetsInQueueDisc;
  return oss.str ();
}

int main (int argc, char *argv[])
{
#if 1
    LogComponentEnable ("CongaFlowletTest", LOG_LEVEL_INFO);
#endif

    // Command line parameters parsing
    std::string transportProt = "Tcp";
    uint32_t flowletTimeout = 50;

    CommandLine cmd;
    cmd.AddValue ("transportProt", "Transport protocol to use: Tcp, DcTcp", transportProt);
    cmd.AddValue ("flowletTimeout", "Conga flowlet timeout", flowletTimeout);
    cmd.Parse (argc, argv);

    if (transportProt.compare ("DcTcp") == 0)
    {
	    NS_LOG_INFO ("Enabling DcTcp");
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpDCTCP::GetTypeId ()));
        Config::SetDefault ("ns3::RedQueueDisc::Mode", StringValue ("QUEUE_MODE_BYTES"));
    	Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (PACKET_SIZE));
    	Config::SetDefault ("ns3::RedQueueDisc::QueueLimit", UintegerValue (BUFFER_SIZE * PACKET_SIZE));
    	Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (false));
    }

    NS_LOG_INFO ("Config parameters");
    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue(PACKET_SIZE));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue (0));
    Config::SetDefault ("ns3::TcpSocket::ConnTimeout", TimeValue (MilliSeconds (5)));
    Config::SetDefault ("ns3::TcpSocket::InitialSlowStartThreshold", UintegerValue (10 * PACKET_SIZE));
    Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (10));
    Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue(MilliSeconds(5)));
    Config::SetDefault ("ns3::TcpSocketBase::ClockGranularity", TimeValue (MicroSeconds (100)));
    Config::SetDefault ("ns3::RttEstimator::InitialEstimation", TimeValue (MicroSeconds (160)));
    Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (160000000));
    Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (160000000));

    NS_LOG_INFO ("Create nodes");
    NodeContainer spines;
    spines.Create (SPINE_COUNT);
    NodeContainer leaves;
    leaves.Create (LEAF_COUNT);
    NodeContainer servers;
    servers.Create (SERVER_COUNT * LEAF_COUNT);

    NS_LOG_INFO ("Install Internet stacks");
    InternetStackHelper internet;
    Ipv4StaticRoutingHelper staticRoutingHelper;
    Ipv4CongaRoutingHelper congaRoutingHelper;

	internet.SetRoutingHelper (staticRoutingHelper);
    internet.Install (servers);

    internet.SetRoutingHelper (congaRoutingHelper);
    internet.Install (spines);
    internet.Install (leaves);

    NS_LOG_INFO ("Install channels and assign addresses");

    PointToPointHelper p2p;
    Ipv4AddressHelper ipv4;

    TrafficControlHelper tc;
    tc.SetRootQueueDisc ("ns3::RedQueueDisc", "MinTh", DoubleValue (RED_QUEUE_MARKING),
                                              "MaxTh", DoubleValue (RED_QUEUE_MARKING));

    NS_LOG_INFO ("Configuring servers");
    // Setting servers
    p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (LEAF_SERVER_CAPACITY)));
    p2p.SetChannelAttribute ("Delay", TimeValue(LINK_LATENCY));
    if (transportProt.compare ("Tcp") == 0)
    {
     	p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (BUFFER_SIZE));
    }
    else
    {
	    p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (5));
    }

    ipv4.SetBase ("10.1.0.0", "255.255.255.0");

    std::vector<Ipv4Address> leafNetworks (LEAF_COUNT);

    for (int i = 0; i < LEAF_COUNT; i++)
    {
        Ipv4Address network = ipv4.NewNetwork ();
        leafNetworks[i] = network;

        for (int j = 0; j < SERVER_COUNT; j++)
        {
            int serverIndex = i * SERVER_COUNT + j;
            NS_LOG_INFO ("Server " << serverIndex << " is connected to leaf switch " << i);
            NodeContainer nodeContainer = NodeContainer (leaves.Get (i), servers.Get (serverIndex));
            NetDeviceContainer netDeviceContainer = p2p.Install (nodeContainer);
	        if (transportProt.compare ("DcTcp") == 0)
	        {
		        NS_LOG_INFO ("Install RED Queue for leaf: " << i << " and server: " << j);
	            tc.Install (netDeviceContainer);
            }
            Ipv4InterfaceContainer interfaceContainer = ipv4.Assign (netDeviceContainer);
	        if (transportProt.compare ("Tcp") == 0)
            {
                tc.Uninstall (netDeviceContainer);
            }

            // All servers just forward the packet to leaf switch
		    staticRoutingHelper.GetStaticRouting (servers.Get (serverIndex)->GetObject<Ipv4> ())->
			        AddNetworkRouteTo (Ipv4Address ("0.0.0.0"),
					                   Ipv4Mask ("0.0.0.0"),
                                       netDeviceContainer.Get (1)->GetIfIndex ());

		    // Conga leaf switches forward the packet to the correct servers
            congaRoutingHelper.GetCongaRouting (leaves.Get (i)->GetObject<Ipv4> ())->
			        AddRoute (interfaceContainer.GetAddress (1),
                              Ipv4Mask("255.255.255.255"),
                              netDeviceContainer.Get (0)->GetIfIndex ());

            for (int k = 0; k < LEAF_COUNT; k++)
	        {
                congaRoutingHelper.GetCongaRouting (leaves.Get (k)->GetObject<Ipv4> ())->
			    AddAddressToLeafIdMap (interfaceContainer.GetAddress (1), i);
	        }
        }
    }

    NS_LOG_INFO ("Configuring switches");
    // Setting switches
    p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (SPINE_LEAF_CAPACITY)));

    for (int i = 0; i < LEAF_COUNT; i++)
    {
	 	ipv4.NewNetwork ();
        for (int j = 0; j < SPINE_COUNT; j++)
        {
            uint64_t leafSpineCapacity = SPINE_LEAF_CAPACITY;
            if (j == 0)
            {
                leafSpineCapacity = SPINE_LEAF_CAPACITY / 2;
            }

            Ptr<Ipv4CongaRouting> congaLeaf = congaRoutingHelper.GetCongaRouting (leaves.Get (i)->GetObject<Ipv4> ());
            congaLeaf->SetLeafId (i);
	        congaLeaf->SetTDre (MicroSeconds (60));
	        congaLeaf->SetAlpha (0.2);
            congaLeaf->SetLinkCapacity (DataRate (leafSpineCapacity));
	        congaLeaf->SetFlowletTimeout (MicroSeconds (flowletTimeout));

            NodeContainer nodeContainer = NodeContainer (leaves.Get (i), spines.Get (j));
            p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (leafSpineCapacity)));
            NetDeviceContainer netDeviceContainer = p2p.Install (nodeContainer);
	        if (transportProt.compare ("DcTcp") == 0)
	        {
		        NS_LOG_INFO ("Install RED Queue for leaf: " << i << " and spine: " << j);
	            tc.Install (netDeviceContainer);
            }
 	        Ipv4InterfaceContainer ipv4InterfaceContainer = ipv4.Assign (netDeviceContainer);
	        if (transportProt.compare ("Tcp") == 0)
            {
                tc.Uninstall (netDeviceContainer);
            }

            NS_LOG_INFO ("Leaf " << i << " is connected to Spine " << j
                    << "(" << netDeviceContainer.Get (0)->GetIfIndex ()
                    << "<->" << netDeviceContainer.Get(1)->GetIfIndex () << ")"
                    << "at capacity: " << leafSpineCapacity);

		    // For each conga leaf switch, routing entry to route the packet to OTHER leaves should be added
            for (int k = 0; k < LEAF_COUNT; k++)
		    {
		        if (k != i)
		        {
			        congaRoutingHelper.GetCongaRouting (leaves.Get (i)->GetObject<Ipv4> ())->
				                        AddRoute (leafNetworks[k],
				  	                    Ipv4Mask("255.255.255.0"),
                                  	    netDeviceContainer.Get (0)->GetIfIndex ());
                }
            }

		    // For each conga spine switch, routing entry to THIS leaf switch should be added
		    Ptr<Ipv4CongaRouting> congaSpine = congaRoutingHelper.GetCongaRouting (spines.Get (j)->GetObject<Ipv4> ());
		    congaSpine->SetTDre (MicroSeconds (60));
		    congaSpine->SetAlpha (0.2);
            congaSpine->SetLinkCapacity(DataRate(leafSpineCapacity));
		    congaSpine->AddRoute (leafNetworks[i],
				            Ipv4Mask("255.255.255.0"),
                            netDeviceContainer.Get (1)->GetIfIndex ());
	    }
    }

    NS_LOG_INFO ("Initialize Conga Congestion");
    congaRoutingHelper.GetCongaRouting (leaves.Get (0)->GetObject<Ipv4> ())->InitCongestion (1, 2, 1);

    NS_LOG_INFO ("Create applications");
    NS_LOG_INFO ("Creating Flow A");
    Ptr<Node> destServer = servers.Get (1);
    Ptr<Ipv4> ipv4Object = destServer->GetObject<Ipv4> ();
    Ipv4InterfaceAddress destInterface = ipv4Object->GetAddress (1, 0);
    Ipv4Address destAddress = destInterface.GetLocal ();

    BulkSendHelper sourceA ("ns3::TcpSocketFactory", InetSocketAddress (destAddress, FLOW_A_PORT));
    sourceA.SetAttribute ("SendSize", UintegerValue (PACKET_SIZE));
    sourceA.SetAttribute ("MaxBytes", UintegerValue(0));

    ApplicationContainer sourceAppA = sourceA.Install (servers.Get (0));
    sourceAppA.Start (Seconds (START_TIME));
    sourceAppA.Stop (Seconds (END_TIME));

    PacketSinkHelper sinkA ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), FLOW_A_PORT));
    ApplicationContainer sinkAppA = sinkA.Install (servers.Get (1));
    sinkAppA.Start (Seconds (START_TIME));
    sinkAppA.Stop (Seconds (END_TIME));

    NS_LOG_INFO ("Creating Flow B");

    BulkSendHelper sourceB ("ns3::TcpSocketFactory", InetSocketAddress (destAddress, FLOW_B_PORT));
    sourceA.SetAttribute ("SendSize", UintegerValue (PACKET_SIZE));
    sourceB.SetAttribute ("MaxBytes", UintegerValue(0));

    ApplicationContainer sourceAppB = sourceB.Install (servers.Get (0));
    sourceAppB.Start (Seconds (START_TIME));
    sourceAppB.Stop (Seconds (END_TIME));

    PacketSinkHelper sinkB ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), FLOW_B_PORT));
    ApplicationContainer sinkAppB = sinkB.Install (servers.Get (1));
    sinkAppB.Start (Seconds (START_TIME));
    sinkAppB.Stop (Seconds (END_TIME));

    NS_LOG_INFO ("Enabling flow monitor");
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    NS_LOG_INFO ("Enabling link monitor");

    Ptr<LinkMonitor> linkMonitor = Create<LinkMonitor> ();
    for (int i = 0; i < SPINE_COUNT; i++)
    {
        std::stringstream name;
        name << "Spine " << i;
        Ptr<Ipv4LinkProbe> spineLinkProbe = Create<Ipv4LinkProbe> (spines.Get (i), linkMonitor);
        spineLinkProbe->SetProbeName (name.str ());
        spineLinkProbe->SetCheckTime (Seconds (0.001));
        spineLinkProbe->SetDataRateAll (DataRate (SPINE_LEAF_CAPACITY));
    }
    for (int i = 0; i < LEAF_COUNT; i++)
    {
        std::stringstream name;
        name << "Leaf " << i;
        Ptr<Ipv4LinkProbe> leafLinkProbe = Create<Ipv4LinkProbe> (leaves.Get (i), linkMonitor);
        leafLinkProbe->SetProbeName (name.str ());
        leafLinkProbe->SetCheckTime (Seconds (0.001));
        leafLinkProbe->SetDataRateAll (DataRate (SPINE_LEAF_CAPACITY));
    }

    linkMonitor->Start (Seconds (START_TIME));
    linkMonitor->Stop (Seconds (END_TIME));

    NS_LOG_INFO ("Enable Throughput Tracing");

    remove (StringCombine ("flow_A_", ToString (flowletTimeout), "_throughput.plt").c_str ());
    remove (StringCombine ("flow_B_", ToString (flowletTimeout), "_throughput.plt").c_str ());

    throughputDatasetA.SetTitle ("Throughput A");
    throughputDatasetA.SetStyle (Gnuplot2dDataset::LINES_POINTS);
    throughputDatasetB.SetTitle ("Throughput B");
    throughputDatasetB.SetStyle (Gnuplot2dDataset::LINES_POINTS);

    Simulator::ScheduleNow (&CheckFlowAThroughput, sinkAppA.Get (0)->GetObject<PacketSink> ());
    Simulator::ScheduleNow (&CheckFlowBThroughput, sinkAppB.Get (0)->GetObject<PacketSink> ());

    NS_LOG_INFO ("Start simulation");
    Simulator::Stop (Seconds (END_TIME));
    Simulator::Run ();

    flowMonitor->CheckForLostPackets ();

    std::stringstream flowMonitorFilename;
    std::stringstream linkMonitorFilename;

    flowMonitorFilename << "conga-flowlet-" << flowletTimeout << "-test-flow-monitor.xml";
    linkMonitorFilename << "conga-flowlet-" << flowletTimeout << "-test-link-monitor.out";

    flowMonitor->SerializeToXmlFile(flowMonitorFilename.str (), true, true);
    linkMonitor->OutputToFile (linkMonitorFilename.str (), &DefaultFormat);

    Simulator::Destroy ();
    NS_LOG_INFO ("Stop simulation");

    DoGnuPlot (flowletTimeout);

    return 0;
}
