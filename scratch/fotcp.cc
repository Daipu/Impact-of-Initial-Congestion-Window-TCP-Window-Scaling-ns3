// Network topology
//
//       n0 ---+      +--- n2
//             |      |
//             n4 -- n5
//             |      |
//       n1 ---+      +--- n3
//
// - All links are P2P with 500kb/s and 2ms


#include <fstream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("GP7");

int main (int argc, char *argv[])
{

	uint32_t    nLeftLeaf = 5;
  uint32_t    nRightLeaf = 5;
  //uint32_t    nLeaf = 0;
  uint32_t maxBytes = 0;
  
  // Create the point-to-point link helpers
	PointToPointHelper pointToPointRouter;
  pointToPointRouter.SetDeviceAttribute  ("DataRate", StringValue ("5Mbps"));
  pointToPointRouter.SetChannelAttribute ("Delay", StringValue ("5ms"));
  PointToPointHelper pointToPointLeaf;
  pointToPointLeaf.SetDeviceAttribute    ("DataRate", StringValue ("10Mbps"));
  pointToPointLeaf.SetChannelAttribute   ("Delay", StringValue ("1ms"));
  
  PointToPointDumbbellHelper d (nLeftLeaf, pointToPointLeaf, nRightLeaf, pointToPointLeaf, pointToPointRouter);

	// Install Stack
  InternetStackHelper stack;
  d.InstallStack (stack);
  
  NS_LOG_INFO ("Assign IP Addresses.");
  d.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"),
                          Ipv4AddressHelper ("10.2.1.0", "255.255.255.0"),
                          Ipv4AddressHelper ("10.3.1.0", "255.255.255.0"));

	NS_LOG_INFO ("Enable static global routing.");
  //
  // Turn on global static routing so we can actually be routed across the network.
  //
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  
  NS_LOG_INFO ("Create Applications.");
  
  uint16_t sinkPort = 9001;
  Address sinkAddress (InetSocketAddress (d.GetRightIpv4Address (0), sinkPort)); // interface of n2
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  ApplicationContainer sinkApps = packetSinkHelper.Install (d.GetRight (0)); //n2 as sink
  sinkApps.Start (Seconds (0.));
  sinkApps.Stop (Seconds (10.));
  
  BulkSendHelper source ("ns3::TcpSocketFactory", sinkAddress);
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  ApplicationContainer sourceApps = source.Install (d.GetLeft (0));
    
  sourceApps.Start (Seconds (1.0));
  sourceApps.Stop (Seconds (10.0));
  
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
  
  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));
  std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
}
