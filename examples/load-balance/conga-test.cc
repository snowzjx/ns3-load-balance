#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-conga-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CongaTest");

int main (int argc, char *argv[])
{
#if 1
    LogComponentEnable ("CongaTest", LOG_LEVEL_INFO);
#endif

    Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue (Seconds (0.01)));
    Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (100000000));
    Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (100000000));


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

    // Sender and Receiver
    internet.Install (c.Get (0));
    internet.Install (c.Get (5));

    // Conga Switch
    Config::SetDefault ("ns3::Ipv4GlobalRouting::CongaRouting", BooleanValue(true));
    internet.Install (c.Get (1));
    internet.Install (c.Get (2));
    internet.Install (c.Get (3));
    internet.Install (c.Get (4));

    NS_LOG_INFO ("Install channel");
    PointToPointHelper p2p;

    p2p.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
    p2p.SetChannelAttribute ("Delay", TimeValue(MicroSeconds (10)));
    NetDeviceContainer d0d1 = p2p.Install (n0n1);
    NetDeviceContainer d1d2 = p2p.Install (n1n2);
    NetDeviceContainer d1d3 = p2p.Install (n1n3);
    NetDeviceContainer d2d4 = p2p.Install (n2n4);
    NetDeviceContainer d3d4 = p2p.Install (n3n4);
    NetDeviceContainer d4d5 = p2p.Install (n4n5);

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


    NS_LOG_INFO ("Populate global routing tables");
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    // Set up Conga Switch
    NS_LOG_INFO ("Setting up Conga");
    Ipv4CongaHelper ipv4CongaHelper;
    Ptr<Ipv4Conga> conga1 = ipv4CongaHelper.GetIpv4Conga(c.Get(1)->GetObject<Ipv4>());
    conga1->SetLeafId(1);

    Ptr<Ipv4Conga> conga4 = ipv4CongaHelper.GetIpv4Conga(c.Get(4)->GetObject<Ipv4>());
    conga4->SetLeafId(4);

    conga1->AddAddressToLeafIdMap (i0i1.GetAddress (0), 1);
    conga1->AddAddressToLeafIdMap (i4i5.GetAddress (1), 4);

    conga4->AddAddressToLeafIdMap (i0i1.GetAddress (0), 1);
    conga4->AddAddressToLeafIdMap (i4i5.GetAddress (1), 4);

    NS_LOG_INFO ("Create applications");

    // Create bulk application
    uint16_t port = 22;
    BulkSendHelper source ("ns3::TcpSocketFactory",
            InetSocketAddress (i4i5.GetAddress(1), port));
    source.SetAttribute ("MaxBytes", UintegerValue (0));
    ApplicationContainer sourceApps = source.Install (c.Get (0));
    sourceApps.Start (Seconds (0.0));
    sourceApps.Stop (Seconds (0.1));

    PacketSinkHelper sink ("ns3::TcpSocketFactory",
            InetSocketAddress (Ipv4Address::GetAny (), port));
    ApplicationContainer sinkApp = sink.Install (c.Get(5));
    sinkApp.Start (Seconds (0.0));
    sinkApp.Stop (Seconds (0.1));


    NS_LOG_INFO ("Run Simulations");

    Simulator::Stop (Seconds (0.1));
    Simulator::Run ();
    Simulator::Destroy ();

    return 0;
}
