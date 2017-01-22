#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-conga-routing-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-drb-routing-helper.h"
#include "ns3/ipv4-xpath-routing-helper.h"
#include "ns3/ipv4-tlb.h"
#include "ns3/ipv4-clove.h"
#include "ns3/ipv4-tlb-probing.h"
#include "ns3/link-monitor-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/tcp-resequence-buffer.h"
#include "ns3/ipv4-drill-routing-helper.h"
#include "ns3/ipv4-letflow-routing-helper.h"

#include <vector>
#include <map>
#include <utility>
#include <set>

// The CDF in TrafficGenerator
extern "C"
{
#include "cdf.h"
}

#define LINK_CAPACITY_BASE    1000000000          // 1Gbps
#define BUFFER_SIZE 600                           // 250 packets

#define RED_QUEUE_MARKING 65 		        	  // 65 Packets (available only in DcTcp)


// The flow port range, each flow will be assigned a random port number within this range
#define PORT_START 10000
#define PORT_END 50000

// Adopted from the simulation from WANG PENG
// Acknowledged to https://williamcityu@bitbucket.org/williamcityu/2016-socc-simulation.git
#define PACKET_SIZE 1400

#define PRESTO_RATIO 10

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CongaSimulationLarge");

enum RunMode {
    TLB,
    CONGA,
    CONGA_FLOW,
    CONGA_ECMP,
    PRESTO,
    WEIGHTED_PRESTO, // Distribute the packet according to the topology
    DRB,
    FlowBender,
    ECMP,
    Clove,
    DRILL,
    LetFlow
};

std::stringstream tlbBibleFilename;
std::stringstream tlbBibleFilename2;
std::stringstream rbTraceFilename;

void TLBPathSelectTrace (uint32_t flowId, uint32_t fromTor, uint32_t destTor, uint32_t path, bool isRandom, PathInfo pathInfo, std::vector<PathInfo> parallelPaths)
{
    NS_LOG_UNCOND ("Flow: " << flowId << " (" << fromTor << " -> " << destTor << ") selects path: " << path << " at: " << Simulator::Now ());
    NS_LOG_UNCOND ("\t Is random select: " << isRandom);
    NS_LOG_UNCOND ("\t Path info: type -> " << Ipv4TLB::GetPathType (pathInfo.pathType) << ", min RTT -> " << pathInfo.rttMin
            << ", ECN Portion -> " << pathInfo.ecnPortion << ", Flow counter -> " << pathInfo.counter
            << ", Quantified DRE -> " << pathInfo.quantifiedDre);

    NS_LOG_UNCOND ("\t Parallel path info: ");
    std::vector<PathInfo>::iterator itr = parallelPaths.begin ();
    for (; itr != parallelPaths.end (); ++itr)
    {
        struct PathInfo path = *itr;
        NS_LOG_UNCOND ("\t\t Path info: " << path.pathId << ", type -> " << Ipv4TLB::GetPathType (path.pathType) << ", min RTT -> " << path.rttMin
            << ", Path size -> " << path.size << ", ECN Portion -> " << path.ecnPortion << ", Flow counter -> " << path.counter
            << ", Quantified DRE -> " << path.quantifiedDre);
    }
    NS_LOG_UNCOND ("\n");

    std::ofstream out (tlbBibleFilename.str ().c_str (), std::ios::out|std::ios::app);
    out << "Flow: " << flowId << " (" << fromTor << " -> " << destTor << ") selects path: " << path << " at: " << Simulator::Now () << std::endl;
    out << "\t Is random select: " << isRandom << std::endl;
    out << "\t Path info: type -> " << Ipv4TLB::GetPathType (pathInfo.pathType) << ", min RTT -> " << pathInfo.rttMin
            << ", ECN Portion -> " << pathInfo.ecnPortion << ", Flow counter -> " << pathInfo.counter
            << ", Quantified DRE -> " << pathInfo.quantifiedDre << std::endl;

    out << "\t Parallel path info: " << std::endl;
    itr = parallelPaths.begin ();
    for (; itr != parallelPaths.end (); ++itr)
    {
        struct PathInfo path = *itr;
        out << "\t\t Path info: " << path.pathId << ", type -> " << Ipv4TLB::GetPathType (path.pathType) << ", min RTT -> " << path.rttMin
            << ", Path size -> " << path.size << ", ECN Portion -> " << path.ecnPortion << ", Flow counter -> " << path.counter
            << ", Quantified DRE -> " << path.quantifiedDre << std::endl;
    }
    out << std::endl;
}

void TLBPathChangeTrace (uint32_t flowId, uint32_t fromTor, uint32_t destTor, uint32_t newPath, uint32_t oldPath, bool isRandom, std::vector<PathInfo> parallelPaths)
{
    NS_LOG_UNCOND ("Flow: " << flowId << " (" << fromTor << " -> " << destTor << ") changes path from: " << oldPath << " to " << newPath << " at: " << Simulator::Now ());
    NS_LOG_UNCOND ("\t Is random select: " << isRandom);
    NS_LOG_UNCOND ("\t Parallel path info: ");
    std::vector<PathInfo>::iterator itr = parallelPaths.begin ();
    for (; itr != parallelPaths.end (); ++itr)
    {
        struct PathInfo path = *itr;
        NS_LOG_UNCOND ("\t\t Path info: " << path.pathId << ", type -> " << Ipv4TLB::GetPathType (path.pathType) << ", min RTT -> " << path.rttMin
            << ", Path size -> " << path.size << ", ECN Portion -> " << path.ecnPortion << ", Flow counter -> " << path.counter
            << ", Quantified DRE -> " << path.quantifiedDre);
    }
    NS_LOG_UNCOND ("\n");

    std::ofstream out (tlbBibleFilename2.str ().c_str (), std::ios::out|std::ios::app);
    out << "Flow: " << flowId << " (" << fromTor << " -> " << destTor << ") changes path from: " << oldPath << " to " << newPath << " at: " << Simulator::Now ();
    out << "\t Is random select: " << isRandom << std::endl;
    out << "\t Parallel path info: " << std::endl;
    itr = parallelPaths.begin ();
    for (; itr != parallelPaths.end (); ++itr)
    {
        struct PathInfo path = *itr;
        out << "\t\t Path info: " << path.pathId << ", type -> " << Ipv4TLB::GetPathType (path.pathType) << ", min RTT -> " << path.rttMin
            << ", Path size -> " << path.size << ", ECN Portion -> " << path.ecnPortion << ", Flow counter -> " << path.counter
            << ", Quantified DRE -> " << path.quantifiedDre << std::endl;
    }
    out << std::endl;
}

void RBTraceBuffer (uint32_t flowId, Time time, SequenceNumber32 revSeq, SequenceNumber32 expectSeq)
{
    NS_LOG_UNCOND ("Flow: " << flowId << " (at time: " << time << "), receives: " << revSeq << ", while expecting: " << expectSeq);
    std::ofstream out (rbTraceFilename.str ().c_str (), std::ios::out|std::ios::app);
    out << "Flow: " << flowId << " (at time: " << time << "), receives: " << revSeq << ", while expecting: " << expectSeq << std::endl;
}

void RBTraceFlush (uint32_t flowId, Time time, SequenceNumber32 popSeq, uint32_t inOrderQueueLength, uint32_t outOrderQueueLength, TcpRBPopReason reason)
{
    NS_LOG_UNCOND ("Flow: " << flowId << " (at time: " << time << "), pops: " << popSeq << ", with in order queue: " << inOrderQueueLength
            << ", out order queue: " << outOrderQueueLength << ", reason: " << reason);
    std::ofstream out (rbTraceFilename.str ().c_str (), std::ios::out|std::ios::app);
    out << "Flow: " << flowId << " (at time: " << time << "), pops: " << popSeq << ", with in order queue: " << inOrderQueueLength
            << ", out order queue: " << outOrderQueueLength << ", reason: " << reason << std::endl;
}

void RBTrace (void)
{
    Config::ConnectWithoutContext ("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/ResequenceBufferPointer/Buffer", MakeCallback (&RBTraceBuffer));
    Config::ConnectWithoutContext ("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/ResequenceBufferPointer/Flush", MakeCallback (&RBTraceFlush));
}

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

void install_applications (int fromLeafId, NodeContainer servers, double requestRate, struct cdf_table *cdfTable,
        long &flowCount, long &totalFlowSize, int SERVER_COUNT, int LEAF_COUNT, double START_TIME, double END_TIME, double FLOW_LAUNCH_END_TIME, uint32_t applicationPauseThresh, uint32_t applicationPauseTime)
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

            totalFlowSize += flowSize;
 	        source.SetAttribute ("SendSize", UintegerValue (PACKET_SIZE));
            source.SetAttribute ("MaxBytes", UintegerValue(flowSize));
            source.SetAttribute ("DelayThresh", UintegerValue (applicationPauseThresh));
            source.SetAttribute ("DelayTime", TimeValue (MicroSeconds (applicationPauseTime)));

            // Install apps
            ApplicationContainer sourceApp = source.Install (servers.Get (fromServerIndex));
            sourceApp.Start (Seconds (startTime));
            sourceApp.Stop (Seconds (END_TIME));

            // Install packet sinks
            PacketSinkHelper sink ("ns3::TcpSocketFactory",
                    InetSocketAddress (Ipv4Address::GetAny (), port));
            ApplicationContainer sinkApp = sink.Install (servers. Get (destServerIndex));
            sinkApp.Start (Seconds (START_TIME));
            sinkApp.Stop (Seconds (END_TIME));

            /*
            NS_LOG_INFO ("\tFlow from server: " << fromServerIndex << " to server: "
                    << destServerIndex << " on port: " << port << " with flow size: "
                    << flowSize << " [start time: " << startTime <<"]");
            */

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
    std::string id = "0";
    std::string runModeStr = "Conga";
    unsigned randomSeed = 0;
    std::string cdfFileName = "";
    double load = 0.0;
    std::string transportProt = "Tcp";

    // The simulation starting and ending time
    double START_TIME = 0.0;
    double END_TIME = 0.25;

    double FLOW_LAUNCH_END_TIME = 0.1;

    uint32_t linkLatency = 10;

    bool asymCapacity = false;

    uint32_t asymCapacityPoss = 40;  // 40 %

    bool resequenceBuffer = false;
    uint32_t resequenceInOrderTimer = 5; // MicroSeconds
    uint32_t resequenceInOrderSize = 100; // 100 Packets
    uint32_t resequenceOutOrderTimer = 100; // MicroSeconds
    bool resequenceBufferLog = false;

    double flowBenderT = 0.05;
    uint32_t flowBenderN = 1;

    int SERVER_COUNT = 8;
    int SPINE_COUNT = 4;
    int LEAF_COUNT = 4;
    int LINK_COUNT = 1;

    uint64_t spineLeafCapacity = 10;
    uint64_t leafServerCapacity = 10;

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
    bool TLBReverseACK = false;
    uint32_t TLBFlowletTimeout = 500;

    bool tcpPause = false;

    uint32_t applicationPauseThresh = 0;
    uint32_t applicationPauseTime = 1000;

    uint32_t cloveFlowletTimeout = 500;
    uint32_t cloveRunMode = 0;
    uint32_t cloveHalfRTT = 40;
    bool cloveDisToUncongestedPath = false;

    uint32_t quantifyRTTBase = 10;

    bool enableLargeDupAck = false;

    uint32_t congaFlowletTimeout = 500;
    uint32_t letFlowFlowletTimeout = 500;

    bool enableRandomDrop = false;
    double randomDropRate = 0.005; // 0.5%

    uint32_t blackHoleMode = 0; // When the black hole is enabled, the
    std::string blackHoleSrcAddrStr = "10.1.1.1";
    std::string blackHoleSrcMaskStr = "255.255.255.0";
    std::string blackHoleDestAddrStr = "10.1.2.0";
    std::string blackHoleDestMaskStr = "255.255.255.0";

    bool congaAwareAsym = true;
    bool asymCapacity2 = false;

    bool enableLargeSynRetries = false;

    bool enableLargeDataRetries = false;

    bool enableFastReConnection = false;

    CommandLine cmd;
    cmd.AddValue ("ID", "Running ID", id);
    cmd.AddValue ("StartTime", "Start time of the simulation", START_TIME);
    cmd.AddValue ("EndTime", "End time of the simulation", END_TIME);
    cmd.AddValue ("FlowLaunchEndTime", "End time of the flow launch period", FLOW_LAUNCH_END_TIME);
    cmd.AddValue ("runMode", "Running mode of this simulation: Conga, Conga-flow, Presto, Weighted-Presto, DRB, FlowBender, ECMP, Clove, DRILL, LetFlow", runModeStr);
    cmd.AddValue ("randomSeed", "Random seed, 0 for random generated", randomSeed);
    cmd.AddValue ("cdfFileName", "File name for flow distribution", cdfFileName);
    cmd.AddValue ("load", "Load of the network, 0.0 - 1.0", load);
    cmd.AddValue ("transportProt", "Transport protocol to use: Tcp, DcTcp", transportProt);
    cmd.AddValue ("linkLatency", "Link latency, should be in MicroSeconds", linkLatency);

    cmd.AddValue ("resequenceBuffer", "Whether enabling the resequence buffer", resequenceBuffer);
    cmd.AddValue ("resequenceInOrderTimer", "In order queue timeout in resequence buffer", resequenceInOrderTimer);
    cmd.AddValue ("resequenceOutOrderTimer", "Out order queue timeout in resequence buffer", resequenceOutOrderTimer);
    cmd.AddValue ("resequenceInOrderSize", "In order queue size in resequence buffer", resequenceInOrderSize);
    cmd.AddValue ("resequenceBufferLog", "Whether enabling the resequence buffer logging system", resequenceBufferLog);

    cmd.AddValue ("asymCapacity", "Whether the capacity is asym, which means some link will have only 1/10 the capacity of others", asymCapacity);
    cmd.AddValue ("asymCapacityPoss", "The possibility that a path will have only 1/10 capacity", asymCapacityPoss);

    cmd.AddValue ("flowBenderT", "The T in flowBender", flowBenderT);
    cmd.AddValue ("flowBenderN", "The N in flowBender", flowBenderN);

    cmd.AddValue ("serverCount", "The Server count", SERVER_COUNT);
    cmd.AddValue ("spineCount", "The Spine count", SPINE_COUNT);
    cmd.AddValue ("leafCount", "The Leaf count", LEAF_COUNT);
    cmd.AddValue ("linkCount", "The Link count", LINK_COUNT);

    cmd.AddValue ("spineLeafCapacity", "Spine <-> Leaf capacity in Gbps", spineLeafCapacity);
    cmd.AddValue ("leafServerCapacity", "Leaf <-> Server capacity in Gbps", leafServerCapacity);

    cmd.AddValue ("TLBMinRTT", "Min RTT used to judge a good path in TLB", TLBMinRTT);
    cmd.AddValue ("TLBHighRTT", "High RTT used to judge a bad path in TLB", TLBHighRTT);
    cmd.AddValue ("TLBPoss", "Possibility to change the path in TLB", TLBPoss);
    cmd.AddValue ("TLBBetterPathRTT", "RTT Threshold used to judge one path is better than another in TLB", TLBBetterPathRTT);
    cmd.AddValue ("TLBT1", "The path aging time interval in TLB", TLBT1);
    cmd.AddValue ("TLBECNPortionLow", "The ECN portion used in judging a good path in TLB", TLBECNPortionLow);
    cmd.AddValue ("TLBRunMode", "The running mode of TLB, 0 for minimize counter, 1 for minimize RTT, 2 for random, 11 for RTT counter, 12 for RTT DRE", TLBRunMode);
    cmd.AddValue ("TLBProbingEnable", "Whether the TLB probing is enable", TLBProbingEnable);
    cmd.AddValue ("TLBProbingInterval", "Probing interval for TLB probing", TLBProbingInterval);
    cmd.AddValue ("TLBSmooth", "Whether the RTT calculation is smooth", TLBSmooth);
    cmd.AddValue ("TLBRerouting", "Whether the rerouting is enabled in TLB", TLBRerouting);
    cmd.AddValue ("TLBDREMultiply", "DRE multiply factor in TLB", TLBDREMultiply);
    cmd.AddValue ("TLBS", "The S used to judge a whether a flow should change path in TLB", TLBS);
    cmd.AddValue ("TLBReverseACK", "Whether to enable the TLB reverse ACK path selection", TLBReverseACK);
    cmd.AddValue ("quantifyRTTBase", "The quantify RTT base in TLB", quantifyRTTBase);
    cmd.AddValue ("TLBFlowletTimeout", "The TLB flowlet timeout", TLBFlowletTimeout);

    cmd.AddValue ("TcpPause", "Whether TCP will pause in TLB & FlowBender", tcpPause);

    cmd.AddValue ("applicationPauseThresh", "How many packets can pass before we have delay, 0 for disable", applicationPauseThresh);
    cmd.AddValue ("applicationPauseTime", "The time for a delay, in MicroSeconds", applicationPauseTime);

    cmd.AddValue ("cloveFlowletTimeout", "Flowlet timeout for Clove", cloveFlowletTimeout);
    cmd.AddValue ("cloveRunMode", "Clove run mode, 1 for edge flowlet, 2 for ECN, 3 for INT (not yet implemented)", cloveRunMode);
    cmd.AddValue ("cloveHalfRTT", "Half RTT used in Clove ECN", cloveHalfRTT);
    cmd.AddValue ("cloveDisToUncongestedPath", "Whether Clove will distribute the weight to uncongested path (no ECN) or all paths", cloveDisToUncongestedPath);

    cmd.AddValue ("enableLargeDupAck", "Whether to set the ReTxThreshold to a very large value to mask reordering", enableLargeDupAck);

    cmd.AddValue ("congaFlowletTimeout", "Flowlet timeout in Conga", congaFlowletTimeout);
    cmd.AddValue ("letFlowFlowletTimeout", "Flowlet timeout in LetFlow", letFlowFlowletTimeout);

    cmd.AddValue ("enableRandomDrop", "Whether the Spine-0 to other leaves has the random drop problem", enableRandomDrop);
    cmd.AddValue ("randomDropRate", "The random drop rate when the random drop is enabled", randomDropRate);

    cmd.AddValue ("blackHoleMode", "The packet black hole mode, 0 to disable, 1 src, 2 dest, 3 src/dest pair", blackHoleMode);
    cmd.AddValue ("blackHoleSrcAddr", "The packet black hole source address", blackHoleSrcAddrStr);
    cmd.AddValue ("blackHoleSrcMask", "The packet black hole source mask", blackHoleSrcMaskStr);
    cmd.AddValue ("blackHoleDestAddr", "The packet black hole destination address", blackHoleDestAddrStr);
    cmd.AddValue ("blackHoleDestMask", "The packet black hole destination mask", blackHoleDestMaskStr);

    cmd.AddValue ("congaAwareAsym", "Whether Conga is aware of the capacity of asymmetric path capacity", congaAwareAsym);

    cmd.AddValue ("asymCapacity2", "Whether the Spine0-Leaf0's capacity is asymmetric", asymCapacity2);

    cmd.AddValue ("enableLargeSynRetries", "Whether the SYN packet would retry thousands of times", enableLargeSynRetries);
    cmd.AddValue ("enableFastReConnection", "Whether the SYN gap will be very small when reconnecting", enableFastReConnection);
    cmd.AddValue ("enableLargeDataRetries", "Whether the data retransmission will be more than 6 times", enableLargeDataRetries);

    cmd.Parse (argc, argv);

    uint64_t SPINE_LEAF_CAPACITY = spineLeafCapacity * LINK_CAPACITY_BASE;
    uint64_t LEAF_SERVER_CAPACITY = leafServerCapacity * LINK_CAPACITY_BASE;
    Time LINK_LATENCY = MicroSeconds (linkLatency);

    Ipv4Address blackHoleSrcAddr = Ipv4Address (blackHoleSrcAddrStr.c_str ());
    Ipv4Mask blackHoleSrcMask = Ipv4Mask (blackHoleSrcMaskStr.c_str ());
    Ipv4Address blackHoleDestAddr = Ipv4Address (blackHoleDestAddrStr.c_str ());
    Ipv4Mask blackHoleDestMask = Ipv4Mask (blackHoleDestMaskStr.c_str ());

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
        if (LINK_COUNT != 1)
        {
            NS_LOG_ERROR ("Presto currently does not support link count more than 1");
            return 0;
        }
        runMode = PRESTO;
    }
    else if (runModeStr.compare ("Weighted-Presto") == 0)
    {
        if (asymCapacity == false && asymCapacity2 == false)
        {
            NS_LOG_ERROR ("The Weighted-Presto has to work with asymmetric topology. For a symmetric topology, please use Presto instead");
            return 0;
        }
        runMode = WEIGHTED_PRESTO;
    }
    else if (runModeStr.compare ("DRB") == 0)
    {
        if (LINK_COUNT != 1)
        {
            NS_LOG_ERROR ("DRB currently does not support link count more than 1");
            return 0;
        }
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
    else if (runModeStr.compare ("TLB") == 0)
    {
        std::cout << Ipv4TLB::GetLogo () << std::endl;
        if (LINK_COUNT != 1)
        {
            NS_LOG_ERROR ("TLB currently does not support link count more than 1");
            return 0;
        }
        runMode = TLB;
    }
    else if (runModeStr.compare ("Clove") == 0)
    {
        runMode = Clove;
    }
    else if (runModeStr.compare ("DRILL") == 0)
    {
        runMode = DRILL;
    }
    else if (runModeStr.compare ("LetFlow") == 0)
    {
        runMode = LetFlow;
    }
    else
    {
        NS_LOG_ERROR ("The running mode should be TLB, Conga, Conga-flow, Conga-ECMP, Presto, FlowBender, DRB and ECMP");
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
        Config::SetDefault ("ns3::RedQueueDisc::Mode", StringValue ("QUEUE_MODE_BYTES"));
    	Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (PACKET_SIZE));
        Config::SetDefault ("ns3::RedQueueDisc::QueueLimit", UintegerValue (BUFFER_SIZE * PACKET_SIZE));
        //Config::SetDefault ("ns3::QueueDisc::Quota", UintegerValue (BUFFER_SIZE));
        Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (false));
    }

    if (resequenceBuffer)
    {
	    NS_LOG_INFO ("Enabling Resequence Buffer");
	    Config::SetDefault ("ns3::TcpSocketBase::ResequenceBuffer", BooleanValue (true));
        Config::SetDefault ("ns3::TcpResequenceBuffer::InOrderQueueTimerLimit", TimeValue (MicroSeconds (resequenceInOrderTimer)));
        Config::SetDefault ("ns3::TcpResequenceBuffer::SizeLimit", UintegerValue (resequenceInOrderSize));
        Config::SetDefault ("ns3::TcpResequenceBuffer::OutOrderQueueTimerLimit", TimeValue (MicroSeconds (resequenceOutOrderTimer)));
    }

    if (runMode == TLB)
    {
        NS_LOG_INFO ("Enabling TLB");
        Config::SetDefault ("ns3::TcpSocketBase::TLB", BooleanValue (true));
        Config::SetDefault ("ns3::TcpSocketBase::TLBReverseACK", BooleanValue (TLBReverseACK));
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
        Config::SetDefault ("ns3::Ipv4TLB::QuantifyRttBase", TimeValue (MicroSeconds (quantifyRTTBase)));
        Config::SetDefault ("ns3::Ipv4TLB::FlowletTimeout", TimeValue (MicroSeconds (TLBFlowletTimeout)));
    }

    if (runMode == Clove)
    {
        NS_LOG_INFO ("Enabling Clove");
        Config::SetDefault ("ns3::TcpSocketBase::Clove", BooleanValue (true));
        Config::SetDefault ("ns3::Ipv4Clove::FlowletTimeout", TimeValue (MicroSeconds (cloveFlowletTimeout)));
        Config::SetDefault ("ns3::Ipv4Clove::RunMode", UintegerValue (cloveRunMode));
        Config::SetDefault ("ns3::Ipv4Clove::HalfRTT", TimeValue (MicroSeconds (cloveHalfRTT)));
        Config::SetDefault ("ns3::Ipv4Clove::DisToUncongestedPath", BooleanValue (cloveDisToUncongestedPath));
    }

    if (tcpPause)
    {
        NS_LOG_INFO ("Enabling TCP pause");
        Config::SetDefault ("ns3::TcpSocketBase::Pause", BooleanValue (true));
    }

    NS_LOG_INFO ("Config parameters");
    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue(PACKET_SIZE));
    Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (0));
    if (enableFastReConnection)
    {
        Config::SetDefault ("ns3::TcpSocket::ConnTimeout", TimeValue (MicroSeconds (40)));
    }
    else
    {
        Config::SetDefault ("ns3::TcpSocket::ConnTimeout", TimeValue (MilliSeconds (5)));
    }
    Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (10));
    Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue (MilliSeconds (5)));
    Config::SetDefault ("ns3::TcpSocketBase::ClockGranularity", TimeValue (MicroSeconds (100)));
    Config::SetDefault ("ns3::RttEstimator::InitialEstimation", TimeValue (MicroSeconds (80)));
    Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (160000000));
    Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (160000000));

    if (enableLargeDupAck)
    {
        Config::SetDefault ("ns3::TcpSocketBase::ReTxThreshold", UintegerValue (1000));
    }

    if (enableLargeSynRetries)
    {
        Config::SetDefault ("ns3::TcpSocket::ConnCount", UintegerValue (10000));
    }

    if (enableLargeDataRetries)
    {
        Config::SetDefault ("ns3::TcpSocket::DataRetries", UintegerValue (10000));
    }

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
    Ipv4XPathRoutingHelper xpathRoutingHelper;
    Ipv4DrbRoutingHelper drbRoutingHelper;
    Ipv4DrillRoutingHelper drillRoutingHelper;
    Ipv4LetFlowRoutingHelper letFlowRoutingHelper;

    if (runMode == CONGA || runMode == CONGA_FLOW || runMode == CONGA_ECMP)
    {
	    internet.SetRoutingHelper (staticRoutingHelper);
        internet.Install (servers);

        internet.SetRoutingHelper (congaRoutingHelper);
        internet.Install (spines);
    	internet.Install (leaves);
    }
    else if (runMode == PRESTO || runMode == DRB || runMode == WEIGHTED_PRESTO)
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
        internet.Install (spines);
        internet.Install (leaves);
    }
    else if (runMode == TLB)
    {
        internet.SetTLB (true);
        internet.Install (servers);

        internet.SetTLB (false);
        listRoutingHelper.Add (xpathRoutingHelper, 1);
        listRoutingHelper.Add (globalRoutingHelper, 0);
        internet.SetRoutingHelper (listRoutingHelper);
        Config::SetDefault ("ns3::Ipv4GlobalRouting::PerflowEcmpRouting", BooleanValue(true));

        internet.Install (spines);
        internet.Install (leaves);
    }
    else if (runMode == Clove)
    {
        internet.SetClove (true);
        internet.Install (servers);

        internet.SetClove (false);
        listRoutingHelper.Add (xpathRoutingHelper, 1);
        listRoutingHelper.Add (globalRoutingHelper, 0);
        internet.SetRoutingHelper (listRoutingHelper);
        Config::SetDefault ("ns3::Ipv4GlobalRouting::PerflowEcmpRouting", BooleanValue(true));

        internet.Install (spines);
        internet.Install (leaves);
    }
    else if (runMode == DRILL)
    {
	    internet.SetRoutingHelper (staticRoutingHelper);
        internet.Install (servers);

        internet.SetRoutingHelper (drillRoutingHelper);
        internet.Install (spines);
    	internet.Install (leaves);
    }
    else if (runMode == LetFlow)
    {
        internet.SetRoutingHelper (staticRoutingHelper);
        internet.Install (servers);

        internet.SetRoutingHelper (letFlowRoutingHelper);
        internet.Install (spines);
        internet.Install (leaves);
    }
    else if (runMode == ECMP || runMode == FlowBender)
    {
        if (runMode == FlowBender)
        {
            NS_LOG_INFO ("Enabling Flow Bender");
            if (transportProt.compare ("Tcp") == 0)
            {
                NS_LOG_ERROR ("FlowBender has to be working with DCTCP");
                return 0;
            }
            Config::SetDefault ("ns3::TcpSocketBase::FlowBender", BooleanValue (true));
            Config::SetDefault ("ns3::TcpFlowBender::T", DoubleValue (flowBenderT));
            Config::SetDefault ("ns3::TcpFlowBender::N", UintegerValue (flowBenderN));
        }

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
    if (transportProt.compare ("DcTcp") == 0)
    {
        tc.SetRootQueueDisc ("ns3::RedQueueDisc", "MinTh", DoubleValue (RED_QUEUE_MARKING * PACKET_SIZE),
                                                  "MaxTh", DoubleValue (RED_QUEUE_MARKING * PACKET_SIZE));
    }

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
	    p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (10));
    }

    ipv4.SetBase ("10.1.0.0", "255.255.255.0");

    std::vector<Ipv4Address> leafNetworks (LEAF_COUNT);

    std::vector<Ipv4Address> serverAddresses (SERVER_COUNT * LEAF_COUNT);

    std::map<std::pair<int, int>, uint32_t> leafToSpinePath;
    std::map<std::pair<int, int>, uint32_t> spineToLeafPath;

    std::vector<Ptr<Ipv4TLBProbing> > probings (SERVER_COUNT * LEAF_COUNT);

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

            NS_LOG_INFO ("Leaf - " << i << " is connected to Server - " << j << " with address "
                    << interfaceContainer.GetAddress(0) << " <-> " << interfaceContainer.GetAddress (1)
                    << " with port " << netDeviceContainer.Get (0)->GetIfIndex () << " <-> " << netDeviceContainer.Get (1)->GetIfIndex ());

            serverAddresses [serverIndex] = interfaceContainer.GetAddress (1);
		    if (transportProt.compare ("Tcp") == 0)
            {
                tc.Uninstall (netDeviceContainer);
            }

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

            if (runMode == DRILL)
            {
                // All servers just forward the packet to leaf switch
		        staticRoutingHelper.GetStaticRouting (servers.Get (serverIndex)->GetObject<Ipv4> ())->
			                AddNetworkRouteTo (Ipv4Address ("0.0.0.0"),
					                           Ipv4Mask ("0.0.0.0"),
                                               netDeviceContainer.Get (1)->GetIfIndex ());

                // DRILL leaf switches forward the packet to the correct servers
                drillRoutingHelper.GetDrillRouting (leaves.Get (i)->GetObject<Ipv4> ())->
			                AddRoute (interfaceContainer.GetAddress (1),
				                      Ipv4Mask("255.255.255.255"),
                                      netDeviceContainer.Get (0)->GetIfIndex ());
            }

            if (runMode == LetFlow)
            {
                // All servers just forward the packet to leaf switch
		        staticRoutingHelper.GetStaticRouting (servers.Get (serverIndex)->GetObject<Ipv4> ())->
			                AddNetworkRouteTo (Ipv4Address ("0.0.0.0"),
					                           Ipv4Mask ("0.0.0.0"),
                                               netDeviceContainer.Get (1)->GetIfIndex ());

                Ptr<Ipv4LetFlowRouting> letFlowLeaf = letFlowRoutingHelper.GetLetFlowRouting (leaves.Get (i)->GetObject<Ipv4> ());

                // LetFlow leaf switches forward the packet to the correct servers
                letFlowLeaf->AddRoute (interfaceContainer.GetAddress (1),
				                       Ipv4Mask("255.255.255.255"),
                                       netDeviceContainer.Get (0)->GetIfIndex ());
                letFlowLeaf->SetFlowletTimeout (MicroSeconds (letFlowFlowletTimeout));
            }

            if (runMode == TLB)
            {
                for (int k = 0; k < SERVER_COUNT * LEAF_COUNT; k++)
                {
                    Ptr<Ipv4TLB> tlb = servers.Get (k)->GetObject<Ipv4TLB> ();
                    tlb->AddAddressWithTor (interfaceContainer.GetAddress (1), i);
                    // NS_LOG_INFO ("Configuring TLB with " << k << "'s server, inserting server: " << j << " under leaf: " << i);
                }
            }

            if (runMode == Clove)
            {
                for (int k = 0; k < SERVER_COUNT * LEAF_COUNT; k++)
                {
                    Ptr<Ipv4Clove> clove = servers.Get (k)->GetObject<Ipv4Clove> ();
                    clove->AddAddressWithTor (interfaceContainer.GetAddress (1), i);
                }

            }
        }
    }

    NS_LOG_INFO ("Configuring switches");
    // Setting up switches
    p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (SPINE_LEAF_CAPACITY)));
    std::set<std::pair<uint32_t, uint32_t> > asymLink; // set< (A, B) > Leaf A -> Spine B is asymmetric

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
	            congaLeaf->SetFlowletTimeout (MicroSeconds (congaFlowletTimeout));
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

        for (int j = 0; j < SPINE_COUNT; j++)
        {

        for (int l = 0; l < LINK_COUNT; l++)
        {
            bool isAsymCapacity = false;

            if (asymCapacity && static_cast<uint32_t> (rand () % 100) < asymCapacityPoss)
            {
                isAsymCapacity = true;
            }

            if (asymCapacity2 && i == 0 && j ==0)
            {
                isAsymCapacity = true;
            }

            // TODO
            uint64_t spineLeafCapacity = SPINE_LEAF_CAPACITY;

            if (isAsymCapacity)
            {
                spineLeafCapacity = SPINE_LEAF_CAPACITY / 5;
                asymLink.insert (std::make_pair (i, j));
                asymLink.insert (std::make_pair (j, i));
            }

            p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (spineLeafCapacity)));
            ipv4.NewNetwork ();

            NodeContainer nodeContainer = NodeContainer (leaves.Get (i), spines.Get (j));
            NetDeviceContainer netDeviceContainer = p2p.Install (nodeContainer);
		    if (transportProt.compare ("DcTcp") == 0)
		    {
		        NS_LOG_INFO ("Install RED Queue for leaf: " << i << " and spine: " << j);
                if (enableRandomDrop)
                {
                    if (j == 0)
                    {
                        Config::SetDefault ("ns3::RedQueueDisc::DropRate", DoubleValue (0.0));
                        tc.Install (netDeviceContainer.Get (0)); // Leaf to Spine Queue
                        Config::SetDefault ("ns3::RedQueueDisc::DropRate", DoubleValue (randomDropRate));
                        tc.Install (netDeviceContainer.Get (1)); // Spine to Leaf Queue
                    }
                    else
                    {
                        Config::SetDefault ("ns3::RedQueueDisc::DropRate", DoubleValue (0.0));
	                    tc.Install (netDeviceContainer);
                    }
                }
                else if (blackHoleMode != 0)
                {
                    if (j == 0)
                    {
                        Config::SetDefault ("ns3::RedQueueDisc::BlackHole", UintegerValue (0));
                        tc.Install (netDeviceContainer.Get (0)); // Leaf to Spine Queue
                        Config::SetDefault ("ns3::RedQueueDisc::BlackHole", UintegerValue (blackHoleMode));
                        tc.Install (netDeviceContainer.Get (1)); // Spine to Leaf Queue
                        Ptr<TrafficControlLayer> tc = netDeviceContainer.Get (1)->GetNode ()->GetObject<TrafficControlLayer> ();
                        Ptr<QueueDisc> queueDisc = tc->GetRootQueueDiscOnDevice (netDeviceContainer.Get (1));
                        Ptr<RedQueueDisc> redQueueDisc = DynamicCast<RedQueueDisc> (queueDisc);
                        redQueueDisc->SetBlackHoleSrc (blackHoleSrcAddr, blackHoleSrcMask);
                        redQueueDisc->SetBlackHoleDest (blackHoleDestAddr, blackHoleDestMask);
                    }
                    else
                    {
                        Config::SetDefault ("ns3::RedQueueDisc::BlackHole", UintegerValue (0));
                        tc.Install (netDeviceContainer);
                    }
                }
                else
                {
	                tc.Install (netDeviceContainer);
                }
            }
            Ipv4InterfaceContainer ipv4InterfaceContainer = ipv4.Assign (netDeviceContainer);
            NS_LOG_INFO ("Leaf - " << i << " is connected to Spine - " << j << " with address "
                    << ipv4InterfaceContainer.GetAddress(0) << " <-> " << ipv4InterfaceContainer.GetAddress (1)
                    << " with port " << netDeviceContainer.Get (0)->GetIfIndex () << " <-> " << netDeviceContainer.Get (1)->GetIfIndex ()
                    << " with data rate " << spineLeafCapacity);

            if (runMode == TLB || runMode == DRB || runMode == PRESTO || runMode == WEIGHTED_PRESTO || runMode == Clove)
            {
                std::pair<int, int> leafToSpine = std::make_pair<int, int> (i, j);
                leafToSpinePath[leafToSpine] = netDeviceContainer.Get (0)->GetIfIndex ();

                std::pair<int, int> spineToLeaf = std::make_pair<int, int> (j, i);
                spineToLeafPath[spineToLeaf] = netDeviceContainer.Get (1)->GetIfIndex ();
            }

		    if (transportProt.compare ("Tcp") == 0)
            {
                tc.Uninstall (netDeviceContainer);
            }

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

                if (isAsymCapacity)
                {
                    uint64_t congaAwareCapacity = congaAwareAsym ? spineLeafCapacity : SPINE_LEAF_CAPACITY;
                    Ptr<Ipv4CongaRouting> congaLeaf = congaRoutingHelper.GetCongaRouting (leaves.Get (i)->GetObject<Ipv4> ());
                    congaLeaf->SetLinkCapacity (netDeviceContainer.Get (0)->GetIfIndex (), DataRate (congaAwareCapacity));
                    NS_LOG_INFO ("Reducing Link Capacity of Conga Leaf: " << i << " with port: " << netDeviceContainer.Get (0)->GetIfIndex ());
                    congaSpine->SetLinkCapacity(netDeviceContainer.Get (1)->GetIfIndex (), DataRate (congaAwareCapacity));
                    NS_LOG_INFO ("Reducing Link Capacity of Conga Spine: " << j << " with port: " << netDeviceContainer.Get (1)->GetIfIndex ());
                }
	        }

            if (runMode == DRILL)
            {
                // For each drill leaf switch, routing entry to route the packet to OTHER leaves should be added
                for (int k = 0; k < LEAF_COUNT; k++)
		        {
		            if (k != i)
		            {
                        drillRoutingHelper.GetDrillRouting (leaves.Get (i)->GetObject<Ipv4> ())->
				                                            AddRoute (leafNetworks[k],
				  	                                        Ipv4Mask("255.255.255.0"),
                                  	                         netDeviceContainer.Get (0)->GetIfIndex ());
                    }
                }

                // For each drill spine switch, routing entry to THIS leaf switch should be added
                Ptr<Ipv4DrillRouting> drillSpine = drillRoutingHelper.GetDrillRouting (spines.Get (j)->GetObject<Ipv4> ());
                drillSpine->AddRoute (leafNetworks[i],
				                      Ipv4Mask("255.255.255.0"),
                                      netDeviceContainer.Get (1)->GetIfIndex ());
	        }

            if (runMode == LetFlow)
            {
                // For each LetFlow leaf switch, routing entry to route the packet to OTHER leaves should be added
                for (int k = 0; k < LEAF_COUNT; k++)
		        {
		            if (k != i)
		            {
                        letFlowRoutingHelper.GetLetFlowRouting (leaves.Get (i)->GetObject<Ipv4> ())->
				                                                AddRoute (leafNetworks[k],
				  	                                            Ipv4Mask("255.255.255.0"),
                                  	                            netDeviceContainer.Get (0)->GetIfIndex ());
                    }
                }

                // For each LetFlow spine switch, routing entry to THIS leaf switch should be added
                Ptr<Ipv4LetFlowRouting> letFlowSpine = letFlowRoutingHelper.GetLetFlowRouting (spines.Get (j)->GetObject<Ipv4> ());
                letFlowSpine->AddRoute (leafNetworks[i],
				                        Ipv4Mask("255.255.255.0"),
                                        netDeviceContainer.Get (1)->GetIfIndex ());
                letFlowSpine->SetFlowletTimeout (MicroSeconds (letFlowFlowletTimeout));
	        }
        }
        }
    }

    if (runMode == ECMP || runMode == PRESTO || runMode == WEIGHTED_PRESTO || runMode == DRB || runMode == FlowBender || runMode == TLB || runMode == Clove)
    {
        NS_LOG_INFO ("Populate global routing tables");
        Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    }

    if (runMode == DRB || runMode == PRESTO || runMode == WEIGHTED_PRESTO)
    {
        NS_LOG_INFO ("Configuring DRB / PRESTO paths");
        for (int i = 0; i < LEAF_COUNT; i++)
        {
            for (int j = 0; j < SERVER_COUNT; j++)
            {
                for (int k = 0; k < SPINE_COUNT; k++)
                {
                    int serverIndex = i * SERVER_COUNT + j;
                    Ptr<Ipv4DrbRouting> drbRouting = drbRoutingHelper.GetDrbRouting (servers.Get (serverIndex)->GetObject<Ipv4> ());
                    if (runMode == DRB)
                    {
                        drbRouting->AddPath (leafToSpinePath[std::make_pair (i, k)]);
                    }
                    else if (runMode == WEIGHTED_PRESTO)
                    {
                        // If the capacity of a uplink is reduced, the weight should be reduced either
                        if (asymLink.find (std::make_pair(i, k)) != asymLink.end ())
                        {
                            drbRouting->AddWeightedPath (PRESTO_RATIO * 0.2, leafToSpinePath[std::make_pair (i, k)]);
                        }
                        else
                        {
                            // Check whether the spine down to the leaf is reduced and add the exception
                            std::set<Ipv4Address> exclusiveIPs;
                            for (int l = 0; l < LEAF_COUNT; l++)
                            {
                                if (asymLink.find (std::make_pair(k, l)) != asymLink.end ())
                                {
                                    for (int m = l * SERVER_COUNT; m < l * SERVER_COUNT + SERVER_COUNT; m++)
                                    {
                                        Ptr<Node> destServer = servers.Get (m);
                                        Ptr<Ipv4> ipv4 = destServer->GetObject<Ipv4> ();
                                        Ipv4InterfaceAddress destInterface = ipv4->GetAddress (1,0);
                                        Ipv4Address destAddress = destInterface.GetLocal ();
                                        drbRouting->AddWeightedPath (destAddress, PRESTO_RATIO * 0.2, leafToSpinePath[std::make_pair (i, k)]);
                                        exclusiveIPs.insert (destAddress);
                                    }
                                }
                            }
                            drbRouting->AddWeightedPath (PRESTO_RATIO, leafToSpinePath[std::make_pair (i, k)], exclusiveIPs);
                        }
                    }
                    else
                    {
                        drbRouting->AddPath (PRESTO_RATIO, leafToSpinePath[std::make_pair (i, k)]);
                    }
                }
            }
        }
    }

    if (runMode == Clove)
    {
        NS_LOG_INFO ("Configuring Clove available paths");
        for (int i = 0; i < LEAF_COUNT; i++)
        {
            for (int j = 0; j < SERVER_COUNT; j++)
            {
                int serverIndex = i * SERVER_COUNT + j;
                for (int k = 0; k < SPINE_COUNT; k++)
                {
                    int path = 0;
                    int pathBase = 1;
                    path += leafToSpinePath[std::make_pair (i, k)] * pathBase;
                    pathBase *= 100;
                    for (int l = 0; l < LEAF_COUNT; l++)
                    {
                        if (i == l)
                        {
                            continue;
                        }
                        int newPath = spineToLeafPath[std::make_pair (k, l)] * pathBase + path;
                        Ptr<Ipv4Clove> clove = servers.Get (serverIndex)->GetObject<Ipv4Clove> ();
                        clove->AddAvailPath (l, newPath);
                    }
                }
            }
        }
    }

    if (runMode == TLB)
    {
        NS_LOG_INFO ("Configuring TLB available paths");
        for (int i = 0; i < LEAF_COUNT; i++)
        {
            for (int j = 0; j < SERVER_COUNT; j++)
            {
                int serverIndex = i * SERVER_COUNT + j;
                for (int k = 0; k < SPINE_COUNT; k++)
                {
                    int path = 0;
                    int pathBase = 1;
                    path += leafToSpinePath[std::make_pair (i, k)] * pathBase;
                    pathBase *= 100;
                    for (int l = 0; l < LEAF_COUNT; l++)
                    {
                        if (i == l)
                        {
                            continue;
                        }
                        int newPath = spineToLeafPath[std::make_pair (k, l)] * pathBase + path;
                        Ptr<Ipv4TLB> tlb = servers.Get (serverIndex)->GetObject<Ipv4TLB> ();
                        tlb->AddAvailPath (l, newPath);
                        //NS_LOG_INFO ("Configuring server: " << serverIndex << " to leaf: " << l << " with path: " << newPath);
                    }
                }
            }
        }


        if (runMode == TLB && TLBProbingEnable)
        {
        NS_LOG_INFO ("Configuring TLB Probing");
        for (int i = 0; i < SERVER_COUNT * LEAF_COUNT; i++)
        {
            // The i th server under one leaf is used to probe the leaf i by contacting the i th server under that leaf
            Ptr<Ipv4TLBProbing> probing = CreateObject<Ipv4TLBProbing> ();
            probings[i] = probing;
            probing->SetNode (servers.Get (i));
            probing->SetSourceAddress (serverAddresses[i]);
            probing->Init ();

            int serverIndexUnderLeaf = i % SERVER_COUNT;

            if (serverIndexUnderLeaf < LEAF_COUNT)
            {
                int serverBeingProbed = SERVER_COUNT * serverIndexUnderLeaf;
                if (serverBeingProbed == i)
                {
                    continue;
                }
                probing->SetProbeAddress (serverAddresses[serverBeingProbed]);
                //NS_LOG_INFO ("Server: " << i << " is going to probe server: " << serverBeingProbed);
                int leafIndex = i / SERVER_COUNT;
                for (int j = leafIndex * SERVER_COUNT; j < leafIndex * SERVER_COUNT + SERVER_COUNT; j++)
                {
                    if (i == j)
                    {
                        continue;
                    }
                    probing->AddBroadCastAddress (serverAddresses[j]);
                    //NS_LOG_INFO ("Server:" << i << " is going to broadcast to server: " << j);
                }
                probing->StartProbe ();
                probing->StopProbe (Seconds (END_TIME));
            }
        }
        }
    }

    double oversubRatio = static_cast<double>(SERVER_COUNT * LEAF_SERVER_CAPACITY) / (SPINE_LEAF_CAPACITY * SPINE_COUNT * LINK_COUNT);
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

    long flowCount = 0;
    long totalFlowSize = 0;

    for (int fromLeafId = 0; fromLeafId < LEAF_COUNT; fromLeafId ++)
    {
        install_applications(fromLeafId, servers, requestRate, cdfTable, flowCount, totalFlowSize, SERVER_COUNT, LEAF_COUNT, START_TIME, END_TIME, FLOW_LAUNCH_END_TIME, applicationPauseThresh, applicationPauseTime);
    }

    NS_LOG_INFO ("Total flow: " << flowCount);

    NS_LOG_INFO ("Actual average flow size: " << static_cast<double> (totalFlowSize) / flowCount);

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

    flowMonitor->CheckForLostPackets ();

    std::stringstream flowMonitorFilename;
    std::stringstream linkMonitorFilename;

    flowMonitorFilename << id << "-1-large-load-" << LEAF_COUNT << "X" << SPINE_COUNT << "-" << load << "-"  << transportProt <<"-";
    linkMonitorFilename << id << "-1-large-load-" << LEAF_COUNT << "X" << SPINE_COUNT << "-" << load << "-"  << transportProt <<"-";
    tlbBibleFilename << id << "-1-large-load-" << LEAF_COUNT << "X" << SPINE_COUNT << "-" << load << "-"  << transportProt <<"-";
    tlbBibleFilename2 << id << "-1-large-load-" << LEAF_COUNT << "X" << SPINE_COUNT << "-" << load << "-"  << transportProt <<"-";
    rbTraceFilename << id << "-1-large-load-" << LEAF_COUNT << "X" << SPINE_COUNT << "-" << load << "-"  << transportProt <<"-";

    if (runMode == CONGA)
    {
        flowMonitorFilename << "conga-simulation-" << congaFlowletTimeout << "-";
        linkMonitorFilename << "conga-simulation-" << congaFlowletTimeout << "-";
    }
    else if (runMode == CONGA_FLOW)
    {
        flowMonitorFilename << "conga-flow-simulation-";
        linkMonitorFilename << "conga-flow-simulation-";
    }
    else if (runMode == CONGA_ECMP)
    {
        flowMonitorFilename << "conga-ecmp-simulation-" << congaFlowletTimeout << "-";
        linkMonitorFilename << "conga-ecmp-simulation-" << congaFlowletTimeout << "-";
    }
    else if (runMode == PRESTO)
    {
	    flowMonitorFilename << "presto-simulation-";
        linkMonitorFilename << "presto-simulation-";
        rbTraceFilename << "presto-simulation-";
    }
    else if (runMode == WEIGHTED_PRESTO)
    {
	    flowMonitorFilename << "weighted-presto-simulation-";
        linkMonitorFilename << "weighted-presto-simulation-";
        rbTraceFilename << "weighted-presto-simulation-";
    }
    else if (runMode == DRB)
    {
        flowMonitorFilename << "drb-simulation-";
        linkMonitorFilename << "drb-simulation-";
        rbTraceFilename << "drb-simulation-";
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
    else if (runMode == TLB)
    {
        flowMonitorFilename << "tlb-" << TLBHighRTT << "-" << TLBMinRTT << "-" << TLBBetterPathRTT << "-" << TLBPoss << "-" << TLBS << "-" << TLBT1 << "-" << TLBProbingInterval << "-" << TLBSmooth << "-" << TLBRerouting << "-" << quantifyRTTBase << "-";
        linkMonitorFilename << "tlb-" << TLBHighRTT << "-" << TLBMinRTT << "-" << TLBBetterPathRTT << "-" << TLBPoss << "-" << TLBS << "-" << TLBT1 << "-" << TLBProbingInterval << "-" << TLBSmooth << "-" << TLBRerouting << "-" << quantifyRTTBase << "-";
        tlbBibleFilename << "tlb-" << TLBHighRTT << "-" << TLBMinRTT << "-" << TLBBetterPathRTT << "-" << TLBPoss << "-" << TLBS << "-" << TLBT1 << "-" << TLBProbingInterval << "-" << TLBSmooth << "-" << TLBRerouting << "-" << quantifyRTTBase << "-";
        tlbBibleFilename2 << "tlb-" << TLBHighRTT << "-" << TLBMinRTT << "-" << TLBBetterPathRTT << "-" << TLBPoss << "-" << TLBS << "-" << TLBT1 << "-" << TLBProbingInterval << "-" << TLBSmooth << "-" << TLBRerouting << "-" << quantifyRTTBase << "-";
    }
    else if (runMode == Clove)
    {
        flowMonitorFilename << "clove-" << cloveRunMode << "-" << cloveFlowletTimeout << "-" << cloveHalfRTT << "-" << cloveDisToUncongestedPath << "-";
        linkMonitorFilename << "clove-" << cloveRunMode << "-" << cloveFlowletTimeout << "-" << cloveHalfRTT << "-" << cloveDisToUncongestedPath << "-";
    }
    else if (runMode == DRILL)
    {
        flowMonitorFilename << "drill-simulation-";
        linkMonitorFilename << "drill-simulation-";
    }
    else if (runMode == LetFlow)
    {
        flowMonitorFilename << "letflow-simulation-" << letFlowFlowletTimeout << "-";
        linkMonitorFilename << "letflow-simulation-" << letFlowFlowletTimeout << "-";
    }

    flowMonitorFilename << randomSeed << "-";
    linkMonitorFilename << randomSeed << "-";
    tlbBibleFilename << randomSeed << "-";
    tlbBibleFilename2 << randomSeed << "-";
    rbTraceFilename << randomSeed << "-";

    if (asymCapacity)
    {
        flowMonitorFilename << "capacity-asym-";
	    linkMonitorFilename << "capacity-asym-";
        tlbBibleFilename << "capacity-asym-";
        tlbBibleFilename2 << "capacity-asym-";
    }

    if (asymCapacity2)
    {
        flowMonitorFilename << "capacity-asym2-";
	    linkMonitorFilename << "capacity-asym2-";
        tlbBibleFilename << "capacity-asym2-";
        tlbBibleFilename2 << "capacity-asym2-";
    }

    if (resequenceBuffer)
    {
        flowMonitorFilename << "rb-" << resequenceInOrderSize << "-" <<resequenceInOrderTimer << "-" << resequenceOutOrderTimer;
        linkMonitorFilename << "rb-" << resequenceInOrderSize << "-" <<resequenceInOrderTimer << "-" << resequenceOutOrderTimer;
        rbTraceFilename << "rb-";
    }

    if (applicationPauseThresh > 0)
    {
        flowMonitorFilename << "p" << applicationPauseThresh << "-" << applicationPauseTime << "-";
        linkMonitorFilename << "p" << applicationPauseThresh << "-" << applicationPauseTime << "-";
        tlbBibleFilename << "p" << applicationPauseThresh << "-" << applicationPauseTime << "-";
        tlbBibleFilename2 << "p" << applicationPauseThresh << "-" << applicationPauseTime << "-";
    }

    if (enableRandomDrop)
    {
        flowMonitorFilename << "random-drop-" << randomDropRate << "-";
        linkMonitorFilename << "random-drop-" << randomDropRate << "-";
        tlbBibleFilename << "random-drop-" << randomDropRate << "-";
        tlbBibleFilename2 << "random-drop-" << randomDropRate << "-";
        rbTraceFilename << "random-drop-" << randomDropRate << "-";
    }

    if (blackHoleMode != 0)
    {
        flowMonitorFilename << "black-hole-" << blackHoleMode << "-";
        linkMonitorFilename << "black-hole-" << blackHoleMode << "-";
        tlbBibleFilename << "black-hole-" << blackHoleMode << "-";
        tlbBibleFilename2 << "black-hole-" << blackHoleMode << "-";
        rbTraceFilename << "black-hole-" << blackHoleMode << "-";
    }


    flowMonitorFilename << "b" << BUFFER_SIZE << ".xml";
    linkMonitorFilename << "b" << BUFFER_SIZE << "-link-utility.out";
    tlbBibleFilename << "b" << BUFFER_SIZE << "-bible.txt";
    tlbBibleFilename2 << "b" << BUFFER_SIZE << "-piple.txt";
    rbTraceFilename << "b" << BUFFER_SIZE << "-RBTrace.txt";

    if (runMode == TLB)
    {
        NS_LOG_INFO ("Enabling TLB tracing");
        remove (tlbBibleFilename.str ().c_str ());
        remove (tlbBibleFilename2.str ().c_str ());

        Config::ConnectWithoutContext ("/NodeList/*/$ns3::Ipv4TLB/SelectPath",
                MakeCallback (&TLBPathSelectTrace));

        Config::ConnectWithoutContext ("/NodeList/*/$ns3::Ipv4TLB/ChangePath",
                MakeCallback (&TLBPathChangeTrace));
        std::ofstream out (tlbBibleFilename.str ().c_str (), std::ios::out|std::ios::app);
        out << Ipv4TLB::GetLogo ();
        std::ofstream out2 (tlbBibleFilename2.str ().c_str (), std::ios::out|std::ios::app);
        out2 << Ipv4TLB::GetLogo ();
    }

    if (resequenceBuffer && resequenceBufferLog)
    {
        remove (rbTraceFilename.str ().c_str ());
        Simulator::Schedule (Seconds (START_TIME) + MicroSeconds (1), &RBTrace);
    }

    NS_LOG_INFO ("Start simulation");
    Simulator::Stop (Seconds (END_TIME));
    Simulator::Run ();

    flowMonitor->SerializeToXmlFile(flowMonitorFilename.str (), true, true);
    linkMonitor->OutputToFile (linkMonitorFilename.str (), &LinkMonitor::DefaultFormat);

    Simulator::Destroy ();
    free_cdf (cdfTable);
    NS_LOG_INFO ("Stop simulation");
}
