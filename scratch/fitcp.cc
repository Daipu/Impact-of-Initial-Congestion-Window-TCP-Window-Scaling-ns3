// Network topology
//
//       n0 ---+            +--- n5
//             |            |
//       n1 ---+            +--- n6
//             |            |
//       n2 ---+--n10--n11--+--- n7
//             |            |
//       n3 ---+            +--- n8
//             |            |
//       n4 ---+            +--- n9
//
// - All links are P2P with 5Mb/s and 5ms
//
//
//       n0 ----------- n1
//            500 Kbps
//             5 ms
//
// - Flow from n0 to n1 using BulkSendApplication.


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

Ptr<OutputStreamWrapper> strm;

static void 
CwndChange (uint32_t oldCwnd, uint32_t newCwnd)
{
  *strm->GetStream () << Simulator::Now ().GetSeconds () << " " << newCwnd << std::endl;
}

static void
TraceCwnd ()
{
  AsciiTraceHelper ascii;
  strm = ascii.CreateFileStream ("tcp-flow.dat");
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback (&CwndChange));
}

int main (int argc, char *argv[])
{
	uint32_t maxBytes = 0;
	char topology = 'S';
	uint32_t leaf = 5;
	//
	// Allow the user to override any of the defaults at
	// run-time, via command-line arguments
	//
  CommandLine cmd;
  cmd.AddValue ("topology", "Dumbbell(D)/Single P2P(S)", topology);
  cmd.AddValue ("leafNodes", "If Dumbbell, number of leaf nodes", leaf);
  cmd.AddValue ("maxBytes",
                "Total number of bytes for application to send", maxBytes);
  cmd.Parse (argc, argv);
  
  ApplicationContainer sinkApps;
  uint16_t sinkPort = 9001;
  InternetStackHelper stack;
  
  Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(4));
  Config::SetDefault("ns3::TcpSocketBase::WindowScaling", BooleanValue(false));
  
  if(topology == 'S')
  {
  	//
		// Explicitly create the nodes required by the topology (shown above).
		//
		
		leaf = 1;
		
  	NS_LOG_INFO ("Create nodes.");
  	NodeContainer nodes;
  	nodes.Create (2);

  	NS_LOG_INFO ("Create channels.");

		//
		// Explicitly create the point-to-point link required by the topology (shown above).
		//
  	PointToPointHelper pointToPoint;
  	pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("500Kbps"));
  	pointToPoint.SetChannelAttribute ("Delay", StringValue ("5ms"));

  	NetDeviceContainer devices;
  	devices = pointToPoint.Install (nodes);

		//
		// Install the internet stack on the nodes
		//
  	//InternetStackHelper internet;
  	stack.Install (nodes);

		//
		// We've got the "hardware" in place.  Now we need to add IP addresses.
		//

  	NS_LOG_INFO ("Assign IP Addresses.");
  	Ipv4AddressHelper ipv4;
  	ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  	Ipv4InterfaceContainer i = ipv4.Assign (devices);

  	NS_LOG_INFO ("Create Applications.");

		//
		// Create a BulkSendApplication and install it on node 0
		//

  	BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (i.GetAddress (1), sinkPort));
  	// Set the amount of data to send in bytes.  Zero is unlimited.
  	source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  	ApplicationContainer sourceApps = source.Install (nodes.Get (0));
  	Ptr<BulkSendApplication> source1 = DynamicCast<BulkSendApplication> (sourceApps.Get(0));
  
  	/*Ptr<Socket> sck = source1->GetSocket();
  	sck->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndChange));*/
  
  	sourceApps.Start (Seconds (0.0));
  	sourceApps.Stop (Seconds (10.0));
  
		//
		// Create a PacketSinkApplication and install it on node 1
		//
  	PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  	/*ApplicationContainer */sinkApps = sink.Install (nodes.Get (1));
  	sinkApps.Start (Seconds (0.0));
  	sinkApps.Stop (Seconds (10.0));

  	//Simulator::Schedule(Seconds(0.00001),&TraceRtt);
  	Simulator::Schedule(Seconds(0.00001),&TraceCwnd);
  }
  else
  {
  	// Create the point-to-point link helpers
		PointToPointHelper pointToPointRouter;
  	pointToPointRouter.SetDeviceAttribute  ("DataRate", StringValue ("5Mbps"));
  	pointToPointRouter.SetChannelAttribute ("Delay", StringValue ("5ms"));
  	PointToPointHelper pointToPointLeaf;
  	pointToPointLeaf.SetDeviceAttribute    ("DataRate", StringValue ("10Mbps"));
  	pointToPointLeaf.SetChannelAttribute   ("Delay", StringValue ("1ms"));
  
  	PointToPointDumbbellHelper d (leaf, pointToPointLeaf, leaf, pointToPointLeaf, pointToPointRouter);

		// Install Stack
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
  
  	PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  	for(uint32_t i=0; i < d.RightCount(); i++)
  	{
  		sinkApps.Add (packetSinkHelper.Install (d.GetRight (i))); //n5-n9 as sink
  	}
  	sinkApps.Start (Seconds (0.0));
  	sinkApps.Stop (Seconds (10.0));
  
  	//Address sinkAddress (InetSocketAddress (d.GetRightIpv4Address (0), sinkPort));
  	BulkSendHelper source ("ns3::TcpSocketFactory", sinkAddress);
  	// Set the amount of data to send in bytes.  Zero is unlimited.
  	source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  	ApplicationContainer sourceApps/* = source.Install (d.GetLeft (0))*/;
  	for(uint32_t i=0; i < d.LeftCount(); i++)
  	{
  		AddressValue sinkAddress (InetSocketAddress (d.GetRightIpv4Address (i), sinkPort));
  		source.SetAttribute ("Remote", sinkAddress);
  		sourceApps.Add (source.Install (d.GetLeft (i)));
  	}
    
  	sourceApps.Start (Seconds (1.0));
  	sourceApps.Stop (Seconds (10.0));
  }
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
  
  Ptr<PacketSink> sink1;
  for(uint32_t i=0; i<leaf; i++)
  {
  	sink1 = DynamicCast<PacketSink> (sinkApps.Get (i));
  	std::cout << "Total Bytes Received leaf n" << i+5 << ": " << sink1->GetTotalRx () << std::endl;
  }
}
