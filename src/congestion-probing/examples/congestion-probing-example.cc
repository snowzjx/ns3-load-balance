/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/congestion-probing-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/applications-module.h"
#include "ns3/link-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CongestionProbingExample");

std::string
DefaultFormat (struct LinkProbe::LinkStats stat)
{
  std::ostringstream oss;
  oss<< stat.packetsInQueueDisc;
  return oss.str ();
}

void ProbingEvent (uint32_t v, Ptr<Packet> packet, Ipv4Header header, Time rtt, bool isCE)
{
  NS_LOG_UNCOND ("Under V: " << v << ", the RTT is " << rtt << " and CE is " << isCE);
}


void ProbingTimeout (uint32_t v)
{
  NS_LOG_UNCOND ("Timeout for V: "<< v);
}

int
main (int argc, char *argv[])
{
#if 1
  LogComponentEnable ("CongestionProbingExample", LOG_LEVEL_INFO);
#endif

  CommandLine cmd;

  cmd.Parse (argc,argv);

  NodeContainer nodes;
  nodes.Create (2);

  InternetStackHelper internet;
  internet.Install (nodes);

  Config::SetDefault ("ns3::RedQueueDisc::Mode", StringValue ("QUEUE_MODE_PACKETS"));
  Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (1040));
  Config::SetDefault ("ns3::RedQueueDisc::QueueLimit", UintegerValue (100));

  Config::SetDefault ("ns3::CongestionProbing::ProbingInterval", TimeValue (Seconds (0.01)));

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpDCTCP::GetTypeId ()));

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (1e6)));
  p2p.SetChannelAttribute ("Delay", TimeValue(MicroSeconds (100)));
  p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (5));

  NetDeviceContainer netDeviceContainer = p2p.Install (nodes);

  TrafficControlHelper tc;
  tc.SetRootQueueDisc ("ns3::RedQueueDisc", "MinTh", DoubleValue (35),
                                            "MaxTh", DoubleValue (35));

  tc.Install (netDeviceContainer);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaceContainer = ipv4.Assign (netDeviceContainer);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Ptr<CongestionProbing> probing1= CreateObject<CongestionProbing> ();
  //probing1->SetFlowId (1);
  probing1->SetSourceAddress (interfaceContainer.GetAddress (0));
  probing1->SetProbeAddress (interfaceContainer.GetAddress (1));
  probing1->SetNode (nodes.Get (0));
  probing1->StopProbe (Seconds (10.0));

  bool traceConnectionResult =
        probing1->TraceConnectWithoutContext ("Probing", MakeCallback (&ProbingEvent));
  if (!traceConnectionResult)
  {
    NS_LOG_ERROR ("Connection failed");
  }

  traceConnectionResult = probing1->TraceConnectWithoutContext ("ProbingTimeout", MakeCallback (&ProbingTimeout));
  if (!traceConnectionResult)
  {
    NS_LOG_ERROR ("Connection failed");
  }

  probing1->StartProbe ();

  Ptr<CongestionProbing> probing2 = CreateObject<CongestionProbing> ();
  //probing2->SetFlowId (2);
  probing2->SetSourceAddress (interfaceContainer.GetAddress (1));
  probing2->SetProbeAddress (interfaceContainer.GetAddress (0));
  probing2->SetNode (nodes.Get (1));
  probing2->StopProbe (Seconds (10.0));
  probing2->StartProbe ();

  BulkSendHelper source ("ns3::TcpSocketFactory",
          InetSocketAddress (interfaceContainer.GetAddress(1), 2333));
  source.SetAttribute ("MaxBytes", UintegerValue (0));
  ApplicationContainer sourceApps = source.Install (nodes.Get (0));
  sourceApps.Start (Seconds (0.0));
  sourceApps.Stop (Seconds (10.0));

  PacketSinkHelper sink ("ns3::TcpSocketFactory",
           InetSocketAddress (Ipv4Address::GetAny (), 2333));
  ApplicationContainer sinkApp = sink.Install (nodes.Get(1));
  sinkApp.Start (Seconds (0.0));
  sinkApp.Stop (Seconds (10.0));

  Ptr<LinkMonitor> linkMonitor = Create<LinkMonitor> ();
  Ptr<Ipv4LinkProbe> linkProbe0 = Create<Ipv4LinkProbe> (nodes.Get (0), linkMonitor);
  linkProbe0->SetProbeName ("Node 0");
  linkProbe0->SetCheckTime (MicroSeconds (200));
  linkProbe0->SetDataRateAll (DataRate (1e9));

  Ptr<Ipv4LinkProbe> linkProbe1 = Create<Ipv4LinkProbe> (nodes.Get (1), linkMonitor);
  linkProbe1->SetProbeName ("Node 1");
  linkProbe1->SetCheckTime (MicroSeconds (200));
  linkProbe1->SetDataRateAll (DataRate (1e9));

  linkMonitor->Start (Seconds (0.0));
  linkMonitor->Stop (Seconds (10.0));

  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();

  linkMonitor->OutputToFile ("link-monitor.out", &DefaultFormat);

  Simulator::Destroy ();
  return 0;
}


