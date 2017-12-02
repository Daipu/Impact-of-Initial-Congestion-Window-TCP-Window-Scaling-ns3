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
#include "ns3/point-to-point-dumbbell.h"

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
	std::string link_bandwidth = "2Mbps";
	std::string host_bandwidth = "1Gbps";
	std::string link_delay = "50ms";
	std::string host_delay = "5ms";
	float duration = 4.0;
	bool en_sack = true;
	bool wnd_scaling = true;
	uint32_t init_cw = 1;
	//
	// Allow the user to override any of the defaults at
	// run-time, via command-line arguments
	//
  CommandLine cmd;
  cmd.AddValue ("topology", "Dumbbell(D)/Single P2P(S)", topology);
  cmd.AddValue ("leafNodes", "If Dumbbell, number of leaf nodes", leaf);
  cmd.AddValue ("maxBytes",
                "Total number of bytes for application to send", maxBytes);
  cmd.AddValue ("Link_bandwidth", "Bandwidth of router-router link", link_bandwidth);
  cmd.AddValue ("Link_delay", "Delay of router-router link", link_delay);
  cmd.AddValue ("Host_bandwidth", "Bandwidth of point to point link", host_bandwidth);
  cmd.AddValue ("Host_delay", "Delay of point to point link", host_delay);
  cmd.AddValue ("duration", "Time to allow flows to run in seconds", duration);
	cmd.AddValue ("sack", "Enable or disable SACK option", en_sack);
	cmd.AddValue ("wnd_scaling", "Enable or disable Window Scaling option", wnd_scaling);
	cmd.AddValue ("InitCwnd", "Size in packets for Initial Congestion Window", init_cw);
  cmd.Parse (argc, argv);
  
  ApplicationContainer sinkApps;
  uint16_t sinkPort = 9001;
  InternetStackHelper stack;
  
  float start_time = 1.0;
  float stop_time = start_time + duration;
  
  Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(init_cw));
  Config::SetDefault("ns3::TcpSocketBase::WindowScaling", BooleanValue(wnd_scaling));
  Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (en_sack));
	int tcpSegmentSize = 1000;
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (tcpSegmentSize));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (0));
  
  if(topology == 'S')
  {
  	leaf = 1; //This will be useful when printing bytes received at end	
	
  	NS_LOG_INFO ("Create nodes.");
  	Ptr<Node> A = CreateObject<Node> ();
  	Ptr<Node> B = CreateObject<Node> ();

  	NS_LOG_INFO ("Create channels.");

	//
	// Explicitly create the point-to-point link required by the topology (shown above).
	//
  	PointToPointHelper pointToPoint;
  	pointToPoint.SetDeviceAttribute ("DataRate", StringValue (host_bandwidth));
  	pointToPoint.SetChannelAttribute ("Delay", StringValue (host_delay));

  	NetDeviceContainer devices;
  	devices = pointToPoint.Install (A, B);

		stack.Install (A);
		stack.Install (B);
		
		Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  	em->SetAttribute ("ErrorRate", DoubleValue (0.00001));
  	devices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

	//
	// We've got the "hardware" in place.  Now we need to add IP addresses.
	//

  	NS_LOG_INFO ("Assign IP Addresses.");
  	Ipv4AddressHelper ipv4;
  	ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  	Ipv4InterfaceContainer i = ipv4.Assign (devices);
  	
  	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  	NS_LOG_INFO ("Create Applications.");
  	
	//
	// Create a PacketSinkApplication and install it on node 1
	//
  	PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  	/*ApplicationContainer */sinkApps = sink.Install (B);
  	sinkApps.Start (Seconds (start_time));
  	sinkApps.Stop (Seconds (stop_time));
  	
  	ObjectFactory factory;
  	factory.SetTypeId ("ns3::BulkSendApplication");
  	factory.Set ("Protocol", StringValue ("ns3::TcpSocketFactory"));
  	factory.Set ("MaxBytes", UintegerValue (maxBytes));
  	factory.Set ("SendSize", UintegerValue (tcpSegmentSize));
  	factory.Set ("Remote", AddressValue(InetSocketAddress(i.GetAddress (1), sinkPort)));
  	Ptr<Object> bulkSendAppObj = factory.Create();
  	Ptr<Application> bulkSendApp = bulkSendAppObj -> GetObject<Application>();
  	bulkSendApp->SetStartTime(Seconds(0.0));
  	bulkSendApp->SetStopTime(Seconds(stop_time));
  	A->AddApplication(bulkSendApp);

  	//Simulator::Schedule(Seconds(0.00001),&TraceRtt);
  	Simulator::Schedule(Seconds(0.00001),&TraceCwnd);
  }
  else
  {
  	// Create the point-to-point link helpers	

	PointToPointHelper pointToPointRouter;
  	pointToPointRouter.SetDeviceAttribute  ("DataRate", StringValue (link_bandwidth));
  	pointToPointRouter.SetChannelAttribute ("Delay", StringValue (link_delay));
  	PointToPointHelper pointToPointLeaf;
  	pointToPointLeaf.SetDeviceAttribute    ("DataRate", StringValue (host_bandwidth));
  	pointToPointLeaf.SetChannelAttribute   ("Delay", StringValue (host_delay));
  
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
  	sinkApps.Start (Seconds (start_time));
  	sinkApps.Stop (Seconds (stop_time - 1));
  
  	//Address sinkAddress (InetSocketAddress (d.GetRightIpv4Address (0), sinkPort));
  	BulkSendHelper source ("ns3::TcpSocketFactory", Address());
  	// Set the amount of data to send in bytes.  Zero is unlimited.
  	source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  	ApplicationContainer sourceApps;// = source.Install (d.GetLeft (0));
  	for(uint32_t i=0; i < d.LeftCount(); i++)
  	{
  		AddressValue sinkAddress (InetSocketAddress (d.GetRightIpv4Address (i), sinkPort));
  		source.SetAttribute ("Remote", sinkAddress);
  		sourceApps.Add (source.Install (d.GetLeft (i)));
  	}
    
  	sourceApps.Start (Seconds (start_time));
  	sourceApps.Stop (Seconds (stop_time));
  }
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (stop_time));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
  
  Ptr<PacketSink> sink1;
  for(uint32_t i=0; i<leaf; i++)
  {
  	sink1 = DynamicCast<PacketSink> (sinkApps.Get (i));
  	std::cout << "Total Bytes Received leaf n" << (i+leaf) << ": " << sink1->GetTotalRx () << std::endl;
  }
}
