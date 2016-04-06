#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-drb-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DcTcpDrbOriginal");

int main (int argc, char *argv[])
{
#if 1
    LogComponentEnable ("DcTcpDrbOriginal", LOG_LEVEL_INFO);
#endif

    CommandLine cmd;
    cmd.Parse (argc, argv);

    NS_LOG_INFO ("Create nodes.");
    NodeContainer c;
    c.Create (6);

    NodeContainer n0n1 = NodeContainer (c.Get (0), c.Get(1));
    NodeContainer n1n2 = NodeContainer (c.Get (1), c.Get(2));
    NodeContainer n1n3 = NodeContainer (c.Get (1), c.Get(3));
    NodeContainer n2n4 = NodeContainer (c.Get (2), c.Get(4));
    NodeContainer n3n4 = NodeContainer (c.Get (3), c.Get(4));
    NodeContainer n4n5 = NodeContainer (c.Get (4), c.Get(5));

    NS_LOG_INFO ("Install Internet stack");
    InternetStackHelper internet;
    internet.Install (c.Get (0));
    internet.Install (c.Get (2));
    internet.Install (c.Get (3));
    internet.Install (c.Get (5));
    internet.SetDrb (true);
    internet.Install (c.Get (1));
    internet.Install (c.Get (4));

    NS_LOG_INFO ("Install channel");
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
    p2p.SetChannelAttribute ("Delay", TimeValue (MicroSeconds(100)));

    NetDeviceContainer d0d1 = p2p.Install (n0n1);

    p2p.SetQueue ("ns3::RedQueueDisc", "Mode", StringValue("QUEUE_MODE_PACKETS"),
                                       "MinTh", DoubleValue(65),
                                       "MaxTh", DoubleValue(65),
                                       "QueueLimit", UintegerValue(1000));
    NetDeviceContainer d1d2 = p2p.Install (n1n2);
    NetDeviceContainer d2d4 = p2p.Install (n2n4);
    NetDeviceContainer d4d5 = p2p.Install (n4n5);

    p2p.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
    p2p.SetQueue ("ns3::RedQueueDisc", "Mode", StringValue("QUEUE_MODE_PACKETS"),
                                       "MinTh", DoubleValue(20),
                                       "MaxTh", DoubleValue(20),
                                       "QueueLimit", UintegerValue(1000));
    NetDeviceContainer d1d3 = p2p.Install (n1n3);
    NetDeviceContainer d3d4 = p2p.Install (n3n4);

    NS_LOG_INFO ("Assign IP address");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i0i1 = ipv4.Assign (d0d1);
    ipv4.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer i1i2 = ipv4.Assign (d1d2);
    ipv4.SetBase ("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer i2i4 = ipv4.Assign (d2d4);
    ipv4.SetBase ("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer i1i3 = ipv4.Assign (d1d3);
    ipv4.SetBase ("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer i3i4 = ipv4.Assign (d3d4);
    ipv4.SetBase ("10.1.6.0", "255.255.255.0");
    Ipv4InterfaceContainer i4i5 = ipv4.Assign (d4d5);

    NS_LOG_INFO ("Enable DRB");
    Ipv4DrbHelper drb;
    Ptr<Ipv4> ip1 = c.Get(1)->GetObject<Ipv4>();
    Ptr<Ipv4Drb> ipv4Drb1 = drb.GetIpv4Drb(ip1);
    ipv4Drb1->AddCoreSwitchAddress(i1i2.GetAddress (1));
    ipv4Drb1->AddCoreSwitchAddress(i1i3.GetAddress (1));

    Ptr<Ipv4> ip4 = c.Get(4)->GetObject<Ipv4>();
    Ptr<Ipv4Drb> ipv4Drb4 = drb.GetIpv4Drb(ip4);
    ipv4Drb4->AddCoreSwitchAddress(i2i4.GetAddress (0));
    ipv4Drb4->AddCoreSwitchAddress(i3i4.GetAddress (0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


    return 0;
}
