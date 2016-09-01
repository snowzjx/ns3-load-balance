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
#include "ns3/link-monitor-module.h"
#include "ns3/traffic-control-module.h"

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
#define QUEUE_DISC_BUFFER_SIZE 250                // 250 packets
#define BUFFER_SIZE 10                            // 10 packets

#define RED_QUEUE_MARKING 65 		        	  // 65 Packets (available only in DcTcp)

// The simulation starting and ending time
#define START_TIME 0.0
#define END_TIME 10.0

#define FLOW_LAUNCH_END_TIME 4.0

// The flow port range, each flow will be assigned a random port number within this range
#define PORT_START 10000
#define PORT_END 50000

// Adopted from the simulation from WANG PENG
// Acknowledged to https://williamcityu@bitbucket.org/williamcityu/2016-socc-simulation.git
#define PACKET_SIZE 1400

#define PRESTO_RATIO 64

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CongaSimulationLarge");

enum RunMode {
    CONGA,
    CONGA_FLOW,
    CONGA_ECMP,
    PRESTO,
    DRB,
    FlowBender,
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

            BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (destAddress, port));
            uint32_t flowSize = gen_random_cdf (cdfTable);
 	    source.SetAttribute ("SendSize", UintegerValue (PACKET_SIZE));
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
    std::string transportProt = "Tcp";
    bool asym = false;
    bool resequenceBuffer = false;
    double flowBenderT = 0.05;
    uint32_t flowBenderN = 5;

    CommandLine cmd;
    cmd.AddValue ("runMode", "Running mode of this simulation: Conga, Conga-flow, Conga-ECMP (dev use), Presto, DRB, FlowBender, ECMP", runModeStr);
    cmd.AddValue ("randomSeed", "Random seed, 0 for random generated", randomSeed);
    cmd.AddValue ("cdfFileName", "File name for flow distribution", cdfFileName);
    cmd.AddValue ("load", "Load of the network, 0.0 - 1.0", load);
    cmd.AddValue ("transportProt", "Transport protocol to use: Tcp, DcTcp", transportProt);
    cmd.AddValue ("resequenceBuffer", "Whether enabling the resequenceBuffer", resequenceBuffer);
    cmd.AddValue ("asym", "Whether enabling the asym topology", asym);
    cmd.AddValue ("flowBenderT", "The T in flowBender", flowBenderT);
    cmd.AddValue ("flowBenderN", "The N in flowBender", flowBenderN);
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
    else if (runModeStr.compare ("Presto") == 0)
    {
        runMode = PRESTO;
    }
    else if (runModeStr.compare ("DRB") == 0)
    {
        runMode = DRB;
    }
    else if (runModeStr.compare ("FlowBender") == 0)
    {
        runMode = FlowBender;
    }
    else if (runModeStr.compare ("ECMP") == 0)
    {
        runMode = ECMP;
    }
    else
    {
        NS_LOG_ERROR ("The running mode should be Conga, Conga-flow, Conga-ECMP, Presto, DRB and ECMP");
        return 0;
    }

    if (load < 0.0 || load >= 1.0)
    {
        NS_LOG_ERROR ("The network load should within 0.0 and 1.0");
        return 0;
    }

    if (transportProt.compare ("DcTcp") == 0)
    {
	    NS_LOG_INFO ("Enabling DcTcp");
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpDCTCP::GetTypeId ()));
    	Config::SetDefault ("ns3::RedQueueDisc::Mode", StringValue ("QUEUE_MODE_PACKETS"));
    	Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (PACKET_SIZE));
    	Config::SetDefault ("ns3::RedQueueDisc::QueueLimit", UintegerValue (QUEUE_DISC_BUFFER_SIZE));
        Config::SetDefault ("ns3::QueueDisc::Quota", UintegerValue (BUFFER_SIZE));
    }
    else
    {
        Config::SetDefault ("ns3::RedQueueDisc::Mode", StringValue ("QUEUE_MODE_PACKETS"));
    	Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (PACKET_SIZE));
    	Config::SetDefault ("ns3::RedQueueDisc::QueueLimit", UintegerValue (QUEUE_DISC_BUFFER_SIZE));
        Config::SetDefault ("ns3::QueueDisc::Quota", UintegerValue (BUFFER_SIZE));
    }

    if (resequenceBuffer)
    {
	    NS_LOG_INFO ("Enabling Resequence Buffer");
	    Config::SetDefault ("ns3::TcpSocketBase::ResequenceBuffer", BooleanValue (true));
        Config::SetDefault ("ns3::TcpResequenceBuffer::InOrderQueueTimerLimit", TimeValue (MicroSeconds (15)));
        Config::SetDefault ("ns3::TcpResequenceBuffer::SizeLimit", UintegerValue (100));
        Config::SetDefault ("ns3::TcpResequenceBuffer::OutOrderQueueTimerLimit", TimeValue (MicroSeconds (100)));
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
    Ipv4ListRoutingHelper listRoutingHelper;

    Ipv4DrbHelper drbHelper;

    if (runMode == CONGA || runMode == CONGA_FLOW || runMode == CONGA_ECMP)
    {
	internet.SetRoutingHelper (staticRoutingHelper);
        internet.Install (servers);

        internet.SetRoutingHelper (congaRoutingHelper);
        internet.Install (spines);
    	internet.Install (leaves);

    }
    else if (runMode == PRESTO || runMode == DRB)
    {
        listRoutingHelper.Add (globalRoutingHelper, 0);
        internet.SetRoutingHelper (listRoutingHelper);

        internet.Install (servers);
        internet.Install (spines);

        internet.SetDrb (true);
        internet.Install (leaves);
    }
    else if (runMode == ECMP || runMode == FlowBender)
    {
	    internet.SetRoutingHelper (globalRoutingHelper);
        Config::SetDefault ("ns3::Ipv4GlobalRouting::PerflowEcmpRouting", BooleanValue(true));

	    internet.Install (servers);
	    internet.Install (spines);
    	internet.Install (leaves);
    }

    if (runMode == FlowBender)
    {
        NS_LOG_INFO ("Enabling Flow Bender");
        Config::SetDefault ("ns3::TcpSocketBase::FlowBender", BooleanValue (true));
        Config::SetDefault ("ns3::TcpFlowBender::RTT", TimeValue (MicroSeconds (80)));
        Config::SetDefault ("ns3::TcpFlowBender::T", DoubleValue (flowBenderT));
        Config::SetDefault ("ns3::TcpFlowBender::N", UintegerValue (flowBenderN));
    }

    NS_LOG_INFO ("Install channels and assign addresses");

    PointToPointHelper p2p;
    Ipv4AddressHelper ipv4;

    TrafficControlHelper tc;
    if (transportProt.compare ("DcTcp") == 0)
    {
        tc.SetRootQueueDisc ("ns3::RedQueueDisc", "MinTh", DoubleValue (RED_QUEUE_MARKING),
                                                  "MaxTh", DoubleValue (RED_QUEUE_MARKING));
    }
    else
    {
        tc.SetRootQueueDisc ("ns3::RedQueueDisc", "MinTh", DoubleValue (QUEUE_DISC_BUFFER_SIZE),
                                                  "MaxTh", DoubleValue (QUEUE_DISC_BUFFER_SIZE));
    }

    NS_LOG_INFO ("Configuring servers");
    // Setting servers
    p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (LEAF_SERVER_CAPACITY)));
    p2p.SetChannelAttribute ("Delay", TimeValue(LINK_LATENCY));
    //if (transportProt.compare ("Tcp") == 0)
    //{
     	p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (BUFFER_SIZE));
    //}
    //else
    //{
	//p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (10));
    //}

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
		//if (transportProt.compare ("DcTcp") == 0)
		//{
		//NS_LOG_INFO ("Install RED Queue for leaf: " << i << " and server: " << j);
	        tc.Install (netDeviceContainer);
            //}
            Ipv4InterfaceContainer interfaceContainer = ipv4.Assign (netDeviceContainer);
		//if (transportProt.compare ("Tcp") == 0)
            //{
                //tc.Uninstall (netDeviceContainer);
            //}

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
	    if (asym && i == LEAF_COUNT - 1 && j == SPINE_COUNT - 1)
	    {
    		p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (SPINE_LEAF_CAPACITY / 2)));
	    }
            NetDeviceContainer netDeviceContainer = p2p.Install (nodeContainer);
		//if (transportProt.compare ("DcTcp") == 0)
		//{
		//NS_LOG_INFO ("Install RED Queue for leaf: " << i << " and spine: " << j);
	        tc.Install (netDeviceContainer);
            //}
         Ipv4InterfaceContainer ipv4InterfaceContainer = ipv4.Assign (netDeviceContainer);
		//if (transportProt.compare ("Tcp") == 0)
            //{
                //tc.Uninstall (netDeviceContainer);
            //}


            if (runMode == CONGA || runMode == CONGA_FLOW || runMode == CONGA_ECMP)
            {
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
		if (runMode == CONGA_ECMP)
		{
	    		congaSpine->EnableEcmpMode ();
		}
		congaSpine->AddRoute (leafNetworks[i],
				      Ipv4Mask("255.255.255.0"),
                                      netDeviceContainer.Get (1)->GetIfIndex ());
	    }

	    if (runMode == PRESTO)
            {
		Ptr<Ipv4Drb> drb = drbHelper.GetIpv4Drb (leaves.Get (i)->GetObject<Ipv4> ());
		drb->AddCoreSwitchAddress (PRESTO_RATIO, ipv4InterfaceContainer.GetAddress (1));
            }
 	    if (runMode == DRB)
	    {
		Ptr<Ipv4Drb> drb = drbHelper.GetIpv4Drb (leaves.Get (i)->GetObject<Ipv4> ());
		drb->AddCoreSwitchAddress (1, ipv4InterfaceContainer.GetAddress (1));
            }
        }

    }

    if (runMode == ECMP || runMode == PRESTO || runMode == DRB || runMode == FlowBender)
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
    double requestRate = load * LEAF_SERVER_CAPACITY * SERVER_COUNT / oversubRatio / (8 * avg_cdf (cdfTable)) / SERVER_COUNT;
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

    NS_LOG_INFO ("Enabling link monitor");

    Ptr<LinkMonitor> linkMonitor = Create<LinkMonitor> ();
    for (int i = 0; i < SPINE_COUNT; i++)
    {
      std::stringstream name;
      name << "Spine " << i;
      Ptr<Ipv4LinkProbe> spineLinkProbe = Create<Ipv4LinkProbe> (spines.Get (i), linkMonitor);
      spineLinkProbe->SetProbeName (name.str ());
      spineLinkProbe->SetCheckTime (Seconds (0.01));
      spineLinkProbe->SetDataRateAll (DataRate (SPINE_LEAF_CAPACITY));
    }
    for (int i = 0; i < LEAF_COUNT; i++)
    {
      std::stringstream name;
      name << "Leaf " << i;
      Ptr<Ipv4LinkProbe> leafLinkProbe = Create<Ipv4LinkProbe> (leaves.Get (i), linkMonitor);
      leafLinkProbe->SetProbeName (name.str ());
      leafLinkProbe->SetCheckTime (Seconds (0.01));
      leafLinkProbe->SetDataRateAll (DataRate (SPINE_LEAF_CAPACITY));
    }

    linkMonitor->Start (Seconds (START_TIME));
    linkMonitor->Stop (Seconds (END_TIME));

    NS_LOG_INFO ("Start simulation");
    Simulator::Stop (Seconds (END_TIME));
    Simulator::Run ();

    flowMonitor->CheckForLostPackets ();

    std::stringstream flowMonitorFilename;
    std::stringstream linkMonitorFilename;

    flowMonitorFilename << "12-1-large-load-" << load << "-"  << transportProt <<"-";
    linkMonitorFilename << "12-1-large-load-" << load << "-"  << transportProt <<"-";

    if (runMode == CONGA)
    {
        flowMonitorFilename << "conga-simulation-";
        linkMonitorFilename << "conga-simulation-";
    }
    else if (runMode == CONGA_FLOW)
    {
        flowMonitorFilename << "conga-flow-simulation-";
        linkMonitorFilename << "conga-flow-simulation-";
    }
    else if (runMode == CONGA_ECMP)
    {
        flowMonitorFilename << "conga-ecmp-simulation-";
        linkMonitorFilename << "conga-ecmp-simulation-";
    }
    else if (runMode == PRESTO)
    {
	    flowMonitorFilename << "presto-simulation-";
        linkMonitorFilename << "presto-simulation-";
    }
    else if (runMode == DRB)
    {
        flowMonitorFilename << "drb-simulation-";
        linkMonitorFilename << "drb-simulation-";
    }
    else if (runMode == ECMP)
    {
        flowMonitorFilename << "ecmp-simulation-";
        linkMonitorFilename << "ecmp-simulation-";
    }
    else if (runMode == FlowBender)
    {
        flowMonitorFilename << "flow-bender-" << flowBenderT << "-" << flowBenderN << "-simulation-";
        linkMonitorFilename << "flow-bender-" << flowBenderT << "-" << flowBenderN << "-simulation-";
    }

    flowMonitorFilename << randomSeed << "-";
    linkMonitorFilename << randomSeed << "-";

    if (asym)
    {
	flowMonitorFilename << "asym-";
	linkMonitorFilename << "asym-";
    }

    if (resequenceBuffer)
    {
	flowMonitorFilename << "rb-";
	linkMonitorFilename << "rb-";
    }

    flowMonitorFilename << "b" << BUFFER_SIZE << ".xml";
    linkMonitorFilename << "b" << BUFFER_SIZE << "-link-utility.out";

    flowMonitor->SerializeToXmlFile(flowMonitorFilename.str (), true, true);
    linkMonitor->OutputToFile (linkMonitorFilename.str (), &LinkMonitor::DefaultFormat);

    Simulator::Destroy ();
    free_cdf (cdfTable);
    NS_LOG_INFO ("Stop simulation");
}
