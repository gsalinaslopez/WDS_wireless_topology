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
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/mac48-address.h"
#include "ns3/uinteger.h"

#include "ns3/fd-net-device-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"

// Default Network Topology
//
// Number of wifi or csma nodes can be increased up to 250
//                          |
//                 Rank 0   |   Rank 1
// -------------------------|----------------------------
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n5   cli  c0   c1 -------------- n1   n2   n3   n4
//                   point-to-point  |    |    |    |
//                                   ================
//                                     LAN 10.1.2.0

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample");

int
main (int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nCsma = 3;
  uint32_t nWifi = 1;
  bool tracing = true;

  CommandLine cmd;
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);

  cmd.Parse (argc,argv);

  // Check for valid number of csma or wifi nodes
  // 250 should be enough, otherwise IP addresses
  // soon become an issue
  if (nWifi > 250 || nCsma > 250)
    {
      std::cout << "Too many wifi or csma nodes, no more than 250 each." << std::endl;
      return 1;
    }

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

// Create Curt1 and CSMA0
  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (1));
  csmaNodes.Create (nCsma);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

// Create client NodeContainer
  NodeContainer wifiCliNodes;
  wifiCliNodes.Create(1);

// Create Curts
  NodeContainer wifiCurtNodes;
  wifiCurtNodes.Create (6);

// Curt4 connected to csma
  NodeContainer wifiApNode = p2pNodes.Get (0);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());
  phy.Set ("ChannelNumber", ns3::UintegerValue(1));

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer cliDevices;
  cliDevices = wifi.Install (phy, mac, wifiCliNodes);

// Setup Curt0's west AP
  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer curtDevices;
  curtDevices = wifi.Install (phy, mac, wifiCurtNodes.Get(0));

// wifi and mac helper for WDS`
  NqosWifiMacHelper wdsWifiMac = NqosWifiMacHelper::Default ();
  WifiHelper wdsWifiHelper = WifiHelper::Default ();
  wdsWifiHelper.SetRemoteStationManager("ns3::MinstrelWifiManager");

  YansWifiChannelHelper wdsChannel = YansWifiChannelHelper::Default ();
  phy.SetChannel (wdsChannel.Create ());
  phy.Set ("ChannelNumber", ns3::UintegerValue(6));

// Curt0's east AP and Curt1's west AP
  Mac48Address curt0EastAPMAC = ns3::Mac48Address::Allocate();
  Mac48Address curt1WestAPMAC = ns3::Mac48Address::Allocate();

  wdsWifiMac.SetType("ns3::WDSWifiMac", "ReceiverAddress",
              Mac48AddressValue(curt1WestAPMAC));
  NetDeviceContainer curt0EastAPDevice = wifi.Install(phy, wdsWifiMac, wifiCurtNodes.Get(0));
  curt0EastAPDevice.Get(0)->SetAddress(curt0EastAPMAC);

  wdsWifiMac.SetType("ns3::WDSWifiMac", "ReceiverAddress",
              Mac48AddressValue(curt0EastAPMAC));
  NetDeviceContainer curt1WestAPDevice = wifi.Install(phy, wdsWifiMac, wifiCurtNodes.Get(1));
  curt1WestAPDevice.Get(0)->SetAddress(curt1WestAPMAC);

// Curt1's east AP and Curt2's west AP
YansWifiChannelHelper wds1Channel = YansWifiChannelHelper::Default ();
phy.SetChannel (wds1Channel.Create ());
phy.Set ("ChannelNumber", ns3::UintegerValue(1));

  Mac48Address curt1EastAPMAC = ns3::Mac48Address::Allocate();
  Mac48Address curt2WestAPMAC = ns3::Mac48Address::Allocate();

  wdsWifiMac.SetType("ns3::WDSWifiMac", "ReceiverAddress",
              Mac48AddressValue(curt2WestAPMAC));
  NetDeviceContainer curt1EastAPDevice = wifi.Install(phy, wdsWifiMac, wifiCurtNodes.Get(1));
  curt1EastAPDevice.Get(0)->SetAddress(curt1EastAPMAC);

  wdsWifiMac.SetType("ns3::WDSWifiMac", "ReceiverAddress",
              Mac48AddressValue(curt1EastAPMAC));
  NetDeviceContainer curt2WestAPDevice = wifi.Install(phy, wdsWifiMac, wifiCurtNodes.Get(2));
  curt2WestAPDevice.Get(0)->SetAddress(curt2WestAPMAC);

// Curt2's east AP and Curt3's west AP
YansWifiChannelHelper wds2Channel = YansWifiChannelHelper::Default ();
phy.SetChannel (wds2Channel.Create ());
phy.Set ("ChannelNumber", ns3::UintegerValue(9));
  Mac48Address curt2EastAPMAC = ns3::Mac48Address::Allocate();
  Mac48Address curt3WestAPMAC = ns3::Mac48Address::Allocate();

  wdsWifiMac.SetType("ns3::WDSWifiMac", "ReceiverAddress",
              Mac48AddressValue(curt3WestAPMAC));
  NetDeviceContainer curt2EastAPDevice = wifi.Install(phy, wdsWifiMac, wifiCurtNodes.Get(2));
  curt2EastAPDevice.Get(0)->SetAddress(curt2EastAPMAC);

  wdsWifiMac.SetType("ns3::WDSWifiMac", "ReceiverAddress",
              Mac48AddressValue(curt2EastAPMAC));
  NetDeviceContainer curt3WestAPDevice = wifi.Install(phy, wdsWifiMac, wifiCurtNodes.Get(3));
  curt3WestAPDevice.Get(0)->SetAddress(curt3WestAPMAC);

// Curt3's east AP and Curt4's west AP
YansWifiChannelHelper wds3Channel = YansWifiChannelHelper::Default ();
phy.SetChannel (wds3Channel.Create ());
phy.Set ("ChannelNumber", ns3::UintegerValue(13));
  Mac48Address curt3EastAPMAC = ns3::Mac48Address::Allocate();
  Mac48Address curt4WestAPMAC = ns3::Mac48Address::Allocate();

  wdsWifiMac.SetType("ns3::WDSWifiMac", "ReceiverAddress",
              Mac48AddressValue(curt4WestAPMAC));
  NetDeviceContainer curt3EastAPDevice = wifi.Install(phy, wdsWifiMac, wifiCurtNodes.Get(3));
  curt3EastAPDevice.Get(0)->SetAddress(curt3EastAPMAC);

  wdsWifiMac.SetType("ns3::WDSWifiMac", "ReceiverAddress",
              Mac48AddressValue(curt3EastAPMAC));
  NetDeviceContainer curt4WestAPDevice = wifi.Install(phy, wdsWifiMac, wifiCurtNodes.Get(4));
  curt4WestAPDevice.Get(0)->SetAddress(curt4WestAPMAC);

// Curt4's east AP and Curt5's west AP
  YansWifiChannelHelper wds4Channel = YansWifiChannelHelper::Default ();
  phy.SetChannel (wds4Channel.Create ());
  phy.Set ("ChannelNumber", ns3::UintegerValue(18));

  Mac48Address curt4EastAPMAC = ns3::Mac48Address::Allocate();
  Mac48Address curt5WestAPMAC = ns3::Mac48Address::Allocate();

  wdsWifiMac.SetType("ns3::WDSWifiMac", "ReceiverAddress",
              Mac48AddressValue(curt5WestAPMAC));
  NetDeviceContainer curt4EastAPDevice = wifi.Install(phy, wdsWifiMac, wifiCurtNodes.Get(4));
  curt4EastAPDevice.Get(0)->SetAddress(curt4EastAPMAC);

  wdsWifiMac.SetType("ns3::WDSWifiMac", "ReceiverAddress",
              Mac48AddressValue(curt4EastAPMAC));
  NetDeviceContainer curt5WestAPDevice = wifi.Install(phy, wdsWifiMac, wifiCurtNodes.Get(5));
  curt5WestAPDevice.Get(0)->SetAddress(curt5WestAPMAC);

// Curt4's east AP and Curt5's west AP
  YansWifiChannelHelper wds5Channel = YansWifiChannelHelper::Default ();
  phy.SetChannel (wds5Channel.Create ());
  phy.Set ("ChannelNumber", ns3::UintegerValue(22));

  Mac48Address curt5EastAPMAC = ns3::Mac48Address::Allocate();
  Mac48Address curt6WestAPMAC = ns3::Mac48Address::Allocate();

  wdsWifiMac.SetType("ns3::WDSWifiMac", "ReceiverAddress",
              Mac48AddressValue(curt6WestAPMAC));
  NetDeviceContainer curt5EastAPDevice = wifi.Install(phy, wdsWifiMac, wifiCurtNodes.Get(5));
  curt5EastAPDevice.Get(0)->SetAddress(curt5EastAPMAC);

  wdsWifiMac.SetType("ns3::WDSWifiMac", "ReceiverAddress",
              Mac48AddressValue(curt5EastAPMAC));
  NetDeviceContainer curt6WestAPDevice = wifi.Install(phy, wdsWifiMac, wifiApNode);
  curt6WestAPDevice.Get(0)->SetAddress(curt6WestAPMAC);

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (15.0),
                                 "DeltaY", DoubleValue (20.0),
                                 "GridWidth", UintegerValue (6),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (wifiCliNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiCurtNodes);
  mobility.Install (wifiApNode);

// install rawsocket file descriptor for access to the InternetStackHelper
  std::string deviceName ("enp0s3");
  std::string remote ("93.184.216.34");
  Ipv4Address remoteIp (remote.c_str ());
  Ipv4Address localIp ("10.0.2.20");
  Ipv4Mask localMask ("255.255.255.0");
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  EmuFdNetDeviceHelper emu;
  emu.SetDeviceName (deviceName);
  NetDeviceContainer emuDevice = emu.Install (wifiCurtNodes.Get(3));
  emuDevice.Get(0)->SetAttribute("Address", Mac48AddressValue (
                                  Mac48Address::Allocate ()));

  InternetStackHelper stack;
  stack.Install (csmaNodes);
  stack.Install (wifiApNode);
  stack.Install (wifiCurtNodes);
  stack.Install (wifiCliNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  address.SetBase ("10.1.12.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  address.Assign (cliDevices);
  address.Assign (curtDevices);

  /*Ptr<Ipv4> ipv4Router = wifiCurtNodes.Get(0)->GetObject<Ipv4> ();
  uint32_t ifIndex = ipv4Router->AddInterface (curtDevices.Get(0));
  ipv4Router->AddAddress (ifIndex, Ipv4InterfaceAddress (Ipv4Address ("10.1.3.1"), Ipv4Mask ("/24")));
  ipv4Router->SetForwarding(ifIndex, true);
  ipv4Router->SetUp (ifIndex);*/

  address.SetBase ("10.1.4.0", "255.255.255.0");
  address.Assign (curt0EastAPDevice);
  address.Assign (curt1WestAPDevice);
  address.SetBase ("10.1.5.0", "255.255.255.0");
  address.Assign (curt1EastAPDevice);
  address.Assign (curt2WestAPDevice);
  address.SetBase ("10.1.6.0", "255.255.255.0");
  address.Assign (curt2EastAPDevice);
  address.Assign (curt3WestAPDevice);
  address.SetBase ("10.1.7.0", "255.255.255.0");
  address.Assign (curt3EastAPDevice);
  address.Assign (curt4WestAPDevice);
  address.SetBase ("10.1.8.0", "255.255.255.0");
  address.Assign (curt4EastAPDevice);
  address.Assign (curt5WestAPDevice);
  address.SetBase ("10.1.9.0", "255.255.255.0");
  address.Assign (curt5EastAPDevice);
  address.Assign (curt6WestAPDevice);

// emufd Interface
  /*Ptr<Ipv4> ipv4 = wifiCurtNodes.Get(3)->GetObject<Ipv4> ();
  uint32_t interface = ipv4->AddInterface (emuDevice.Get(0));
  Ipv4InterfaceAddress emuAddress = Ipv4InterfaceAddress (localIp, localMask);
  ipv4->AddAddress (interface, emuAddress);
  ipv4->SetMetric (interface, 1);
  ipv4->SetUp (interface);*/

// local and global routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

/*  Ipv4Address gateway ("10.0.2.2");
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> staticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4);
  staticRouting->SetDefaultRoute (gateway, interface);*/

// Appliation creation
  /*uint16_t dhcp_port = 67;
  DhcpServerHelper dhcp_server(Ipv4Address("10.1.3.0"), Ipv4Mask("/24"),
                                Ipv4Address("10.1.3.1"), dhcp_port);
  ApplicationContainer apDhcpServer = dhcp_server.Install(wifiCurtNodes.Get(0));
  apDhcpServer.Start (Seconds(1.0));
  apDhcpServer.Stop (Seconds(10.0));

  DhcpClientHelper dhcp_client(dhcp_port);
  ApplicationContainer apDhcpClient = dhcp_client.Install(wifiCliNodes.Get(0));
  apDhcpClient.Start (Seconds(1.0));
  apDhcpClient.Stop (Seconds(10.0));*/

  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (nCsma));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (nCsma), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps =
  echoClient.Install (wifiCliNodes.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));
/*
  Ptr<V4Ping> app = CreateObject<V4Ping> ();
  app->SetAttribute ("Remote", Ipv4AddressValue (remoteIp));
  app->SetAttribute ("Verbose", BooleanValue (true) );
  wifiCliNodes.Get(0)->AddApplication (app);
  app->SetStartTime (Seconds (3.0));
  app->SetStopTime (Seconds (21.0));*/

  Simulator::Stop (Seconds (10.0));

  if (tracing == true)
    {
      //pointToPoint.EnablePcapAll ("third");
      //phy.EnablePcap ("third", curtDevices.Get (0));
      //csma.EnablePcap ("third", csmaDevices.Get (0), true);
    }

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
