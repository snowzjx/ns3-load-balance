/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-drb-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DrbRouterTest");

int
main (int argc, char *argv[])
{

  // Allow the user to override any of the defaults and the above
  // DefaultValue::Bind ()s at run-time, via command-line arguments
  CommandLine cmd;
  cmd.Parse (argc, argv);

  Ptr<Node> nA = CreateObject<Node> ();
  Ptr<Node> nB = CreateObject<Node> ();
  Ptr<Node> nC = CreateObject<Node> ();
  Ptr<Node> nD = CreateObject<Node> ();
  Ptr<Node> nE = CreateObject<Node> ();
  Ptr<Node> nF = CreateObject<Node> ();

  InternetStackHelper internet;
  internet.Install (nA);
  internet.Install (nC);
  internet.Install (nD);
  internet.Install (nF);

  internet.SetDrb (true);
  internet.Install (nB);
  internet.Install (nE);

  // Point-to-point links
  NodeContainer nAnB = NodeContainer (nA, nB);
  NodeContainer nBnC = NodeContainer (nB, nC);
  NodeContainer nBnD = NodeContainer (nB, nD);
  NodeContainer nEnC = NodeContainer (nE, nC);
  NodeContainer nEnD = NodeContainer (nE, nD);
  NodeContainer nFnE = NodeContainer (nF, nE);

  // We create the channels first without any IP addressing information
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer dAdB = p2p.Install (nAnB);
  NetDeviceContainer dBdC = p2p.Install (nBnC);
  NetDeviceContainer dBdD = p2p.Install (nBnD);
  NetDeviceContainer dEdC = p2p.Install (nEnC);
  NetDeviceContainer dEdD = p2p.Install (nEnD);
  NetDeviceContainer dFdE = p2p.Install (nFnE);

  // Later, we add IP addresses.
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer iAiB = ipv4.Assign (dAdB);

  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer iBiC = ipv4.Assign (dBdC);

  ipv4.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer iBiD = ipv4.Assign (dBdD);

  ipv4.SetBase ("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer iEiC = ipv4.Assign (dEdC);

  ipv4.SetBase ("10.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer iEiD = ipv4.Assign (dEdD);

  ipv4.SetBase ("10.1.6.0", "255.255.255.0");
  Ipv4InterfaceContainer iFiE = ipv4.Assign (dFdE);

  // DRB bouncing switch configuration
  Ipv4DrbHelper drb;
  Ptr<Ipv4> ipv4B = nB->GetObject<Ipv4> ();
  Ptr<Ipv4Drb> ipv4DrbB = drb.GetIpv4Drb(ipv4B);
  ipv4DrbB->AddCoreSwitchAddress(iBiC.GetAddress (1));
  ipv4DrbB->AddCoreSwitchAddress(iBiD.GetAddress (1));

  Ptr<Ipv4> ipv4E = nE->GetObject<Ipv4> ();
  Ptr<Ipv4Drb> ipv4DrbE = drb.GetIpv4Drb(ipv4E);
  ipv4DrbE->AddCoreSwitchAddress(iEiC.GetAddress (1));
  ipv4DrbE->AddCoreSwitchAddress(iEiD.GetAddress (1));

  // Create router nodes, initialize routing database and set up the routing
  // tables in the nodes.
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Create the OnOff application to send TCP datagrams of size
  // 210 bytes at a rate of 448 Kb/s
  uint16_t port = 22;
  OnOffHelper onoff ("ns3::TcpSocketFactory",
           Address (InetSocketAddress (iFiE.GetAddress (0), port)));
  onoff.SetConstantRate (DataRate (6000));
  ApplicationContainer apps = onoff.Install (nA);
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (10.0));

  // Create a packet sink to receive these packets
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         Address (InetSocketAddress (Ipv4Address::GetAny (), port)));
  apps = sink.Install (nF);
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (10.0));

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
