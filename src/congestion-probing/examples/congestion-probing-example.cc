/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/congestion-probing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CongestionProbingExample");

void ProbingEvent (uint32_t v, Ptr<Packet> packet, Ipv4Header header, Time rtt, bool isCE)
{
  NS_LOG_UNCOND ("Under V: " << v << ", the RTT is " << rtt << " and CE is " << isCE);
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

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (1e6)));
  p2p.SetChannelAttribute ("Delay", TimeValue(MicroSeconds (100)));
  p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (100));

  NetDeviceContainer netDeviceContainer = p2p.Install (nodes);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaceContainer = ipv4.Assign (netDeviceContainer);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Ptr<CongestionProbing> probing1= Create<CongestionProbing> ();
  probing1->SetFlowId (1);
  probing1->SetSourceAddress (interfaceContainer.GetAddress (0));
  probing1->SetProbeAddress (interfaceContainer.GetAddress (1));
  probing1->SetNode (nodes.Get (0));
  probing1->StopProbe (Seconds (0.9));

  bool traceConnectionResult =
        probing1->TraceConnectWithoutContext ("Probing", MakeCallback (&ProbingEvent));
  if (!traceConnectionResult)
  {
    NS_LOG_ERROR ("Connection failed");
  }

  probing1->StartProbe ();

  Ptr<CongestionProbing> probing2 = Create<CongestionProbing> ();
  probing2->SetFlowId (2);
  probing2->SetSourceAddress (interfaceContainer.GetAddress (1));
  probing2->SetProbeAddress (interfaceContainer.GetAddress (0));
  probing2->SetNode (nodes.Get (1));
  probing2->StopProbe (Seconds (0.9));
  probing2->StartProbe ();

  Simulator::Stop (Seconds (1));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}


