#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-conga-routing-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/traffic-control-module.h"

#include <vector>
#include <utility>

// Server leaf spine configuration
#define SERVER_COUNT 3
#define SPINE_COUNT 3
#define LEAF_COUNT 3

#define SPINE_LEAF_CAPACITY  10000000000          // 10Gbps
#define LEAF_SERVER_CAPACITY 10000000000          // 10Gbps
#define LINK_LATENCY MicroSeconds(10)             // 25 MicroSeconds, according to 7-22.pdf
#define BUFFER_SIZE 500                           // 500 Packets

#define RED_QUEUE_MARKING 65 			  // 65 Packets (available only in DcTcp)

// The simulation starting and ending time
#define START_TIME 0.0
#define END_TIME 150.0

// The flow port range, each flow will be assigned a random port number within this range
#define PORT_START 10000
#define PORT_END 50000

// Adopted from the simulation from WANG PENG
// Acknowledged to https://williamcityu@bitbucket.org/williamcityu/2016-socc-simulation.git
#define PACKET_SIZE 1400

// Flow size
#define FLOW_SIZE 100000000                         // 100MB

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SparkShuffle");

enum RunMode {
    CONGA,
    CONGA_FLOW,
    CONGA_ECMP,
    ECMP
};

template<typename T>
T rand_range (T min, T max)
{
    return min + ((double)max - min) * rand () / RAND_MAX;
}

void install_applications (int fromLeafId, int toLeafId, NodeContainer servers, int &flowCount)
{
    int fromStartServerIndex = fromLeafId * SERVER_COUNT;
    int fromEndServerIndex = fromLeafId  * SERVER_COUNT + SERVER_COUNT;

    int toStartServerIndex = toLeafId * SERVER_COUNT;
    int toEndServerIndex = toLeafId * SERVER_COUNT + SERVER_COUNT;

    for (int fromServerIndex = fromStartServerIndex; fromServerIndex < fromEndServerIndex; fromServerIndex++)
    {
        NS_LOG_INFO ("Install application for server: " << fromServerIndex);
        for (int toServerIndex = toStartServerIndex; toServerIndex < toEndServerIndex; toServerIndex++)
        {
            //double flowStartTime = START_TIME + rand_range (0.0, 0.01);
            double flowStartTime = START_TIME;
            uint16_t port = rand_range (PORT_START, PORT_END);

	    Ptr<Node> destServer = servers.Get (toServerIndex);
	    Ptr<Ipv4> ipv4 = destServer->GetObject<Ipv4> ();
	    Ipv4InterfaceAddress destInterface = ipv4->GetAddress (1,0);
	    Ipv4Address destAddress = destInterface.GetLocal ();

//          OnOffHelper source ("ns3::TcpSocketFactory", InetSocketAddress (destAddress, port));
            BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (destAddress, port));

//	    source.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
//	    source.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));

//	    source.SetAttribute ("DataRate", DataRateValue (DataRate(LEAF_SERVER_CAPACITY)));
//	    source.SetAttribute ("PacketSize", UintegerValue (PACKET_SIZE));
 	    source.SetAttribute ("SendSize", UintegerValue (PACKET_SIZE));
            source.SetAttribute ("MaxBytes", UintegerValue(FLOW_SIZE));

            // Install applications
            ApplicationContainer sourceApp = source.Install (servers.Get (fromServerIndex));
            sourceApp.Start (Seconds (flowStartTime));
            sourceApp.Stop (Seconds (END_TIME));

            // Install packet sinks
            PacketSinkHelper sink ("ns3::TcpSocketFactory",
                    InetSocketAddress (Ipv4Address::GetAny (), port));
            ApplicationContainer sinkApp = sink.Install (servers.Get (toServerIndex));
            sinkApp.Start (Seconds (flowStartTime));
            sinkApp.Stop (Seconds (END_TIME));

            NS_LOG_INFO ("\tFlow from server: " << fromServerIndex << " to server: "
                    << toServerIndex << " on port: " << port << " [start time: " << flowStartTime <<"]");
        }
    }
}

int main (int argc, char *argv[])
{
#if 1
    LogComponentEnable ("SparkShuffle", LOG_LEVEL_INFO);
#endif

    // Command line parameters parsing

    std::string runModeStr = "Conga";
    unsigned randomSeed = 0;
    std::string transportProt = "Tcp";

    CommandLine cmd;
    cmd.AddValue ("runMode", "Running mode of this simulation: Conga, Conga-flow, Conga-ECMP (dev use), ECMP", runModeStr);
    cmd.AddValue ("transportProt", "Transport protocol to use: Tcp, DcTcp", transportProt);
    cmd.AddValue ("randomSeed", "Random seed, 0 for random generated", randomSeed);
    cmd.Parse (argc, argv);

    RunMode runMode;
    if (runModeStr.compare ("Conga") == 0)
    {
        runMode = CONGA;
    }
    else if (runModeStr.compare ("Conga-flow") == 0)
    {
        runMode = CONGA_FLOW;
    }
    else if (runModeStr.compare ("Conga-ECMP") == 0)
    {
        runMode = CONGA_ECMP;
    }
    else if (runModeStr.compare ("ECMP") == 0)
    {
        runMode = ECMP;
    }
    else
    {
        NS_LOG_ERROR ("The running mode should be Conga, Conga-flow, Conga-ECMP, Presto and ECMP");
        return 0;
    }

    if (transportProt.compare ("DcTcp") == 0)
    {
	NS_LOG_INFO ("Using DcTcp");
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpDCTCP::GetTypeId ()));
    	Config::SetDefault ("ns3::RedQueueDisc::Mode", StringValue ("QUEUE_MODE_PACKETS"));
    	Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (PACKET_SIZE));
    	Config::SetDefault ("ns3::RedQueueDisc::QueueLimit", UintegerValue (BUFFER_SIZE));
    }

    NS_LOG_INFO ("Config parameters");
    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue(1400));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue (0));
    Config::SetDefault("ns3::TcpSocketBase::MinRto", TimeValue(MicroSeconds(200)));

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
    Ipv4GlobalRoutingHelper globalRoutingHelper;

    if (runMode == CONGA || runMode == CONGA_FLOW || runMode == CONGA_ECMP)
    {
	internet.SetRoutingHelper (staticRoutingHelper);
	internet.Install (servers);

	internet.SetRoutingHelper (congaRoutingHelper);
	internet.Install (spines);
    	internet.Install (leaves);

    }
    else if (runMode == ECMP)
    {
	internet.SetRoutingHelper (globalRoutingHelper);
        Config::SetDefault ("ns3::Ipv4GlobalRouting::PerflowEcmpRouting", BooleanValue(true));

	internet.Install (servers);
	internet.Install (spines);
    	internet.Install (leaves);
    }

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

	    NS_LOG_INFO ("Server: " << j << " is connected to leaf: " << i << " through port: " << netDeviceContainer.Get (0)->GetIfIndex ());

            if (runMode == CONGA || runMode == CONGA_FLOW || runMode == CONGA_ECMP)
            {
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
    }

    NS_LOG_INFO ("Configuring switches");
    // Setting switches
    p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (SPINE_LEAF_CAPACITY)));

    for (int i = 0; i < LEAF_COUNT; i++)
    {
        if (runMode == CONGA || runMode == CONGA_FLOW || runMode == CONGA_ECMP)
	{
	    Ptr<Ipv4CongaRouting> congaLeaf = congaRoutingHelper.GetCongaRouting (leaves.Get (i)->GetObject<Ipv4> ());
            congaLeaf->SetLeafId (i);
	    congaLeaf->SetTDre (MicroSeconds (45));
	    congaLeaf->SetAlpha (0.2);
	    congaLeaf->SetLinkCapacity(DataRate(SPINE_LEAF_CAPACITY));
	    if (runMode == CONGA)
	    {
	        congaLeaf->SetFlowletTimeout (MicroSeconds (500));
	    }
	    if (runMode == CONGA_FLOW)
	    {
	        congaLeaf->SetFlowletTimeout (MilliSeconds (13));
	    }
	    if (runMode == CONGA_ECMP)
	    {
	        congaLeaf->EnableEcmpMode ();
	    }
        }

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
            if (runMode == CONGA || runMode == CONGA_FLOW || runMode == CONGA_ECMP)
            {
		// For each conga leaf switch, routing entry to route the packet to OTHER leaves should be added
                for (int k = 0; k < SPINE_COUNT; k++)
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
		congaSpine->SetTDre (MicroSeconds (45));
		congaSpine->SetAlpha (0.2);
		congaSpine->SetLinkCapacity(DataRate(SPINE_LEAF_CAPACITY));
		if (runMode == CONGA_ECMP)
		{
	    		congaSpine->EnableEcmpMode ();
		}
		congaSpine->AddRoute (leafNetworks[i],
				      Ipv4Mask("255.255.255.0"),
                                      netDeviceContainer.Get (1)->GetIfIndex ());
		NS_LOG_INFO ("Leaf: " << i << " is connected to spine: " << j << " through port: " << netDeviceContainer.Get (0)->GetIfIndex ());
	        if ((i == 0 || i == 1) && (j == 1 || j == 2))
		{
		    congaRoutingHelper.GetCongaRouting (leaves.Get (i)->GetObject<Ipv4> ())->
			InitCongestion (2, netDeviceContainer.Get (0)->GetIfIndex (), 8);
		}
	    }
        }

    }

    if (runMode == ECMP)
    {
        NS_LOG_INFO ("Populate global routing tables");
        Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    }

    double oversubRatio = (SERVER_COUNT * LEAF_SERVER_CAPACITY) / (SPINE_LEAF_CAPACITY * SPINE_COUNT);
    NS_LOG_INFO ("Over-subscription ratio: " << oversubRatio);

    NS_LOG_INFO ("Initialize random seed: " << randomSeed);
    if (randomSeed == 0)
    {
        srand ((unsigned)time (NULL));
    }
    else
    {
        srand (randomSeed);
    }

    NS_LOG_INFO ("Create applications");

    int flowCount = 0;

    install_applications(0, 2, servers, flowCount);
    install_applications(1, 2, servers, flowCount);

    NS_LOG_INFO ("Total flow: " << flowCount);

    NS_LOG_INFO ("Enabling flow monitor");

    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    flowMonitor->CheckForLostPackets ();

    std::stringstream fileName;

    fileName << "9-1-spark-shuffle-" << transportProt <<"-";

    if (runMode == CONGA)
    {
        fileName << "conga-simulation-";
    }
    else if (runMode == CONGA_FLOW)
    {
        fileName << "conga-flow-simulation-";
    }
    else if (runMode == CONGA_ECMP)
    {
        fileName << "conga-ecmp-simulation-";
    }
    else if (runMode == ECMP)
    {
        fileName << "ecmp-simulation-";
    }

    fileName <<randomSeed << "-";

    fileName <<"b" << BUFFER_SIZE;

    p2p.EnablePcapAll (fileName.str ());

    fileName << ".xml";

    NS_LOG_INFO ("Start simulation");
    Simulator::Stop (Seconds (END_TIME));
    Simulator::Run ();

    flowMonitor->SerializeToXmlFile(fileName.str (), true, true);

    Simulator::Destroy ();
    NS_LOG_INFO ("Stop simulation");
}
