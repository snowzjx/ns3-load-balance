#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-conga-routing-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-drb-helper.h"

#include <vector>
#include <utility>

// The CDF in TrafficGenerator
extern "C"
{
#include "cdf.h"
}

// There are 8 servers connecting to each leaf switch
#define SERVER_COUNT 8
#define SPINE_COUNT 4
#define LEAF_COUNT 4

#define SPINE_LEAF_CAPACITY  10000000000          // 10Gbps
#define LEAF_SERVER_CAPACITY 10000000000          // 10Gbps
#define LINK_LATENCY MicroSeconds(10)             // 10 MicroSeconds
#define BUFFER_SIZE 150                           // 150 Packets

// The simulation starting and ending time
#define START_TIME 0.0
#define END_TIME 1.5

#define FLOW_LAUNCH_END_TIME 0.5

// The flow port range, each flow will be assigned a random port number within this range
#define PORT_START 10000
#define PORT_END 50000

// Adopted from the simulation from WANG PENG
// Acknowledged to https://williamcityu@bitbucket.org/williamcityu/2016-socc-simulation.git
#define PACKET_SIZE 1400

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CongaSimulationLarge");

enum RunMode {
    CONGA,
    CONGA_FLOW,
    CONGA_ECMP,
    ECMP
};

// Port from Traffic Generator
// Acknowledged to https://github.com/HKUST-SING/TrafficGenerator/blob/master/src/common/common.c
double poission_gen_interval(double avg_rate)
{
    if (avg_rate > 0)
       return -logf(1.0 - (double)rand() / RAND_MAX) / avg_rate;
    else
       return 0;
}

template<typename T>
T rand_range (T min, T max)
{
    return min + ((double)max - min) * rand () / RAND_MAX;
}

void install_applications (int fromLeafId, NodeContainer servers, double requestRate, struct cdf_table *cdfTable, int &flowCount)
{
    NS_LOG_INFO ("Install applications:");
    for (int i = 0; i < SERVER_COUNT; i++)
    {
        int fromServerIndex = fromLeafId * SERVER_COUNT + i;

        double startTime = START_TIME + poission_gen_interval (requestRate);
        while (startTime < FLOW_LAUNCH_END_TIME)
        {
            flowCount ++;
            uint16_t port = rand_range (PORT_START, PORT_END);

            int destServerIndex = fromServerIndex;
	    while (destServerIndex >= fromLeafId * SERVER_COUNT && destServerIndex < fromLeafId * SERVER_COUNT + SERVER_COUNT)
            {
		destServerIndex = rand_range (0, SERVER_COUNT * LEAF_COUNT);
            }

	    Ptr<Node> destServer = servers.Get (destServerIndex);
	    Ptr<Ipv4> ipv4 = destServer->GetObject<Ipv4> ();
	    Ipv4InterfaceAddress destInterface = ipv4->GetAddress (1,0);
	    Ipv4Address destAddress = destInterface.GetLocal ();

            OnOffHelper source ("ns3::TcpSocketFactory", InetSocketAddress (destAddress, port));

            uint32_t flowSize = gen_random_cdf (cdfTable);

	    source.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
	    source.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
           
	    source.SetAttribute ("DataRate", DataRateValue (DataRate(LEAF_SERVER_CAPACITY)));  
	    source.SetAttribute ("PacketSize", UintegerValue (PACKET_SIZE)); 
            source.SetAttribute ("MaxBytes", UintegerValue(flowSize));

            // Install apps
            ApplicationContainer sourceApp = source.Install (servers.Get (fromServerIndex));
            sourceApp.Start (Seconds (startTime));
            sourceApp.Stop (Seconds (END_TIME));

            // Install packet sinks
            PacketSinkHelper sink ("ns3::TcpSocketFactory",
                    InetSocketAddress (Ipv4Address::GetAny (), port));
            ApplicationContainer sinkApp = sink.Install (servers. Get (destServerIndex));
            sinkApp.Start (Seconds (startTime));
            sinkApp.Stop (Seconds (END_TIME));

            NS_LOG_INFO ("\tFlow from server: " << fromServerIndex << " to server: "
                    << destServerIndex << " on port: " << port << " with flow size: "
                    << flowSize << " [start time: " << startTime <<"]");

            startTime += poission_gen_interval (requestRate);
        }
    }
}

int main (int argc, char *argv[])
{
#if 1
    LogComponentEnable ("CongaSimulationLarge", LOG_LEVEL_INFO);
#endif

    // Command line parameters parsing

    std::string runModeStr = "Conga";
    unsigned randomSeed = 0;
    std::string cdfFileName = "";
    double load = 0.0;

    CommandLine cmd;
    cmd.AddValue ("runMode", "Running mode of this simulation: Conga, Conga-flow, Conga-ECMP (dev use), ECMP", runModeStr);
    cmd.AddValue ("randomSeed", "Random seed, 0 for random generated", randomSeed);
    cmd.AddValue ("cdfFileName", "File name for flow distribution", cdfFileName);
    cmd.AddValue ("load", "Load of the network, 0.0 - 1.0", load);
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

    if (load < 0.0 || load > 1.0)
    {
        NS_LOG_ERROR ("The network load should within 0.0 and 1.0");
        return 0;
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

    NS_LOG_INFO ("Configuring servers");
    // Setting servers
    p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (LEAF_SERVER_CAPACITY)));
    p2p.SetChannelAttribute ("Delay", TimeValue(LINK_LATENCY));
    p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (BUFFER_SIZE));
    
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
            Ipv4InterfaceContainer interfaceContainer = ipv4.Assign (netDeviceContainer);
	    
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
	    congaLeaf->SetTDre (MicroSeconds (30));
	    congaLeaf->SetAlpha (0.2);
	    congaLeaf->SetLinkCapacity(DataRate(SPINE_LEAF_CAPACITY));
	    if (runMode == CONGA)
	    {
	        congaLeaf->SetFlowletTimeout (MicroSeconds (200));
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
            Ipv4InterfaceContainer ipv4InterfaceContainer = ipv4.Assign (netDeviceContainer);

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
		congaSpine->SetTDre (MicroSeconds (30));
		congaSpine->SetAlpha (0.2);
		congaSpine->SetLinkCapacity(DataRate(SPINE_LEAF_CAPACITY));
		if (runMode == CONGA_ECMP)
		{
	    		congaSpine->EnableEcmpMode ();
		}
		congaSpine->AddRoute (leafNetworks[i], 
				      Ipv4Mask("255.255.255.0"), 
                                      netDeviceContainer.Get (1)->GetIfIndex ());
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

    NS_LOG_INFO ("Initialize CDF table");
    struct cdf_table* cdfTable = new cdf_table ();
    init_cdf (cdfTable);
    load_cdf (cdfTable, cdfFileName.c_str ());

    NS_LOG_INFO ("Calculating request rate");
    double requestRate = load * LEAF_SERVER_CAPACITY * SERVER_COUNT / oversubRatio / (8 * avg_cdf (cdfTable));
    NS_LOG_INFO ("Average request rate: " << requestRate << " per second");

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

    for (int fromLeafId = 0; fromLeafId < LEAF_COUNT; fromLeafId ++)
    {
        install_applications(fromLeafId, servers, requestRate, cdfTable, flowCount);
    }

    NS_LOG_INFO ("Total flow: " << flowCount);

    NS_LOG_INFO ("Enabling flow monitor");

    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    NS_LOG_INFO ("Start simulation");
    Simulator::Stop (Seconds (END_TIME));
    Simulator::Run ();

    flowMonitor->CheckForLostPackets ();

    std::stringstream fileName;

    fileName << "large-load-" << load <<"-";

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

    fileName <<"b" << BUFFER_SIZE << ".xml";

    flowMonitor->SerializeToXmlFile(fileName.str (), true, true);

    Simulator::Destroy ();
    free_cdf (cdfTable);
    NS_LOG_INFO ("Stop simulation");
}
