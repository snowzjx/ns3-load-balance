#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-conga-routing-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/ipv4-drb-helper.h"

#include "ns3/link-monitor.h"
#include "ns3/ipv4-link-probe.h"

#include <vector>
#include <utility>

// The CDF in TrafficGenerator
extern "C"
{
#include "cdf.h"
}

// There are 8 servers connecting to each leaf switch
#define LEAF_NODE_COUNT 8

#define SPINE_LEAF_CAPACITY  10000000000          // 10Gbps
#define LEAF_SERVER_CAPACITY 10000000000          // 10Gbps
#define LINK_LATENCY MicroSeconds(10)             // 10 MicroSeconds
#define BUFFER_SIZE 250                           // 250 Packets

// The simulation starting and ending time
#define START_TIME 0.0
#define END_TIME 20.0

#define FLOW_LAUNCH_END_TIME 0.5

// The flow port range, each flow will be assigned a random port number within this range
#define PORT_START 10000
#define PORT_END 50000

// Adopted from the simulation from WANG PENG
// Acknowledged to https://williamcityu@bitbucket.org/williamcityu/2016-socc-simulation.git
#define PACKET_SIZE 1400

#define PRESTO_RATIO 64

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CongaSimulation");

enum RunMode {
    CONGA,
    CONGA_FLOW,
    CONGA_ECMP,
    PRESTO,
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

void install_applications (NodeContainer fromServers, NodeContainer destServers, double requestRate,
        struct cdf_table *cdfTable, std::vector<std::pair<Ipv4Address, uint32_t> > toAddresses, int &flowCount)
{
    NS_LOG_INFO ("Install applications:");
    for (int i = 0; i < LEAF_NODE_COUNT; i++)
    {
        double startTime = START_TIME + poission_gen_interval (requestRate);
        while (startTime < FLOW_LAUNCH_END_TIME)
        {
            flowCount ++;
            uint16_t port = rand_range (PORT_START, PORT_END);
            int destIndex = rand_range (0, LEAF_NODE_COUNT - 1);

            BulkSendHelper source ("ns3::TcpSocketFactory",
                    InetSocketAddress (toAddresses[destIndex].first, port));
            uint32_t flowSize = gen_random_cdf (cdfTable);
 	    source.SetAttribute ("SendSize", UintegerValue (PACKET_SIZE));
            source.SetAttribute ("MaxBytes", UintegerValue(flowSize));

            // Install apps
            ApplicationContainer sourceApp = source.Install (fromServers.Get (i));
            sourceApp.Start (Seconds (startTime));
            sourceApp.Stop (Seconds (END_TIME));

            // Install packet sinks
            PacketSinkHelper sink ("ns3::TcpSocketFactory",
                    InetSocketAddress (Ipv4Address::GetAny (), port));
            ApplicationContainer sinkApp = sink.Install (destServers. Get (destIndex));
            sinkApp.Start (Seconds (startTime));
            sinkApp.Stop (Seconds (END_TIME));

            NS_LOG_INFO ("\tFlow from server: " << i << " to server: "
                    << destIndex << " on port: " << port << " with flow size: "
                    << flowSize << " [start time: " << startTime <<"]");

            startTime += poission_gen_interval (requestRate);
        }
    }
}

int main (int argc, char *argv[])
{
#if 1
    LogComponentEnable ("CongaSimulation", LOG_LEVEL_INFO);
#endif

    // Command line parameters parsing

    std::string runModeStr = "Conga";
    unsigned randomSeed = 0;
    std::string cdfFileName = "";
    double load = 0.0;
    bool asym = false;

    CommandLine cmd;
    cmd.AddValue ("runMode", "Running mode of this simulation: Conga, Conga-flow, Conga-ECMP (dev use), Presto, ECMP", runModeStr);
    cmd.AddValue ("randomSeed", "Random seed, 0 for random generated", randomSeed);
    cmd.AddValue ("cdfFileName", "File name for flow distribution", cdfFileName);
    cmd.AddValue ("load", "Load of the network, 0.0 - 1.0", load);
    cmd.AddValue ("asym", "Whether the topology is sym or asym", asym);
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

    NS_LOG_INFO ("Declare data structures");
    std::vector<std::pair<Ipv4Address, uint32_t> > serversAddr0 = std::vector<std::pair<Ipv4Address, uint32_t> > (LEAF_NODE_COUNT);
    std::vector<std::pair<Ipv4Address, uint32_t> > serversAddr1 = std::vector<std::pair<Ipv4Address, uint32_t> > (LEAF_NODE_COUNT);

    NS_LOG_INFO ("Config parameters");
    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue(1400));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue (0));
    Config::SetDefault("ns3::TcpSocketBase::MinRto", TimeValue(MicroSeconds(200)));

    double oversubRatio = (LEAF_NODE_COUNT * LEAF_SERVER_CAPACITY) / (SPINE_LEAF_CAPACITY * 4); // Each leaf has 4 up links to spine
    NS_LOG_INFO ("Over-subscription ratio: " << oversubRatio);

    NS_LOG_INFO ("Initialize CDF table");
    struct cdf_table* cdfTable = new cdf_table ();
    init_cdf (cdfTable);
    load_cdf (cdfTable, cdfFileName.c_str ());

    NS_LOG_INFO ("Calculating request rate");
    double requestRate = load * LEAF_SERVER_CAPACITY * LEAF_NODE_COUNT / oversubRatio / (8 * avg_cdf (cdfTable));
    NS_LOG_INFO ("Average request rate: " << requestRate << " per second");

    NS_LOG_INFO ("Create nodes");

    Ptr<Node> leaf0 = CreateObject<Node> ();
    Ptr<Node> leaf1 = CreateObject<Node> ();
    Ptr<Node> spine0 = CreateObject<Node> ();
    Ptr<Node> spine1 = CreateObject<Node> ();

    NodeContainer servers0;
    servers0.Create (LEAF_NODE_COUNT);

    NodeContainer servers1;
    servers1.Create (LEAF_NODE_COUNT);


    NS_LOG_INFO ("Install Internet stacks");

    InternetStackHelper internet;
    Ipv4StaticRoutingHelper staticRoutingHelper;
    Ipv4CongaRoutingHelper congaRoutingHelper;
    Ipv4GlobalRoutingHelper globalRoutingHelper;
    Ipv4ListRoutingHelper listRoutingHelper;

    if (runMode == CONGA || runMode == CONGA_FLOW || runMode == CONGA_ECMP)
    {
	internet.SetRoutingHelper (staticRoutingHelper);
	internet.Install (servers0);
	internet.Install (servers1);

	internet.SetRoutingHelper (congaRoutingHelper);
	internet.Install (spine0);
    	internet.Install (spine1);
    	internet.Install (leaf0);
    	internet.Install (leaf1);
    }
    else if (runMode == ECMP)
    {
	internet.SetRoutingHelper (globalRoutingHelper);
        Config::SetDefault ("ns3::Ipv4GlobalRouting::PerflowEcmpRouting", BooleanValue(true));

	internet.Install (servers0);
	internet.Install (servers1);
	internet.Install (spine0);
    	internet.Install (spine1);
    	internet.Install (leaf0);
    	internet.Install (leaf1);
    }
    else if (runMode == PRESTO)
    {
	listRoutingHelper.Add (globalRoutingHelper, 0);
	internet.SetRoutingHelper (listRoutingHelper);

	internet.Install (servers0);
	internet.Install (servers1);
    	internet.Install (spine0);
    	internet.Install (spine1);

	internet.SetDrb (true);
   	internet.Install (leaf0);
    	internet.Install (leaf1);
    }


    NS_LOG_INFO ("Install channels and assign addresses");

    PointToPointHelper p2p;
    Ipv4AddressHelper ipv4;

    // Setting switches
    p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (SPINE_LEAF_CAPACITY)));
    p2p.SetChannelAttribute ("Delay", TimeValue(LINK_LATENCY));
    p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (BUFFER_SIZE));

    NodeContainer leaf0_spine0_1 = NodeContainer (leaf0, spine0);
    NodeContainer leaf0_spine0_2 = NodeContainer (leaf0, spine0);

    NodeContainer leaf0_spine1_1 = NodeContainer (leaf0, spine1);
    NodeContainer leaf0_spine1_2 = NodeContainer (leaf0, spine1);

    NodeContainer leaf1_spine0_1 = NodeContainer (leaf1, spine0);
    NodeContainer leaf1_spine0_2 = NodeContainer (leaf1, spine0);

    NodeContainer leaf1_spine1_1 = NodeContainer (leaf1, spine1);

    NetDeviceContainer netdevice_leaf0_spine0_1 = p2p.Install (leaf0_spine0_1);
    NetDeviceContainer netdevice_leaf0_spine0_2 = p2p.Install (leaf0_spine0_2);

    NetDeviceContainer netdevice_leaf0_spine1_1 = p2p.Install (leaf0_spine1_1);
    NetDeviceContainer netdevice_leaf0_spine1_2 = p2p.Install (leaf0_spine1_2);

    NetDeviceContainer netdevice_leaf1_spine0_1 = p2p.Install (leaf1_spine0_1);
    NetDeviceContainer netdevice_leaf1_spine0_2 = p2p.Install (leaf1_spine0_2);

    NetDeviceContainer netdevice_leaf1_spine1_1 = p2p.Install (leaf1_spine1_1);

    NetDeviceContainer netdevice_leaf1_spine1_2;

    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer addr_leaf0_spine0_1 = ipv4.Assign (netdevice_leaf0_spine0_1);
    Ipv4InterfaceContainer addr_leaf0_spine0_2 = ipv4.Assign (netdevice_leaf0_spine0_2);

    Ipv4InterfaceContainer addr_leaf0_spine1_1 = ipv4.Assign (netdevice_leaf0_spine1_1);
    Ipv4InterfaceContainer addr_leaf0_spine1_2 = ipv4.Assign (netdevice_leaf0_spine1_2);

    Ipv4InterfaceContainer addr_leaf1_spine0_1 = ipv4.Assign (netdevice_leaf1_spine0_1);
    Ipv4InterfaceContainer addr_leaf1_spine0_2 = ipv4.Assign (netdevice_leaf1_spine0_2);

    Ipv4InterfaceContainer addr_leaf1_spine1_1 = ipv4.Assign (netdevice_leaf1_spine1_1);

    Ipv4InterfaceContainer addr_leaf1_spine1_2;

    if (asym == false)
    {
        NS_LOG_INFO ("Symmetric topology, generating leaf1-spine1-2");
        NodeContainer leaf1_spine1_2 = NodeContainer (leaf1, spine1);
        netdevice_leaf1_spine1_2 = p2p.Install (leaf1_spine1_2);
        addr_leaf1_spine1_2 = ipv4.Assign (netdevice_leaf1_spine1_2);
    }
    else
    {
        NS_LOG_INFO ("Asymmetric topology, no leaf1-spine1-2");
    }

    // Setting servers under leaf 0
    p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (LEAF_SERVER_CAPACITY)));

    ipv4.SetBase ("10.1.2.0", "255.255.255.0");
    for (int i = 0; i < LEAF_NODE_COUNT; i++)
    {
        NodeContainer nodeContainer = NodeContainer (leaf0, servers0.Get (i));
        NetDeviceContainer netDeviceContainer = p2p.Install (nodeContainer);
        Ipv4InterfaceContainer interfaceContainer = ipv4.Assign (netDeviceContainer);
        serversAddr0[i] = std::make_pair(interfaceContainer.GetAddress (1), netDeviceContainer.Get (0)->GetIfIndex ());
    }

    // Setting servers under leaf 1
    ipv4.SetBase ("10.1.3.0", "255.255.255.0");
    for (int i = 0; i < LEAF_NODE_COUNT; i++)
    {
        NodeContainer nodeContainer = NodeContainer (leaf1, servers1.Get (i));
        NetDeviceContainer netDeviceContainer = p2p.Install (nodeContainer);
        Ipv4InterfaceContainer interfaceContainer = ipv4.Assign (netDeviceContainer);
        serversAddr1[i] = std::make_pair(interfaceContainer.GetAddress (1), netDeviceContainer.Get (0)->GetIfIndex ());
    }

    NS_LOG_INFO ("Ip addresses in servers 0: ");
    for (int i = 0; i < LEAF_NODE_COUNT; i++)
    {
        NS_LOG_INFO ("Server: " << serversAddr0[i].first << " is connected to leaf0 in port: " << serversAddr0[i].second);
    }

    NS_LOG_INFO ("Ip addresses in servers 1: ");
    for (int i = 0; i < LEAF_NODE_COUNT; i++)
    {
        NS_LOG_INFO ("Server: " << serversAddr1[i].first << " is connected to leaf1 in port: " << serversAddr1[i].second);
    }

    if (runMode == CONGA || runMode == CONGA_FLOW || runMode == CONGA_ECMP)
    {
        NS_LOG_INFO ("Initing Conga routing tables");

        // First, init all the servers
        for (int serverIndex = 0; serverIndex < LEAF_NODE_COUNT; ++serverIndex)
        {
	    staticRoutingHelper.GetStaticRouting (servers0.Get (serverIndex)->GetObject<Ipv4> ())->
		AddNetworkRouteTo (Ipv4Address ("0.0.0.0"), Ipv4Mask ("0.0.0.0"), 1);

            congaRoutingHelper.GetCongaRouting (leaf0->GetObject<Ipv4> ())->
		AddRoute (serversAddr0[serverIndex].first, Ipv4Mask("255.255.255.255"), serversAddr0[serverIndex].second);

	    staticRoutingHelper.GetStaticRouting (servers1.Get (serverIndex)->GetObject<Ipv4> ())->
		AddNetworkRouteTo (Ipv4Address ("0.0.0.0"), Ipv4Mask ("0.0.0.0"), 1);

    	    congaRoutingHelper.GetCongaRouting (leaf1->GetObject<Ipv4> ())->
		AddRoute (serversAddr1[serverIndex].first, Ipv4Mask("255.255.255.255"), serversAddr1[serverIndex].second);
        }

	congaRoutingHelper.GetCongaRouting (leaf0->GetObject<Ipv4> ())->
		AddRoute (Ipv4Address ("10.1.3.0"), Ipv4Mask ("255.255.255.0"), netdevice_leaf0_spine0_1.Get (0)->GetIfIndex ());
	congaRoutingHelper.GetCongaRouting (leaf0->GetObject<Ipv4> ())->
		AddRoute (Ipv4Address ("10.1.3.0"), Ipv4Mask ("255.255.255.0"), netdevice_leaf0_spine0_2.Get (0)->GetIfIndex ());
	congaRoutingHelper.GetCongaRouting (leaf0->GetObject<Ipv4> ())->
		AddRoute (Ipv4Address ("10.1.3.0"), Ipv4Mask ("255.255.255.0"), netdevice_leaf0_spine1_1.Get (0)->GetIfIndex ());
	congaRoutingHelper.GetCongaRouting (leaf0->GetObject<Ipv4> ())->
		AddRoute (Ipv4Address ("10.1.3.0"), Ipv4Mask ("255.255.255.0"), netdevice_leaf0_spine1_2.Get (0)->GetIfIndex ());

	congaRoutingHelper.GetCongaRouting (leaf1->GetObject<Ipv4> ())->
		AddRoute (Ipv4Address ("10.1.2.0"), Ipv4Mask ("255.255.255.0"), netdevice_leaf1_spine0_1.Get (0)->GetIfIndex ());
	congaRoutingHelper.GetCongaRouting (leaf1->GetObject<Ipv4> ())->
		AddRoute (Ipv4Address ("10.1.2.0"), Ipv4Mask ("255.255.255.0"), netdevice_leaf1_spine0_2.Get (0)->GetIfIndex ());
	congaRoutingHelper.GetCongaRouting (leaf1->GetObject<Ipv4> ())->
		AddRoute (Ipv4Address ("10.1.2.0"), Ipv4Mask ("255.255.255.0"), netdevice_leaf1_spine1_1.Get (0)->GetIfIndex ());
	if (!asym)
	{
	    congaRoutingHelper.GetCongaRouting (leaf1->GetObject<Ipv4> ())->
		    AddRoute (Ipv4Address ("10.1.2.0"), Ipv4Mask ("255.255.255.0"), netdevice_leaf1_spine1_2.Get (0)->GetIfIndex ());
	}

	congaRoutingHelper.GetCongaRouting (spine0->GetObject<Ipv4> ())->
		AddRoute (Ipv4Address ("10.1.3.0"), Ipv4Mask ("255.255.255.0"), netdevice_leaf1_spine0_1.Get (1)->GetIfIndex ());
	congaRoutingHelper.GetCongaRouting (spine0->GetObject<Ipv4> ())->
		AddRoute (Ipv4Address ("10.1.3.0"), Ipv4Mask ("255.255.255.0"), netdevice_leaf1_spine0_2.Get (1)->GetIfIndex ());
	congaRoutingHelper.GetCongaRouting (spine0->GetObject<Ipv4> ())->
		AddRoute (Ipv4Address ("10.1.2.0"), Ipv4Mask ("255.255.255.0"), netdevice_leaf0_spine0_1.Get (1)->GetIfIndex ());
	congaRoutingHelper.GetCongaRouting (spine0->GetObject<Ipv4> ())->
		AddRoute (Ipv4Address ("10.1.2.0"), Ipv4Mask ("255.255.255.0"), netdevice_leaf0_spine0_2.Get (1)->GetIfIndex ());

	congaRoutingHelper.GetCongaRouting (spine1->GetObject<Ipv4> ())->
		AddRoute (Ipv4Address ("10.1.3.0"), Ipv4Mask ("255.255.255.0"), netdevice_leaf1_spine1_1.Get (1)->GetIfIndex ());
	if (!asym)
	{
	    congaRoutingHelper.GetCongaRouting (spine1->GetObject<Ipv4> ())->
		    AddRoute (Ipv4Address ("10.1.3.0"), Ipv4Mask ("255.255.255.0"), netdevice_leaf1_spine1_2.Get (1)->GetIfIndex ());
	}
	congaRoutingHelper.GetCongaRouting (spine1->GetObject<Ipv4> ())->
		AddRoute (Ipv4Address ("10.1.2.0"), Ipv4Mask ("255.255.255.0"), netdevice_leaf0_spine1_1.Get (1)->GetIfIndex ());
	congaRoutingHelper.GetCongaRouting (spine1->GetObject<Ipv4> ())->
		AddRoute (Ipv4Address ("10.1.2.0"), Ipv4Mask ("255.255.255.0"), netdevice_leaf0_spine1_2.Get (1)->GetIfIndex ());
    }
    else
    {
        NS_LOG_INFO ("Populate global routing tables");
        Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    }

    if (runMode == CONGA || runMode == CONGA_FLOW)
    {
        NS_LOG_INFO ("Setting up Conga switch");

        Ptr<Ipv4CongaRouting> congaLeaf0 = congaRoutingHelper.GetCongaRouting (leaf0->GetObject<Ipv4> ());
        congaLeaf0->SetLeafId (0);

        Ptr<Ipv4CongaRouting> congaLeaf1 = congaRoutingHelper.GetCongaRouting (leaf1->GetObject<Ipv4> ());
        congaLeaf1->SetLeafId (1);

        Ptr<Ipv4CongaRouting> congaSpine0 = congaRoutingHelper.GetCongaRouting (spine0->GetObject<Ipv4> ());
        Ptr<Ipv4CongaRouting> congaSpine1 = congaRoutingHelper.GetCongaRouting (spine1->GetObject<Ipv4> ());

        // T Dre / alpha(0.2) should be larger than network RTT
        congaLeaf0->SetTDre (MicroSeconds (30));
        congaLeaf1->SetTDre (MicroSeconds (30));
        congaSpine0->SetTDre (MicroSeconds (30));
        congaSpine1->SetTDre (MicroSeconds (30));

        congaLeaf0->SetAlpha (0.2);
        congaLeaf1->SetAlpha (0.2);
        congaSpine0->SetAlpha (0.2);
        congaSpine1->SetAlpha (0.2);

        congaLeaf0->SetLinkCapacity(DataRate(SPINE_LEAF_CAPACITY));
        congaLeaf1->SetLinkCapacity(DataRate(SPINE_LEAF_CAPACITY));
        congaSpine0->SetLinkCapacity(DataRate(SPINE_LEAF_CAPACITY));
        congaSpine1->SetLinkCapacity(DataRate(SPINE_LEAF_CAPACITY));

        for (int i = 0; i < LEAF_NODE_COUNT; i++)
        {
            congaLeaf0->AddAddressToLeafIdMap (serversAddr0[i].first, 0);
            congaLeaf0->AddAddressToLeafIdMap (serversAddr1[i].first, 1);
            congaLeaf1->AddAddressToLeafIdMap (serversAddr0[i].first, 0);
            congaLeaf1->AddAddressToLeafIdMap (serversAddr1[i].first, 1);
        }
        if (runMode == CONGA)
        {
            congaLeaf0->SetFlowletTimeout (MicroSeconds (300));
            congaLeaf1->SetFlowletTimeout (MicroSeconds (300));
        }
        if (runMode == CONGA_FLOW)
        {
            congaLeaf0->SetFlowletTimeout (MilliSeconds (13));
            congaLeaf1->SetFlowletTimeout (MilliSeconds (13));
        }
        if (runMode == CONGA_ECMP)
        {
            congaLeaf0->EnableEcmpMode ();
            congaLeaf1->EnableEcmpMode ();
            congaSpine0->EnableEcmpMode ();
            congaSpine1->EnableEcmpMode ();
        }
    }

    if (runMode == PRESTO)
    {
        NS_LOG_INFO ("Setting up DRB switch");
        Ipv4DrbHelper drb;
        Ptr<Ipv4Drb> drb0 = drb.GetIpv4Drb (leaf0->GetObject<Ipv4> ());
        drb0->AddCoreSwitchAddress (PRESTO_RATIO, addr_leaf0_spine0_1.GetAddress (1));
        drb0->AddCoreSwitchAddress (PRESTO_RATIO, addr_leaf0_spine0_2.GetAddress (1));
        drb0->AddCoreSwitchAddress (PRESTO_RATIO, addr_leaf0_spine1_1.GetAddress (1));
        drb0->AddCoreSwitchAddress (PRESTO_RATIO, addr_leaf0_spine1_2.GetAddress (1));

        Ptr<Ipv4Drb> drb1 = drb.GetIpv4Drb (leaf1->GetObject<Ipv4> ());
        drb1->AddCoreSwitchAddress (PRESTO_RATIO, addr_leaf1_spine0_1.GetAddress (1));
        drb1->AddCoreSwitchAddress (PRESTO_RATIO, addr_leaf1_spine0_2.GetAddress (1));
        drb1->AddCoreSwitchAddress (PRESTO_RATIO, addr_leaf1_spine1_1.GetAddress (1));
        if (asym == false)
        {
            drb1->AddCoreSwitchAddress (PRESTO_RATIO, addr_leaf1_spine1_2.GetAddress (1));
        }
    }

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

    // Install apps on servers under switch leaf0
    install_applications(servers0, servers1, requestRate, cdfTable, serversAddr1, flowCount);
    install_applications(servers1, servers0, requestRate, cdfTable, serversAddr0, flowCount);

    NS_LOG_INFO ("Total flow: " << flowCount);

    NS_LOG_INFO ("Enabling flow monitor");

    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();


    // XXX Link Monitor Test Code Starts
    Ptr<LinkMonitor> linkMonitor = Create<LinkMonitor> ();
    Ptr<Ipv4LinkProbe> linkProbe = Create<Ipv4LinkProbe> (leaf0, linkMonitor);
    linkProbe->SetProbeName ("Leaf 0");
    linkProbe->SetCheckTime (Seconds (0.01));
    linkProbe->SetDataRateAll (DataRate (SPINE_LEAF_CAPACITY));
    linkMonitor->Start (Seconds (START_TIME));
    linkMonitor->Stop (Seconds (END_TIME));
    // End

    NS_LOG_INFO ("Start simulation");
    Simulator::Stop (Seconds (END_TIME));
    Simulator::Run ();

    flowMonitor->CheckForLostPackets ();

    std::stringstream fileName;

    fileName << "8-6-load-" << load <<"-";

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
    else if (runMode == PRESTO)
    {
        fileName << "presto-simulation-";
    }
    else if (runMode == ECMP)
    {
        fileName << "ecmp-simulation-";
    }

    fileName <<randomSeed << "-";

    if (asym == true)
    {
        fileName <<"asym.xml";
    }
    else
    {
        fileName << "sym.xml";
    }

    flowMonitor->SerializeToXmlFile(fileName.str (), true, true);
    linkMonitor->OutputToFile ("link-monitor-out.txt", &LinkMonitor::DefaultFormat);

    Simulator::Destroy ();
    free_cdf (cdfTable);
    NS_LOG_INFO ("Stop simulation");
}
