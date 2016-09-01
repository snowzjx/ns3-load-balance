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

// There are 1 server connecting to each leaf switch
#define SERVER_COUNT 3
#define SPINE_COUNT 2
#define LEAF_COUNT 4

#define SPINE_LEAF_CAPACITY  10000000000          // 10Gbps
#define LEAF_SERVER_CAPACITY 10000000000          // 10Gbps
#define LINK_LATENCY MicroSeconds(10)             // 10 MicroSeconds
#define BUFFER_SIZE 250                           // 250 Packets

#define RED_QUEUE_MARKING 65 			  // 65 Packets (available only in DcTcp)

// The simulation starting and ending time
#define START_TIME 0.0
#define END_TIME 3.5

// The flow port range, each flow will be assigned a random port number within this range
#define FLOW_A_PORT 5000
#define FLOW_B_PORT 6000
#define FLOW_C_PORT 7000

// Adopted from the simulation from WANG PENG
// Acknowledged to https://williamcityu@bitbucket.org/williamcityu/2016-socc-simulation.git
#define PACKET_SIZE 1400

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("HiddenTerminal");

double lastCwndCheckTime;

std::string cwndPlot = "cwnd.plotme";

Gnuplot2dDataset cwndDataset;

static void
CwndChange (std::string msg, uint32_t oldCwnd, uint32_t newCwnd)
{
    //NS_LOG_INFO (msg);
    if (Simulator::Now().GetSeconds () - lastCwndCheckTime > 0.0001)
    {
        NS_LOG_UNCOND ("Cwnd: " << Simulator::Now ().GetSeconds () << "\t" << newCwnd);
        lastCwndCheckTime = Simulator::Now ().GetSeconds ();
        std::ofstream fCwndPlot (cwndPlot.c_str (), std::ios::out|std::ios::app);
        fCwndPlot << Simulator::Now ().GetSeconds () << " " << newCwnd << std::endl;

        cwndDataset.Add(Simulator::Now ().GetSeconds (), newCwnd);
    }
}

static void
TraceCwnd ()
{
    std::stringstream ss;
    ss << "/NodeList/11/$ns3::TcpL4Protocol/SocketList/*/CongestionWindow";
    Config::Connect (ss.str (), MakeCallback (&CwndChange));
}

void
DoGnuPlot ()
{
    Gnuplot cwndGnuPlot ("cwnd.png");
    cwndGnuPlot.SetTitle("Cwnd");
    cwndGnuPlot.SetTerminal("png");
    cwndGnuPlot.AddDataset (cwndDataset);
    std::ofstream cwndPlotFile ("cwnd.plt");
    cwndGnuPlot.GenerateOutput (cwndPlotFile);
    cwndPlotFile.close();
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
  oss << stat.packetsInQueueDisc;
  return oss.str ();
}

int main (int argc, char *argv[])
{
#if 1
    LogComponentEnable ("HiddenTerminal", LOG_LEVEL_INFO);
#endif

    // Command line parameters parsing
    std::string transportProt = "Tcp";

    CommandLine cmd;
    cmd.AddValue ("transportProt", "Transport protocol to use: Tcp, DcTcp", transportProt);
    cmd.Parse (argc, argv);

    if (transportProt.compare ("DcTcp") == 0)
    {
	    NS_LOG_INFO ("Enabling DcTcp");
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpDCTCP::GetTypeId ()));
    	Config::SetDefault ("ns3::RedQueueDisc::Mode", StringValue ("QUEUE_MODE_PACKETS"));
    	Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (PACKET_SIZE));
    	Config::SetDefault ("ns3::RedQueueDisc::QueueLimit", UintegerValue (BUFFER_SIZE));
    	Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (false));
    }

    NS_LOG_INFO ("Config parameters");
    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue(1400));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue (0));
    Config::SetDefault("ns3::TcpSocketBase::MinRto", TimeValue(MicroSeconds(200)));
    Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (160000));
    Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (160000));

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
	    Ptr<Ipv4CongaRouting> congaLeaf = congaRoutingHelper.GetCongaRouting (leaves.Get (i)->GetObject<Ipv4> ());
                congaLeaf->SetLeafId (i);
	    congaLeaf->SetTDre (MicroSeconds (30));
	    congaLeaf->SetAlpha (0.2);
	    congaLeaf->SetLinkCapacity(DataRate(SPINE_LEAF_CAPACITY));
	    congaLeaf->SetFlowletTimeout (MicroSeconds (200));

	    ipv4.NewNetwork ();
        for (int j = 0; j < SPINE_COUNT; j++)
        {
            NodeContainer nodeContainer = NodeContainer (leaves.Get (i), spines.Get (j));
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

            NS_LOG_INFO ("Leaf " << i << " is connected to spine " << j
                    << "(" << netDeviceContainer.Get (0)->GetIfIndex ()
                    << "<->" << netDeviceContainer.Get(1)->GetIfIndex () << ")");

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
		    congaSpine->SetTDre (MicroSeconds (30));
		    congaSpine->SetAlpha (0.2);
		    congaSpine->SetLinkCapacity(DataRate(SPINE_LEAF_CAPACITY));
		    congaSpine->AddRoute (leafNetworks[i],
				            Ipv4Mask("255.255.255.0"),
                            netDeviceContainer.Get (1)->GetIfIndex ());
	    }
    }


    NS_LOG_INFO ("Create applications");

    NS_LOG_INFO ("Creating Flow A");
    Ptr<Node> destServer9 = servers.Get (9);
    Ptr<Ipv4> ipv4Object9 = destServer9->GetObject<Ipv4> ();
    Ipv4InterfaceAddress destInterface9 = ipv4Object9->GetAddress (1, 0);
    Ipv4Address destAddress9 = destInterface9.GetLocal ();

    /*OnOffHelper sourceA ("ns3::TcpSocketFactory", InetSocketAddress (destAddress9, FLOW_A_PORT));*/
    BulkSendHelper sourceA ("ns3::TcpSocketFactory", InetSocketAddress (destAddress9, FLOW_A_PORT));
    /*sourceA.SetAttribute ("PacketSize", UintegerValue (PACKET_SIZE));*/
    sourceA.SetAttribute ("SendSize", UintegerValue (PACKET_SIZE));
    sourceA.SetAttribute ("MaxBytes", UintegerValue(0));
    /*sourceA.SetAttribute ("DataRate", DataRateValue (DataRate (LEAF_SERVER_CAPACITY)));*/
    /*sourceA.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));*/
    /*sourceA.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));*/

    ApplicationContainer sourceAppA = sourceA.Install (servers.Get (0));
    sourceAppA.Start (Seconds (START_TIME + 0.01));
    sourceAppA.Stop (Seconds (END_TIME));

    PacketSinkHelper sinkA ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), FLOW_A_PORT));
    ApplicationContainer sinkAppA = sinkA.Install (servers.Get (9));
    sinkAppA.Start (Seconds (START_TIME));
    sinkAppA.Stop (Seconds (END_TIME));

    NS_LOG_INFO ("Creating Flow B");
    Ptr<Node> destServer10 = servers.Get (10);
    Ptr<Ipv4> ipv4Object10 = destServer10->GetObject<Ipv4> ();
    Ipv4InterfaceAddress destInterface10 = ipv4Object10->GetAddress (1, 0);
    Ipv4Address destAddress10 = destInterface10.GetLocal ();

    /*OnOffHelper sourceB ("ns3::TcpSocketFactory", InetSocketAddress (destAddress10, FLOW_B_PORT));*/
    BulkSendHelper sourceB ("ns3::TcpSocketFactory", InetSocketAddress (destAddress10, FLOW_B_PORT));
    /*sourceB.SetAttribute ("PacketSize", UintegerValue (PACKET_SIZE));*/
    sourceA.SetAttribute ("SendSize", UintegerValue (PACKET_SIZE));
    sourceB.SetAttribute ("MaxBytes", UintegerValue(0));
    /*sourceB.SetAttribute ("DataRate", DataRateValue (DataRate (LEAF_SERVER_CAPACITY)));*/
    /*sourceB.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));*/
    /*sourceB.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));*/

    ApplicationContainer sourceAppB = sourceB.Install (servers.Get (3));
    sourceAppB.Start (Seconds (START_TIME));
    sourceAppB.Stop (Seconds (END_TIME));

    PacketSinkHelper sinkB ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), FLOW_B_PORT));
    ApplicationContainer sinkAppB = sinkB.Install (servers.Get (10));
    sinkAppB.Start (Seconds (START_TIME + 0.02));
    sinkAppB.Stop (Seconds (END_TIME));

    NS_LOG_INFO ("Creating Flow C");
    Ptr<Node> destServer11 = servers.Get (11);
    Ptr<Ipv4> ipv4Object11 = destServer11->GetObject<Ipv4> ();
    Ipv4InterfaceAddress destInterface11 = ipv4Object11->GetAddress (1, 0);
    Ipv4Address destAddress11 = destInterface11.GetLocal ();

    OnOffHelper sourceC ("ns3::TcpSocketFactory", InetSocketAddress (destAddress11, FLOW_C_PORT));
    sourceC.SetAttribute ("PacketSize", UintegerValue (PACKET_SIZE));
    sourceC.SetAttribute ("MaxBytes", UintegerValue(0));
    sourceC.SetAttribute ("DataRate", DataRateValue (DataRate (LEAF_SERVER_CAPACITY)));
    sourceC.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.5]"));
    sourceC.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.005]"));

    ApplicationContainer sourceAppC = sourceC.Install (servers.Get (6));
    sourceAppC.Start (Seconds (START_TIME));
    sourceAppC.Stop (Seconds (END_TIME));

    PacketSinkHelper sinkC ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), FLOW_C_PORT));
    ApplicationContainer sinkAppC = sinkC.Install (servers.Get (11));
    sinkAppC.Start (Seconds (START_TIME));
    sinkAppC.Stop (Seconds (END_TIME));

    congaRoutingHelper.GetCongaRouting (leaves.Get (0)->GetObject<Ipv4> ())->InitCongestion (3, 4, 8);
    congaRoutingHelper.GetCongaRouting (leaves.Get (1)->GetObject<Ipv4> ())->InitCongestion (3, 4, 8);

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

    NS_LOG_INFO ("Enable Cwnd tracing");

    remove (cwndPlot.c_str ());

    cwndDataset.SetTitle("Cwnd");
    cwndDataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);

    Simulator::Schedule (Seconds (0.001), &TraceCwnd);

    NS_LOG_INFO ("Start simulation");
    Simulator::Stop (Seconds (END_TIME));
    Simulator::Run ();

    flowMonitor->CheckForLostPackets ();

    std::stringstream flowMonitorFilename;
    std::stringstream linkMonitorFilename;

    flowMonitorFilename << "hidden-terminal-flow-monitor.xml";
    linkMonitorFilename << "hidden-terminal-link-monitor.out";

    flowMonitor->SerializeToXmlFile(flowMonitorFilename.str (), true, true);
    linkMonitor->OutputToFile (linkMonitorFilename.str (), &DefaultFormat);

    Simulator::Destroy ();
    NS_LOG_INFO ("Stop simulation");

    DoGnuPlot ();

    return 0;
}
