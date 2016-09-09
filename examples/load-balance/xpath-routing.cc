/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-xpath-routing-helper.h"
#include "ns3/congestion-probing-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("XPathRouting");

void ProbingEvent (uint32_t path, Ptr<Packet> packet, Ipv4Header header, Time rtt, bool isCE)
{
  NS_LOG_UNCOND ("Path: " << path <<", one way RTT is: " << rtt << "is CE: " << isCE);
}

int
main (int argc, char *argv[])
{
#if 1
  LogComponentEnable ("XPathRouting", LOG_LEVEL_INFO);
#endif

  bool verbose = true;

  CommandLine cmd;
  cmd.AddValue ("verbose", "Tell application to log if true", verbose);

  cmd.Parse (argc,argv);

  NodeContainer nodes;
  nodes.Create (6);

  InternetStackHelper internet;
  Ipv4GlobalRoutingHelper globalRoutingHelper;
  Ipv4XPathRoutingHelper xpathRoutingHelper;
  Ipv4ListRoutingHelper listRoutingHelper;

  internet.SetRoutingHelper (globalRoutingHelper);
  internet.Install (nodes.Get (0));
  internet.Install (nodes.Get (5));

  listRoutingHelper.Add (xpathRoutingHelper, 1);
  listRoutingHelper.Add (globalRoutingHelper, 0);
  internet.SetRoutingHelper (listRoutingHelper);
  internet.Install (nodes.Get (1));
  internet.Install (nodes.Get (2));
  internet.Install (nodes.Get (3));
  internet.Install (nodes.Get (4));

  PointToPointHelper p2p;
  Ipv4AddressHelper ipv4;

  p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (1000000000)));
  p2p.SetChannelAttribute ("Delay", TimeValue(MicroSeconds (10)));

  NodeContainer n0n1 = NodeContainer (nodes.Get (0), nodes.Get (1));
  NodeContainer n1n2 = NodeContainer (nodes.Get (1), nodes.Get (2));
  NodeContainer n1n3 = NodeContainer (nodes.Get (1), nodes.Get (3));
  NodeContainer n2n4 = NodeContainer (nodes.Get (2), nodes.Get (4));
  NodeContainer n3n4 = NodeContainer (nodes.Get (3), nodes.Get (4));
  NodeContainer n4n5 = NodeContainer (nodes.Get (4), nodes.Get (5));

  NetDeviceContainer d0d1 = p2p.Install (n0n1);
  NS_LOG_INFO ("Device 0 is connected to 1 " << d0d1.Get (0)->GetIfIndex () << "<->" << d0d1.Get (1)->GetIfIndex ());

  NetDeviceContainer d1d2 = p2p.Install (n1n2);
  NS_LOG_INFO ("Device 1 is connected to 2 " << d1d2.Get (0)->GetIfIndex () << "<->" << d1d2.Get (1)->GetIfIndex ());

  NetDeviceContainer d1d3 = p2p.Install (n1n3);
  NS_LOG_INFO ("Device 1 is connected to 3 " << d1d3.Get (0)->GetIfIndex () << "<->" << d1d3.Get (1)->GetIfIndex ());

  NetDeviceContainer d2d4 = p2p.Install (n2n4);
  NS_LOG_INFO ("Device 2 is connected to 4 " << d2d4.Get (0)->GetIfIndex () << "<->" << d2d4.Get (1)->GetIfIndex ());

  NetDeviceContainer d3d4 = p2p.Install (n3n4);
  NS_LOG_INFO ("Device 3 is connected to 4 " << d3d4.Get (0)->GetIfIndex () << "<->" << d3d4.Get (1)->GetIfIndex ());

  NetDeviceContainer d4d5 = p2p.Install (n4n5);
  NS_LOG_INFO ("Device 4 is connected to 5 " << d4d5.Get (0)->GetIfIndex () << "<->" << d4d5.Get (1)->GetIfIndex ());

  ipv4.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer i0i1 = ipv4.Assign (d0d1);
  ipv4.NewNetwork ();
  ipv4.Assign (d1d2);
  ipv4.NewNetwork ();
  ipv4.Assign (d1d3);
  ipv4.NewNetwork ();
  ipv4.Assign (d2d4);
  ipv4.NewNetwork ();
  ipv4.Assign (d3d4);
  ipv4.NewNetwork ();
  Ipv4InterfaceContainer i4i5 = ipv4.Assign (d4d5);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Config::SetDefault ("ns3::CongestionProbing::ProbingInterval", TimeValue (MicroSeconds (80)));
  Config::SetDefault ("ns3::CongestionProbing::ProbeTimeout", TimeValue (MicroSeconds (300)));

  Ptr<CongestionProbing> probing1= CreateObject<CongestionProbing> ();
  probing1->SetPathId (203);
  probing1->SetSourceAddress (i0i1.GetAddress (0));
  probing1->SetProbeAddress (i4i5.GetAddress (1));
  probing1->SetNode (nodes.Get (0));
  probing1->StopProbe (Seconds (0.05));
  probing1->StartProbe ();

  Ptr<CongestionProbing> probing2= CreateObject<CongestionProbing> ();
  probing2->SetPathId (101);
  probing2->SetSourceAddress (i4i5.GetAddress (1));
  probing2->SetProbeAddress (i0i1.GetAddress (0));
  probing2->SetNode (nodes.Get (5));
  probing2->StopProbe (Seconds (0.05));
  probing2->StartProbe ();

  bool traceConnectionResult = probing1->TraceConnectWithoutContext ("Probing", MakeCallback (&ProbingEvent));
  if (!traceConnectionResult)
  {
    NS_LOG_ERROR ("Connection failed");
    return -1;
  }

  p2p.EnablePcapAll ("xpath-routing");

  Simulator::Stop (Seconds (0.05));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}


