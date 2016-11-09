#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-xpath-routing-helper.h"
#include "ns3/ipv4-tlb.h"
#include "ns3/ipv4-tlb-probing.h"
#include "ns3/ipv4-drb-routing-helper.h"

#include <map>
#include <utility>

extern "C"
{
#include "cdf.h"
}

#define LINK_CAPACITY_BASE    1000000000         // 1Gbps
#define LINK_DELAY  MicroSeconds(10)             // 10 MicroSeconds
#define BUFFER_SIZE 600                          // 600 packets
#define PACKET_SIZE 1400

#define RED_QUEUE_MARKING 65 		        	 // 65 Packets (available only in DcTcp)

#define PORT_START 10000
#define PORT_END 50000


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FattreeSimulation");

enum RunMode {
    TLB,
    ECMP,
    DRB,
    PRESTO
};

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

void install_applications (uint32_t fromPodId, uint32_t serverCount, uint32_t k, NodeContainer servers, double requestRate, struct cdf_table *cdfTable,
        long &flowCount, long &totalFlowSize, double START_TIME, double END_TIME, double FLOW_LAUNCH_END_TIME)
{
    NS_LOG_INFO ("Install applications:");
    for (uint32_t i = 0; i < serverCount * (k / 2); i++)
    {
        uint32_t fromServerIndex = fromPodId * serverCount * (k / 2) + i;

        double startTime = START_TIME + poission_gen_interval (requestRate);
        while (startTime < FLOW_LAUNCH_END_TIME)
        {
            flowCount ++;
            uint16_t port = rand_range (PORT_START, PORT_END);

            uint32_t destServerIndex = fromServerIndex;
            while (destServerIndex >= fromPodId * serverCount * (k / 2)
                    && destServerIndex < (fromPodId + 1) * serverCount * (k / 2))

            {
                destServerIndex = rand_range (0u, serverCount * (k / 2) * k);
            }

	        Ptr<Node> destServer = servers.Get (destServerIndex);
	        Ptr<Ipv4> ipv4 = destServer->GetObject<Ipv4> ();
	        Ipv4InterfaceAddress destInterface = ipv4->GetAddress (1, 0);
	        Ipv4Address destAddress = destInterface.GetLocal ();

            BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (destAddress, port));
            uint32_t flowSize = gen_random_cdf (cdfTable);

            totalFlowSize += flowSize;
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

            //NS_LOG_INFO ("\tFlow from server: " << fromServerIndex << " to server: "
                    //<< destServerIndex << " on port: " << port << " with flow size: "
                    //<< flowSize << " [start time: " << startTime <<"]");

            startTime += poission_gen_interval (requestRate);
        }
    }
}

int main (int argc, char *argv[])
{

#if 1
    LogComponentEnable ("FattreeSimulation", LOG_LEVEL_INFO);
#endif

    std::string id = "0";
    std::string runModeStr = "ECMP";
    unsigned randomSeed = 0;
    std::string cdfFileName = "examples/load-balance/VL2_CDF.txt";
    double load = 0.5;

    // The simulation starting and ending time
    double START_TIME = 0.0;
    double END_TIME = 0.5;

    double FLOW_LAUNCH_END_TIME = 0.2;

    uint32_t k = 4;
    uint32_t serverCount = 8;

    bool dctcpEnabled = true;

    uint32_t TLBMinRTT = 40;
    uint32_t TLBHighRTT = 180;
    uint32_t TLBPoss = 50;
    uint32_t TLBBetterPathRTT = 1;
    uint32_t TLBT1 = 100;
    double TLBECNPortionLow = 0.1;
    uint32_t TLBRunMode = 0;
    bool TLBProbingEnable = true;
    uint32_t TLBProbingInterval = 50;
    bool TLBSmooth = true;
    bool TLBRerouting = true;
    uint32_t TLBDREMultiply = 5;
    uint32_t TLBS = 64000;

    bool resequenceBuffer = false;

    uint32_t PRESTO_RATIO = 64;

    CommandLine cmd;
    cmd.AddValue ("ID", "Running ID", id);
    cmd.AddValue ("StartTime", "Start time of the simulation", START_TIME);
    cmd.AddValue ("EndTime", "End time of the simulation", END_TIME);
    cmd.AddValue ("FlowLaunchEndTime", "End time of the flow launch period", FLOW_LAUNCH_END_TIME);
    cmd.AddValue ("runMode", "Running mode of this simulation: ECMP, Presto and TLB", runModeStr);
    cmd.AddValue ("randomSeed", "Random seed, 0 for random generated", randomSeed);
    cmd.AddValue ("cdfFileName", "File name for flow distribution", cdfFileName);
    cmd.AddValue ("load", "Load of the network, 0.0 - 1.0", load);
    cmd.AddValue ("enableDcTcp", "Whether to enable DCTCP", dctcpEnabled);

    cmd.AddValue ("TLBMinRTT", "TLBMinRTT", TLBMinRTT);
    cmd.AddValue ("TLBHighRTT", "TLBHighRTT", TLBHighRTT);
    cmd.AddValue ("TLBPoss", "TLBPoss", TLBPoss);
    cmd.AddValue ("TLBBetterPathRTT", "TLBBetterPathRTT", TLBBetterPathRTT);
    cmd.AddValue ("TLBT1", "TLBT1", TLBT1);
    cmd.AddValue ("TLBECNPortionLow", "TLBECNPortionLow", TLBECNPortionLow);
    cmd.AddValue ("TLBRunMode", "TLBRunMode", TLBRunMode);
    cmd.AddValue ("TLBProbingEnable", "TLBProbingEnable", TLBProbingEnable);
    cmd.AddValue ("TLBProbingInterval", "TLBProbingInterval", TLBProbingInterval);
    cmd.AddValue ("TLBSmooth", "TLBSmooth", TLBSmooth);
    cmd.AddValue ("TLBRerouting", "TLBRerouting", TLBRerouting);
    cmd.AddValue ("TLBDREMultiply", "TLBDREMultiply", TLBDREMultiply);
    cmd.AddValue ("TLBS", "TLBS", TLBS);
    cmd.AddValue ("PrestoK" , "PrestoK", PRESTO_RATIO);
    cmd.AddValue ("resequenceBuffer", "ResequenceBuffer", resequenceBuffer);

    cmd.Parse (argc, argv);

    uint64_t serverEdgeCapacity = 10ul * LINK_CAPACITY_BASE;
    uint64_t edgeAggregationCapacity = 10ul * LINK_CAPACITY_BASE;
    uint64_t aggregationCoreCapacity = 10ul * LINK_CAPACITY_BASE;

    RunMode runMode;
    if (runModeStr.compare ("ECMP") == 0)
    {
        runMode = ECMP;
    }
    else if (runModeStr.compare ("TLB") == 0)
    {
        std::cout << Ipv4TLB::GetLogo () << std::endl;
        runMode = TLB;
    }
    else if (runModeStr.compare ("DRB") == 0)
    {
        runMode = DRB;
    }
    else if (runModeStr.compare ("Presto") == 0)
    {
        runMode = PRESTO;
    }
    else
    {
        NS_LOG_ERROR ("The running mode should be ECMP, Presto, DRB and TLB");
        return 0;
    }

    if (load < 0.0 || load >= 1.0)
    {
        NS_LOG_ERROR ("The network load should within 0.0 and 1.0");
        return 0;
    }

    NS_LOG_INFO ("Config parameters");
    if (dctcpEnabled)
    {
	    NS_LOG_INFO ("Enabling DcTcp");
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpDCTCP::GetTypeId ()));
        Config::SetDefault ("ns3::RedQueueDisc::Mode", StringValue ("QUEUE_MODE_BYTES"));
    	Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (PACKET_SIZE));
        Config::SetDefault ("ns3::RedQueueDisc::QueueLimit", UintegerValue (BUFFER_SIZE * PACKET_SIZE));
        //Config::SetDefault ("ns3::QueueDisc::Quota", UintegerValue (BUFFER_SIZE));
        Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (false));
    }

    if (runMode == TLB)
    {
        NS_LOG_INFO ("Enabling TLB");
        Config::SetDefault ("ns3::TcpSocketBase::TLB", BooleanValue (true));
        Config::SetDefault ("ns3::Ipv4TLB::MinRTT", TimeValue (MicroSeconds (TLBMinRTT)));
        Config::SetDefault ("ns3::Ipv4TLB::HighRTT", TimeValue (MicroSeconds (TLBHighRTT)));
        Config::SetDefault ("ns3::Ipv4TLB::BetterPathRTTThresh", TimeValue (MicroSeconds (TLBBetterPathRTT)));
        Config::SetDefault ("ns3::Ipv4TLB::ChangePathPoss", UintegerValue (TLBPoss));
        Config::SetDefault ("ns3::Ipv4TLB::T1", TimeValue (MicroSeconds (TLBT1)));
        Config::SetDefault ("ns3::Ipv4TLB::ECNPortionLow", DoubleValue (TLBECNPortionLow));
        Config::SetDefault ("ns3::Ipv4TLB::RunMode", UintegerValue (TLBRunMode));
        Config::SetDefault ("ns3::Ipv4TLBProbing::ProbeInterval", TimeValue (MicroSeconds (TLBProbingInterval)));
        Config::SetDefault ("ns3::Ipv4TLB::IsSmooth", BooleanValue (TLBSmooth));
        Config::SetDefault ("ns3::Ipv4TLB::Rerouting", BooleanValue (TLBRerouting));
        Config::SetDefault ("ns3::Ipv4TLB::DREMultiply", UintegerValue (TLBDREMultiply));
        Config::SetDefault ("ns3::Ipv4TLB::S", UintegerValue(TLBS));
    }

    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue(PACKET_SIZE));
    Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (0));
    Config::SetDefault ("ns3::TcpSocket::ConnTimeout", TimeValue (MilliSeconds (5)));
    Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (10));
    Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue (MilliSeconds (5)));
    Config::SetDefault ("ns3::TcpSocketBase::ClockGranularity", TimeValue (MicroSeconds (100)));
    Config::SetDefault ("ns3::RttEstimator::InitialEstimation", TimeValue (MicroSeconds (80)));
    Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (160000000));
    Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (160000000));


    Config::SetDefault ("ns3::RedQueueDisc::Mode", StringValue ("QUEUE_MODE_BYTES"));
    Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (PACKET_SIZE));
    Config::SetDefault ("ns3::RedQueueDisc::QueueLimit", UintegerValue (BUFFER_SIZE * PACKET_SIZE));
    Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (false));

    Config::SetDefault ("ns3::Ipv4GlobalRouting::PerflowEcmpRouting", BooleanValue (true));

    if (resequenceBuffer)
    {
        NS_LOG_INFO ("Enabling Resequence Buffer");
	    Config::SetDefault ("ns3::TcpSocketBase::ResequenceBuffer", BooleanValue (true));
        Config::SetDefault ("ns3::TcpResequenceBuffer::InOrderQueueTimerLimit", TimeValue (MicroSeconds (15)));
        Config::SetDefault ("ns3::TcpResequenceBuffer::SizeLimit", UintegerValue (100));
        Config::SetDefault ("ns3::TcpResequenceBuffer::OutOrderQueueTimerLimit", TimeValue (MicroSeconds (250)));
    }

    if (k % 2 != 0)
    {
        NS_LOG_ERROR ("The k should be 2^n");
        k -= (k % 2);
    }

    uint32_t edgeCount = k * (k / 2);
    uint32_t aggregationCount = k * (k / 2);
    uint32_t coreCount = (k / 2) * (k / 2);

    NodeContainer servers;
    NodeContainer edges;
    NodeContainer aggregations;
    NodeContainer cores;

    servers.Create (serverCount * edgeCount);
    edges.Create (edgeCount);
    aggregations.Create (aggregationCount);
    cores.Create (coreCount);

    InternetStackHelper internet;
    Ipv4GlobalRoutingHelper globalRoutingHelper;
    Ipv4ListRoutingHelper listRoutingHelper;
    Ipv4XPathRoutingHelper xpathRoutingHelper;
    Ipv4DrbRoutingHelper drbRoutingHelper;

    if (runMode == PRESTO || runMode == DRB)
    {
        if (runMode == DRB)
        {
            Config::SetDefault ("ns3::Ipv4DrbRouting::Mode", UintegerValue (0)); // Per dest
        }
        else
        {
            Config::SetDefault ("ns3::Ipv4DrbRouting::Mode", UintegerValue (1)); // Per flow
        }

        listRoutingHelper.Add (drbRoutingHelper, 1);
        listRoutingHelper.Add (globalRoutingHelper, 0);
        internet.SetRoutingHelper (listRoutingHelper);
        internet.Install (servers);

        listRoutingHelper.Clear ();
        listRoutingHelper.Add (xpathRoutingHelper, 1);
        listRoutingHelper.Add (globalRoutingHelper, 0);
        internet.SetRoutingHelper (listRoutingHelper);
        internet.Install (edges);
        internet.Install (aggregations);
        internet.Install (cores);
    }
    else if (runMode == TLB)
    {
        internet.SetTLB (true);
        internet.Install (servers);

        internet.SetTLB (false);
        listRoutingHelper.Add (xpathRoutingHelper, 1);
        listRoutingHelper.Add (globalRoutingHelper, 0);
        internet.SetRoutingHelper (listRoutingHelper);

        internet.Install (edges);
        internet.Install (aggregations);
        internet.Install (cores);
    }
    else if (runMode == ECMP)
    {
	    internet.SetRoutingHelper (globalRoutingHelper);

	    internet.Install (servers);
	    internet.Install (edges);
        internet.Install (aggregations);
        internet.Install (cores);
    }

    PointToPointHelper p2p;

    if (dctcpEnabled)
    {
        p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (10));
    }
    else
    {
        p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (BUFFER_SIZE));
    }

    Ipv4AddressHelper ipv4;

    ipv4.SetBase ("10.1.0.0", "255.255.255.0");

    TrafficControlHelper tc;

    if (dctcpEnabled)
    {
        tc.SetRootQueueDisc ("ns3::RedQueueDisc", "MinTh", DoubleValue (RED_QUEUE_MARKING * PACKET_SIZE),
                                                  "MaxTh", DoubleValue (RED_QUEUE_MARKING * PACKET_SIZE));
    }

    p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (serverEdgeCapacity)));
    p2p.SetChannelAttribute ("Delay", TimeValue (LINK_DELAY));

    std::map<std::pair<int, int>, uint32_t> edgeToAggregationPath;
    std::map<std::pair<int, int>, uint32_t> aggregationToCorePath;

    std::vector<Ipv4Address> serverAddresses (serverCount * edgeCount);
    std::vector<Ptr<Ipv4TLBProbing> > probings (serverCount * edgeCount);


    NS_LOG_INFO ("Connecting servers to edges");
    for (uint32_t i = 0; i < edgeCount; i++)
    {
        ipv4.NewNetwork ();
        for (uint32_t j = 0; j < serverCount; j++)
        {
            uint32_t uServerIndex = i * serverCount + j;

            NodeContainer nodeContainer = NodeContainer (edges.Get (i), servers.Get (uServerIndex));
            NetDeviceContainer netDeviceContainer = p2p.Install (nodeContainer);

            if (dctcpEnabled)
            {
                tc.Install (netDeviceContainer);
            }

            Ipv4InterfaceContainer interfaceContainer = ipv4.Assign (netDeviceContainer);

            serverAddresses[uServerIndex] = interfaceContainer.GetAddress (1);

            if (!dctcpEnabled)
            {
                tc.Uninstall (netDeviceContainer);
            }

            NS_LOG_INFO ("Server-" << uServerIndex << " is connected to Edge-" << i
                    << " (" << netDeviceContainer.Get (1)->GetIfIndex () << "<->"
                    << netDeviceContainer.Get (0)->GetIfIndex () << ")");

            if (runMode == TLB)
            {
                for (uint32_t l = 0; l < serverCount * edgeCount; ++l)
                {
                    Ptr<Ipv4TLB> tlb = servers.Get (l)->GetObject<Ipv4TLB> ();
                    tlb->AddAddressWithTor (interfaceContainer.GetAddress (1), i);
                    //NS_LOG_INFO ("Configuring TLB with " << l << "'s server, inserting server: " << uServerIndex << " under leaf: " << i);
                }

            }
        }
    }

    p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (edgeAggregationCapacity)));

    NS_LOG_INFO ("Connecting edges to aggregations");
    for (uint32_t i = 0; i < edgeCount; i++)
    {
        for (uint32_t j = 0; j < k / 2; j++)
        {
            uint32_t uAggregationIndex = (i / (k / 2)) * (k / 2) + j;

            NodeContainer nodeContainer = NodeContainer (edges.Get (i), aggregations.Get (uAggregationIndex));
            NetDeviceContainer netDeviceContainer = p2p.Install (nodeContainer);

            if (dctcpEnabled)
            {
                tc.Install (netDeviceContainer);
            }

            Ipv4InterfaceContainer interfaceContainer = ipv4.Assign (netDeviceContainer);

            if (!dctcpEnabled)
            {
                tc.Uninstall (netDeviceContainer);
            }

            std::pair<uint32_t, uint32_t> pathKey = std::make_pair (i, uAggregationIndex);
            edgeToAggregationPath[pathKey] = netDeviceContainer.Get (0)->GetIfIndex ();

            NS_LOG_INFO ("Edge-" << i << " is connected to Aggregation-" << uAggregationIndex
                    << " (" << netDeviceContainer.Get (0)->GetIfIndex () << "<->"
                    << netDeviceContainer.Get (1)->GetIfIndex () << ")");
        }
    }

    p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (aggregationCoreCapacity)));

    NS_LOG_INFO ("Connecting aggregations to cores");
    for (uint32_t i = 0; i < aggregationCount; i++)
    {
        for (uint32_t j = 0; j < k /2; j++)
        {
            uint32_t uCoreIndex = (i % (k / 2)) * (k / 2) + j;

            NodeContainer nodeContainer = NodeContainer (aggregations.Get (i), cores.Get (uCoreIndex));
            NetDeviceContainer netDeviceContainer = p2p.Install (nodeContainer);

            if (dctcpEnabled)
            {
                tc.Install (netDeviceContainer);
            }

            Ipv4InterfaceContainer interfaceContainer = ipv4.Assign (netDeviceContainer);

            if (!dctcpEnabled)
            {
                tc.Uninstall (netDeviceContainer);
            }

            std::pair<uint32_t, uint32_t> pathKey = std::make_pair (i, uCoreIndex);
            aggregationToCorePath[pathKey] = netDeviceContainer.Get (0)->GetIfIndex ();

            NS_LOG_INFO ("Aggregation-" << i << " is connected to Core-" << uCoreIndex
                    << " (" << netDeviceContainer.Get (0)->GetIfIndex () << "<->"
                    << netDeviceContainer.Get (1)->GetIfIndex () << ")");
        }
    }

    NS_LOG_INFO ("Populate global routing tables");
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    if (runMode == DRB || runMode == PRESTO)
    {
        NS_LOG_INFO ("Configuring DRB/PRESTO paths");
        for (uint32_t i = 0; i < edgeCount; i++)
        {
            for (uint32_t j = 0; j < serverCount; j++)
            {
                uint32_t uServerIndex = i * serverCount + j;
                for (uint32_t m = 0; m < k /2; m++)
                {
                    uint32_t uAggregationIndex = (i / (k / 2)) * (k / 2) + m;
                    int path = 0;
                    int pathBase = 1;
                    path += edgeToAggregationPath[std::make_pair (i, uAggregationIndex)] * pathBase;
                    pathBase *= 100;
                    for (uint32_t n = 0; n < k / 2; n++)
                    {
                        uint32_t uCoreIndex = m * (k / 2) + n;
                        int newPath = aggregationToCorePath[std::make_pair (uAggregationIndex, uCoreIndex)] * pathBase + path;
                        Ptr<Ipv4DrbRouting> drbRouting = drbRoutingHelper.GetDrbRouting (servers.Get (uServerIndex)->GetObject<Ipv4> ());
                        if (runMode == DRB)
                        {
                            drbRouting->AddPath (newPath);
                        }
                        else
                        {
                            drbRouting->AddPath (PRESTO_RATIO, newPath);
                        }
                        NS_LOG_INFO ("For server: " << uServerIndex << " under edge: " << i << ", configured with DRB/PRESTO path: " << newPath);
                    }
                }
            }

        }
    }

    if (runMode == TLB)
    {
        NS_LOG_INFO ("Configuring TLB available paths");
        for (uint32_t i = 0; i < edgeCount; i++)
        {
            for (uint32_t j = 0; j < serverCount; j++)
            {
                uint32_t uServerIndex = i * serverCount + j;
                for (uint32_t m = 0; m < k /2; m++)
                {
                    uint32_t uAggregationIndex = (i / (k / 2)) * (k / 2) + m;
                    int path = 0;
                    int pathBase = 1;
                    path += edgeToAggregationPath[std::make_pair (i, uAggregationIndex)] * pathBase;
                    pathBase *= 100;
                    for (uint32_t n = 0; n < k / 2; n++)
                    {
                        uint32_t uCoreIndex = m * (k / 2) + n;
                        int newPath = aggregationToCorePath[std::make_pair (uAggregationIndex, uCoreIndex)] * pathBase + path;
                        Ptr<Ipv4TLB> tlb = servers.Get (uServerIndex)->GetObject<Ipv4TLB> ();
                        for (uint32_t o = 0; o < edgeCount; o++)
                        {
                            if (o / (k / 2) != i / (k / 2))
                            {
                                tlb->AddAvailPath (o, newPath);
                                NS_LOG_INFO ("Configuring server: " << uServerIndex << " under leaf: " << i << " to leaf: " << o << " with path: " << newPath);
                            }
                        }
                    }
                }
            }
        }

        if (TLBProbingEnable)
        {
        NS_LOG_INFO ("Configuring TLB Probing");
        for (uint32_t i = 0; i < serverCount * edgeCount; i++)
        {
            // The i th server under one leaf is used to probe the leaf i by contacting the i th server under that leaf
            Ptr<Ipv4TLBProbing> probing = CreateObject<Ipv4TLBProbing> ();
            probings[i] = probing;
            probing->SetNode (servers.Get (i));
            probing->SetSourceAddress (serverAddresses[i]);
            probing->Init ();

            uint32_t serverIndexUnderEdge = i % serverCount;

            if (serverIndexUnderEdge < edgeCount)
            {
               uint32_t serverBeingProbed = serverCount * serverIndexUnderEdge;
                if (serverBeingProbed == i)
                {
                    continue;
                }
                probing->SetProbeAddress (serverAddresses[serverBeingProbed]);

                NS_LOG_INFO ("Server: " << i << " is going to probe server: " << serverBeingProbed);

                uint32_t edgeIndex = i / serverCount;
                for (uint32_t j = edgeIndex * serverCount; j < edgeIndex * serverCount + serverCount; j++)
                {
                    if (i == j)
                    {
                        continue;
                    }
                    probing->AddBroadCastAddress (serverAddresses[j]);
                    NS_LOG_INFO ("Server:" << i << " is going to broadcast to server: " << j);
                }
                probing->StartProbe ();
                probing->StopProbe (Seconds (END_TIME));
            }
        }
        }
    }

    double oversubRatio = static_cast<double> (serverCount * (k / 2) * k * serverEdgeCapacity) / (aggregationCoreCapacity * (k / 2) * aggregationCount);
    NS_LOG_INFO ("Over-subscription ratio: " << oversubRatio);

    NS_LOG_INFO ("Initialize CDF table");
    struct cdf_table* cdfTable = new cdf_table ();
    init_cdf (cdfTable);
    load_cdf (cdfTable, cdfFileName.c_str ());

    NS_LOG_INFO ("Calculating request rate");
    double requestRate = load * serverEdgeCapacity / oversubRatio / (8 * avg_cdf (cdfTable));
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

    long flowCount = 0;
    long totalFlowSize = 0;

    for (uint32_t fromPodId = 0; fromPodId < k; ++fromPodId)
    {
        install_applications (fromPodId, serverCount, k, servers, requestRate, cdfTable, flowCount, totalFlowSize, START_TIME, END_TIME, FLOW_LAUNCH_END_TIME);
    }

    NS_LOG_INFO ("Total flow: " << flowCount);

    NS_LOG_INFO ("Actual average flow size: " << static_cast<double> (totalFlowSize) / flowCount);

    NS_LOG_INFO ("Enabling flow monitor");

    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    std::stringstream flowMonitorFilename;

    flowMonitorFilename << id << "-fattree-" << k << "-" << load << "-"  << dctcpEnabled <<"-";
    if (runMode == ECMP)
    {
        flowMonitorFilename << "ecmp-simulation-";
    }
    else if (runMode == DRB)
    {
        flowMonitorFilename << "drb-simulation-";
    }
    else if (runMode == PRESTO)
    {
        flowMonitorFilename << "presto-simulation-";
    }
    else if (runMode == TLB)
    {
        flowMonitorFilename << "tlb-simulation-" << TLBRunMode << "-" << TLBMinRTT << "-" << TLBBetterPathRTT << "-" << TLBPoss << "-" << TLBECNPortionLow << "-" << TLBT1 << "-" << TLBProbingInterval << "-" << TLBSmooth << "-" << TLBRerouting << "-";
    }

    flowMonitorFilename << randomSeed << ".xml";

    NS_LOG_INFO ("Start simulation");
    Simulator::Stop (Seconds (END_TIME));
    Simulator::Run ();

    flowMonitor->SerializeToXmlFile(flowMonitorFilename.str (), true, true);

    Simulator::Destroy ();
    free_cdf (cdfTable);
    NS_LOG_INFO ("Stop simulation");

    return 0;
}

