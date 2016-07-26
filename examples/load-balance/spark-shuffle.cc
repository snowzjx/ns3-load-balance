#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-conga-helper.h"
#include "ns3/ipv4-drb-helper.h"

#include <vector>

// There are 8 servers connecting to each leaf switch
#define SPINE_COUNT 3
#define LEAF_COUNT 3
#define SERVER_COUNT 8

#define SPINE_LEAF_CAPACITY  1000000000           // 1Gbps
#define LEAF_SERVER_CAPACITY 1000000000           // 1Gbps
#define LINK_LATENCY MicroSeconds(100)            // 150 MicroSeconds
#define BUFFER_SIZE 100                           // 100 Packets

#define FLOW_SIZE 100000000                       // 100MB

// The simulation starting and ending time
#define START_TIME 0.0
#define END_TIME 0.5

// The flow port range, each flow will be assigned a random port number within this range
#define PORT_START 2333
#define PORT_END 6666

#define PRESTO_RATIO 64

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CongaSimulation");

enum RunMode {
    CONGA,
    CONGA_FLOW,
    PRESTO,
    ECMP
};

template<typename T>
T rand_range (T min, T max)
{
    return min + ((double)max - min) * rand () / RAND_MAX;
}

void install_applications (int fromLeafId, int toLeafId, NodeContainer servers, std::vector<Ipv4Address> serversAddr)
{
    int fromStartServerIndex = fromLeafId * SERVER_COUNT;
    int fromEndServerIndex = (fromLeafId + 1) * SERVER_COUNT - 1;

    int toStartServerIndex = toLeafId * SERVER_COUNT;
    int toEndServerIndex = (toLeafId + 1) * SERVER_COUNT - 1;

    for (int fromServerIndex = fromStartServerIndex; fromServerIndex <= fromEndServerIndex; fromServerIndex++)
    {
        NS_LOG_INFO ("Install application for server: " << fromServerIndex);
        for (int toServerIndex = toStartServerIndex; toServerIndex <= toEndServerIndex; toServerIndex++)
        {
            double flowStartTime = START_TIME + rand_range (0.0, 0.01);
            uint16_t port = rand_range (PORT_START, PORT_END);

            BulkSendHelper source ("ns3::TcpSocketFactory",
                    InetSocketAddress (serversAddr[toServerIndex], port));

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

            NS_LOG_INFO("\tFlow to:" << serversAddr[toServerIndex] <<" on port: " << port << " starts from: " << flowStartTime);
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
    std::vector<Ipv4Address> serversAddr = std::vector<Ipv4Address> (LEAF_COUNT * SERVER_COUNT);
    std::vector<std::vector<Ipv4Address> > leafSpinesAddrMap = std::vector<std::vector<Ipv4Address> > (LEAF_COUNT);
    for (int i = 0; i < LEAF_COUNT; i++)
    {
        leafSpinesAddrMap[i] = std::vector<Ipv4Address> (SPINE_COUNT);
    }

    NS_LOG_INFO ("Config parameters");
    Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue (Seconds (0.01)));
    Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (100000000));
    Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (100000000));

    NS_LOG_INFO ("Initialize random seed");
    if (randomSeed == 0)
    {
        srand ((unsigned)time (NULL));
    }
    else
    {
        srand (randomSeed);
    }

    NS_LOG_INFO ("Create nodes");

    NodeContainer spines;
    spines.Create (SPINE_COUNT);

    NodeContainer leaves;
    leaves.Create (LEAF_COUNT);

    NodeContainer servers;
    servers.Create (SERVER_COUNT * LEAF_COUNT);

    NS_LOG_INFO ("Install Internet stacks");

    InternetStackHelper internet;

    internet.Install (servers);

    // Enable Conga or per flow ECMP switch
    if (runMode == CONGA || runMode == CONGA_FLOW)
    {
        Config::SetDefault ("ns3::Ipv4GlobalRouting::CongaRouting", BooleanValue (true));
    }
    else
    {
        Config::SetDefault ("ns3::Ipv4GlobalRouting::PerflowEcmpRouting", BooleanValue(true));
    }

    internet.Install (spines);

    // Enable DRB
    if (runMode == PRESTO)
    {
        internet.SetDrb (true);
    }

    internet.Install (leaves);

    NS_LOG_INFO ("Install channels and assign addresses");

    PointToPointHelper p2p;
    Ipv4AddressHelper ipv4;

    // Setting switches
    p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (SPINE_LEAF_CAPACITY)));
    p2p.SetChannelAttribute ("Delay", TimeValue(LINK_LATENCY));
    p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (BUFFER_SIZE));

    ipv4.SetBase ("10.1.1.0", "255.255.255.0");

    // Spine Leaf switches are connected in a full mesh manner
    for (int i = 0; i < SPINE_COUNT; i++)
    {
        for (int j = 0; j < LEAF_COUNT; j++)
        {
            NodeContainer nodeContainer = NodeContainer (spines.Get (i), leaves.Get (j));
            NetDeviceContainer netDeviceContainer = p2p.Install (nodeContainer);
            Ipv4InterfaceContainer ipv4InterfaceContainer = ipv4.Assign (netDeviceContainer);
            leafSpinesAddrMap[j][i] = ipv4InterfaceContainer.GetAddress (0);
        }
        ipv4.NewNetwork ();
    }

    // Setting servers
    p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (LEAF_SERVER_CAPACITY)));

    for (int i = 0; i < LEAF_COUNT; i++)
    {
        for (int j = 0; j < SERVER_COUNT; j++)
        {
            int serverIndex = i * SERVER_COUNT + j;
            NodeContainer nodeContainer = NodeContainer (leaves.Get (i), servers.Get (serverIndex));
            NetDeviceContainer netDeviceContainer = p2p.Install (nodeContainer);
            Ipv4InterfaceContainer interfaceContainer = ipv4.Assign (netDeviceContainer);
            serversAddr[serverIndex] = interfaceContainer.GetAddress (1);
        }
        ipv4.NewNetwork ();
    }

    NS_LOG_INFO ("Ip addresses for servers: ");
    for (int i = 0; i < SERVER_COUNT * LEAF_COUNT; i++)
    {
       NS_LOG_INFO (serversAddr[i]);
    }

    NS_LOG_INFO ("Populate global routing tables");
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    if (runMode == CONGA || runMode == CONGA_FLOW)
    {
        NS_LOG_INFO ("Setting up Conga switch");
        Ipv4CongaHelper conga;
        std::vector<uint32_t> congestionVector (SPINE_COUNT, 0);
        congestionVector[1] = 5;
        congestionVector[2] = 5;

        for (int i = 0; i < LEAF_COUNT; i++)
        {
            Ptr<Ipv4Conga> congaNode = conga.GetIpv4Conga (leaves.Get (i)->GetObject<Ipv4> ());
            congaNode->SetLeafId (i);

            for (int j = 0; j <LEAF_COUNT; j++)
            {
                for (int k = 0; k < SERVER_COUNT; k++)
                {
                    int serverIndex = j * SERVER_COUNT + k;
                    congaNode->AddAddressToLeafIdMap (serversAddr[serverIndex], j);
                }
            }
            if (runMode == CONGA)
            {
                congaNode->SetFlowletTimeout (MicroSeconds (500));
            }
            if (runMode == CONGA_FLOW)
            {
                congaNode->SetFlowletTimeout (MilliSeconds (13));
            }
            congaNode->InsertCongaToLeafTable(2, congestionVector);
        }
    }

    if (runMode == PRESTO)
    {
        NS_LOG_INFO ("Setting up DRB switch");
        Ipv4DrbHelper drb;

        for (int i = 0; i < LEAF_COUNT; i++)
        {
            NS_LOG_INFO ("The bouncing server for Leaf: " << i << " is: ");
            Ptr<Ipv4Drb> drbNode = drb.GetIpv4Drb (leaves.Get (i)->GetObject<Ipv4> ());
            for (int j=0; j < SPINE_COUNT; j++)
            {
               drbNode->AddCoreSwitchAddress (PRESTO_RATIO, leafSpinesAddrMap[i][j]);
               NS_LOG_INFO ("\t" << leafSpinesAddrMap[i][j]);
            }
        }
    }

    NS_LOG_INFO ("Create applications");

    // Install apps on servers under leaf switches
    install_applications (0, 2, servers, serversAddr);
    install_applications (1, 2, servers, serversAddr);

    NS_LOG_INFO ("Enabling flow monitor");

    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    NS_LOG_INFO ("Start simulation");
    Simulator::Stop (Seconds (END_TIME));
    Simulator::Run ();
    Simulator::Destroy ();

    std::string fileName = "default.xml";

    if (runMode == CONGA)
    {
        fileName = "conga-simulation.xml";
    }
    else if (runMode == CONGA_FLOW)
    {
        fileName = "conga-flow-simulation.xml";
    }
    else if (runMode == PRESTO)
    {
        fileName = "presto-simulation.xml";
    }
    else if (runMode == ECMP)
    {
        fileName = "ecmp-simulation.xml";
    }

    flowMonitor->SerializeToXmlFile(fileName, true, true);
}
