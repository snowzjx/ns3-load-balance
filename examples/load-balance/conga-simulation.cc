#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-conga-helper.h"

#include <vector>

// There are 32 servers connecting to each leaf switch
#define LEAF_NODE_COUNT 8

// The simulation starting and ending time
#define START_TIME 0.1
#define END_TIME 2.0

// The flow port range, each flow will be assigned a random port number within this range
#define PORT_START 2333
#define PORT_END 6666

// Request rate per second
#define REQUEST_RATE 2

// Flow configuration
//
//       +--------------------+--------------------+--------------------+
//       |   Small flow       |   Medium flow      |    Large flow      |
//       +--------------------+--------------------+--------------------+
//
// Size: 0            small_flow_max        large_flow_min      large_flow_max
// Poss:  small_flow_poss  1-small_flow_poss-large_flow_poss  large_flow_poss

#define SMALL_FLOW_MAX 100000       // 100KB
#define LARGE_FLOW_MIN 10000000     // 10MB
#define LARGE_FLOW_MAX 100000000    // 100MB

#define SMALL_FLOW_POSS 0.8         // 80%
#define LARGE_FLOW_POSS 0.05        // 5%

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CongaSimulation");

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

void install_applications (NodeContainer fromServers, NodeContainer destServers, std::vector<Ipv4Address> toAddresses)
{
    for (int i = 0; i < LEAF_NODE_COUNT; i++)
    {
        double startTime = START_TIME + poission_gen_interval (REQUEST_RATE);
        while (startTime < END_TIME)
        {
            uint16_t port = rand_range (PORT_START, PORT_END);
            int destIndex = rand_range (0, LEAF_NODE_COUNT - 1);
            BulkSendHelper source ("ns3::TcpSocketFactory",
                    InetSocketAddress (toAddresses[destIndex], port));

            uint32_t flowSize = 0;

            double randomValue = rand_range (0.0, 1.0);
            if (randomValue <= SMALL_FLOW_POSS)
            {
                // small flow
                flowSize = rand_range (0, SMALL_FLOW_MAX);
            }
            else if (randomValue > SMALL_FLOW_POSS && randomValue <= 1 - LARGE_FLOW_POSS)
            {
                // medium flow
                flowSize = rand_range (SMALL_FLOW_MAX, LARGE_FLOW_MIN);
            }
            else
            {
                // large flow
                flowSize = rand_range (LARGE_FLOW_MIN, LARGE_FLOW_MAX);
            }
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

            startTime += poission_gen_interval (REQUEST_RATE);
        }
    }
}

int main (int argc, char *argv[])
{
#if 1
    LogComponentEnable ("CongaSimulation", LOG_LEVEL_INFO);
#endif

    NS_LOG_INFO ("Declare data structures");
    std::vector<Ipv4Address> serversAddr0 = std::vector<Ipv4Address> (LEAF_NODE_COUNT);
    std::vector<Ipv4Address> serversAddr1 = std::vector<Ipv4Address> (LEAF_NODE_COUNT);

    NS_LOG_INFO ("Config parameters");
    Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue (Seconds (0.01)));
    Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (100000000));
    Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (100000000));

    NS_LOG_INFO ("Initialize random seed");
    srand((unsigned)time (NULL));

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

    // Enable Conga switch
    Config::SetDefault ("ns3::Ipv4GlobalRouting::CongaRouting", BooleanValue (true));
    internet.Install (leaf0);
    internet.Install (leaf1);
    internet.Install (spine0);
    internet.Install (spine1);

    NS_LOG_INFO ("Install channels and assign addresses");

    PointToPointHelper p2p;
    Ipv4AddressHelper ipv4;

    // Setting switches
    p2p.SetDeviceAttribute ("DataRate", StringValue ("40Gbps"));
    p2p.SetChannelAttribute ("Delay", TimeValue(MicroSeconds (100)));
    p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (100));

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
    ipv4.Assign (netdevice_leaf0_spine0_1);
    ipv4.Assign (netdevice_leaf0_spine0_2);

    ipv4.Assign (netdevice_leaf0_spine1_1);
    ipv4.Assign (netdevice_leaf0_spine1_2);

    ipv4.Assign (netdevice_leaf1_spine0_1);
    ipv4.Assign (netdevice_leaf1_spine0_2);

    ipv4.Assign (netdevice_leaf1_spine1_1);
    ipv4.Assign (netdevice_leaf1_spine1_2);

    // Setting servers under leaf 0
    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));

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

    NS_LOG_INFO ("Create applications");

    // Install apps on servers under switch leaf0
    install_applications(servers0, servers1, serversAddr1);
    install_applications(servers1, servers0, serversAddr0);

    NS_LOG_INFO ("Enabling flow monitor");

    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    NS_LOG_INFO ("Start simulation");
    Simulator::Stop (Seconds (END_TIME));
    Simulator::Run ();
    Simulator::Destroy ();

    flowMonitor->SerializeToXmlFile("conga-simulation.xml", true, true);
}
