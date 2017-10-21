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
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

//export 'NS_LOG=UdpEchoClientApplication=level_all|prefix_func:UdpEchoServerApplication=level_all|prefix_func'

NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);
  
  Time::SetResolution (Time::NS);
  NS_LOG_INFO("Creating Topology");
  //Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(137));
  //Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue("14kb/s"));
  
  LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
  LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
  
  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("2Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("20ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  PacketSinkHelper sink("ns3::TcpSocketFactory", Address(InetSocketAddress(Ipv4Address::GetAny(), 10)));
  //UdpEchoServerHelper echoServer (9);

  ApplicationContainer RecvApps = sink.Install (nodes.Get (0));
  RecvApps.Start (Seconds (1.0));
  RecvApps.Stop (Seconds (10.0));

  OnOffHelper ooh ("ns3::TcpSocketFactory", Address(InetSocketAddress(Ipv4Address("10.1.1.1"), 10)));
  ooh.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
  ooh.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
  
  //ooh.SetAttribute("DataRate", StringValue("2048bps"));
  ooh.SetAttribute("PacketSize", UintegerValue(1460));
  ooh.SetAttribute("MaxBytes", UintegerValue(10220));
  /*UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));*/

  ApplicationContainer SendApps = ooh.Install (nodes.Get (1));
  SendApps.Start (Seconds (2.0));
  SendApps.Stop (Seconds (10.0));
  
  pointToPoint.EnablePcapAll("testtcp");

  Simulator::Run ();
  //Simulator::Destroy ();
  return 0;
}
