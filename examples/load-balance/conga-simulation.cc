#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-conga-helper.h"
#include "ns3/ipv4-drb-helper.h"

#include <vector>

// The CDF in TrafficGenerator
extern "C"
{
#include "cdf.h"
}

// There are 8 servers connecting to each leaf switch
#define LEAF_NODE_COUNT 8

#define SPINE_LEAF_CAPACITY  4000000000           // 4Gbps
#define LEAF_SERVER_CAPACITY 4000000000           // 4Gbps
#define LINK_LATENCY MicroSeconds(100)            // 100 MicroSeconds
#define BUFFER_SIZE 100                           // 100 Packets

// The simulation starting and ending time
#define START_TIME 0.0
#define END_TIME 20.0

#define FLOW_LAUNCH_END_TIME 0.01

// The flow port range, each flow will be assigned a random port number within this range
#define PORT_START 10000
#define PORT_END 20000

#define PRESTO_RATIO 64

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CongaSimulation");

enum RunMode {
    CONGA,
    CONGA_FLOW,
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
        struct cdf_table *cdfTable, std::vector<Ipv4Address> toAddresses)
{
    NS_LOG_INFO ("Install applications:");
    for (int i = 0; i < LEAF_NODE_COUNT; i++)
    {
        double startTime = START_TIME + poission_gen_interval (requestRate);
        while (startTime < FLOW_LAUNCH_END_TIME)
        {
            uint16_t port = rand_range (PORT_START, PORT_END);
            int destIndex = rand_range (0, LEAF_NODE_COUNT - 1);
            BulkSendHelper source ("ns3::TcpSocketFactory",
                    InetSocketAddress (toAddresses[destIndex], port));

            uint32_t flowSize = gen_random_cdf (cdfTable);

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
                    << flowSize << "[start time: " << startTime <<" ]");

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

    CommandLine cmd;
    cmd.AddValue ("runMode", "Running mode of this simulation: Conga, Conga-flow, Presto, ECMP", runModeStr);
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
        NS_LOG_ERROR ("The running mode should be Conga, Conga-flow, Presto and ECMP");
        return 0;
    }

    if (load < 0.0 || load > 1.0)
    {
        NS_LOG_ERROR ("The network load should within 0.0 and 1.0");
        return 0;
    }

    NS_LOG_INFO ("Declare data structures");
    std::vector<Ipv4Address> serversAddr0 = std::vector<Ipv4Address> (LEAF_NODE_COUNT);
    std::vector<Ipv4Address> serversAddr1 = std::vector<Ipv4Address> (LEAF_NODE_COUNT);

    NS_LOG_INFO ("Config parameters");
    Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue (Seconds (0.01)));
    Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (100000000));
    Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (100000000));

    NS_LOG_INFO ("Initialize CDF table");
    struct cdf_table* cdfTable = new cdf_table ();
    init_cdf (cdfTable);
    load_cdf (cdfTable, cdfFileName.c_str ());

    NS_LOG_INFO ("Calculating request rate");
    double requestRate = load * SPINE_LEAF_CAPACITY  / avg_cdf (cdfTable);
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

    internet.Install (servers0);
    internet.Install (servers1);

    // Enable Conga or per flow ECMP switch
    if (runMode == CONGA || runMode == CONGA_FLOW)
    {
        Config::SetDefault ("ns3::Ipv4GlobalRouting::CongaRouting", BooleanValue (true));
    }
    else
    {
        Config::SetDefault ("ns3::Ipv4GlobalRouting::PerflowEcmpRouting", BooleanValue(true));
    }

    internet.Install (spine0);
    internet.Install (spine1);

    // Enable DRB
    if (runMode == PRESTO)
    {
        internet.SetDrb (true);
    }

    internet.Install (leaf0);
    internet.Install (leaf1);

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
    NodeContainer leaf1_spine1_2 = NodeContainer (leaf1, spine1);

    NetDeviceContainer netdevice_leaf0_spine0_1 = p2p.Install (leaf0_spine0_1);
    NetDeviceContainer netdevice_leaf0_spine0_2 = p2p.Install (leaf0_spine0_2);

    NetDeviceContainer netdevice_leaf0_spine1_1 = p2p.Install (leaf0_spine1_1);
    NetDeviceContainer netdevice_leaf0_spine1_2 = p2p.Install (leaf0_spine1_2);

    NetDeviceContainer netdevice_leaf1_spine0_1 = p2p.Install (leaf1_spine0_1);
    NetDeviceContainer netdevice_leaf1_spine0_2 = p2p.Install (leaf1_spine0_2);

    NetDeviceContainer netdevice_leaf1_spine1_1 = p2p.Install (leaf1_spine1_1);
    NetDeviceContainer netdevice_leaf1_spine1_2 = p2p.Install (leaf1_spine1_2);

    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer addr_leaf0_spine0_1 = ipv4.Assign (netdevice_leaf0_spine0_1);
    Ipv4InterfaceContainer addr_leaf0_spine0_2 = ipv4.Assign (netdevice_leaf0_spine0_2);

    Ipv4InterfaceContainer addr_leaf0_spine1_1 = ipv4.Assign (netdevice_leaf0_spine1_1);
    Ipv4InterfaceContainer addr_leaf0_spine1_2 = ipv4.Assign (netdevice_leaf0_spine1_2);

    Ipv4InterfaceContainer addr_leaf1_spine0_1 = ipv4.Assign (netdevice_leaf1_spine0_1);
    Ipv4InterfaceContainer addr_leaf1_spine0_2 = ipv4.Assign (netdevice_leaf1_spine0_2);

    Ipv4InterfaceContainer addr_leaf1_spine1_1 = ipv4.Assign (netdevice_leaf1_spine1_1);
    Ipv4InterfaceContainer addr_leaf1_spine1_2 = ipv4.Assign (netdevice_leaf1_spine1_2);

    // Setting servers under leaf 0
    p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (LEAF_SERVER_CAPACITY)));

    ipv4.SetBase ("10.1.2.0", "255.255.255.0");
    for (int i = 0; i < LEAF_NODE_COUNT; i++)
    {
        NodeContainer nodeContainer = NodeContainer (leaf0, servers0.Get (i));
        NetDeviceContainer netDeviceContainer = p2p.Install (nodeContainer);
        Ipv4InterfaceContainer interfaceContainer = ipv4.Assign (netDeviceContainer);
        serversAddr0[i] = interfaceContainer.GetAddress (1);
    }

    // Setting servers under leaf 1
    ipv4.SetBase ("10.1.3.0", "255.255.255.0");
    for (int i = 0; i < LEAF_NODE_COUNT; i++)
    {
        NodeContainer nodeContainer = NodeContainer (leaf1, servers1.Get (i));
        NetDeviceContainer netDeviceContainer = p2p.Install (nodeContainer);
        Ipv4InterfaceContainer interfaceContainer = ipv4.Assign (netDeviceContainer);
        serversAddr1[i] = interfaceContainer.GetAddress (1);
    }

    NS_LOG_INFO ("Ip addresses in servers 0: ");
    for (int i = 0; i < LEAF_NODE_COUNT; i++)
    {
       NS_LOG_INFO (serversAddr0[i]);
    }

    NS_LOG_INFO ("Ip addresses in servers 1: ");
    for (int i = 0; i < LEAF_NODE_COUNT; i++)
    {
       NS_LOG_INFO (serversAddr1[i]);
    }

    NS_LOG_INFO ("Populate global routing tables");
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    if (runMode == CONGA || runMode == CONGA_FLOW)
    {
        NS_LOG_INFO ("Setting up Conga switch");
        Ipv4CongaHelper conga;

        Ptr<Ipv4Conga> conga0 = conga.GetIpv4Conga (leaf0->GetObject<Ipv4> ());
        conga0->SetLeafId (0);

        Ptr<Ipv4Conga> conga1 = conga.GetIpv4Conga (leaf1->GetObject<Ipv4> ());
        conga1->SetLeafId (1);

        for (int i = 0; i < LEAF_NODE_COUNT; i++)
        {
            conga0->AddAddressToLeafIdMap (serversAddr0[i], 0);
            conga0->AddAddressToLeafIdMap (serversAddr1[i], 1);
            conga1->AddAddressToLeafIdMap (serversAddr0[i], 0);
            conga1->AddAddressToLeafIdMap (serversAddr1[i], 1);
        }
        if (runMode == CONGA)
        {
            conga0->SetFlowletTimeout (MicroSeconds (500));
            conga1->SetFlowletTimeout (MicroSeconds (500));
        }
        if (runMode == CONGA_FLOW)
        {
            conga0->SetFlowletTimeout (MilliSeconds (13));
            conga1->SetFlowletTimeout (MilliSeconds (13));
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
        drb1->AddCoreSwitchAddress (PRESTO_RATIO, addr_leaf1_spine1_2.GetAddress (1));
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

    // Install apps on servers under switch leaf0
    install_applications(servers0, servers1, requestRate, cdfTable, serversAddr1);
    install_applications(servers1, servers0, requestRate, cdfTable, serversAddr0);

    NS_LOG_INFO ("Enabling flow monitor");

    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    NS_LOG_INFO ("Start simulation");
    Simulator::Stop (Seconds (END_TIME));
    Simulator::Run ();
    Simulator::Destroy ();

    std::stringstream fileName;

    fileName << "load-" << load <<"-";

    if (runMode == CONGA)
    {
        fileName << "conga-simulation.xml";
    }
    else if (runMode == CONGA_FLOW)
    {
        fileName << "conga-flow-simulation.xml";
    }
    else if (runMode == PRESTO)
    {
        fileName << "presto-simulation.xml";
    }
    else if (runMode == ECMP)
    {
        fileName << "ecmp-simulation.xml";
    }

    flowMonitor->SerializeToXmlFile(fileName.str (), true, true);

    free_cdf (cdfTable);
}
