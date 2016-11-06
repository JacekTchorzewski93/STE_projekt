#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Siec1");

int main (int argc, char *argv[]) {
  CommandLine cmd;
  cmd.Parse (argc, argv);

  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  /* Stworzenie węzłów */
  Ptr<Node> r1 = CreateObject<Node>();
  Ptr<Node> r2 = CreateObject<Node>();
  Ptr<Node> r3 = CreateObject<Node>();
  Ptr<Node> r4 = CreateObject<Node>();
  Ptr<Node> client = CreateObject<Node>();
  Ptr<Node> server = CreateObject<Node>();

  NodeContainer nodeContainer;
  nodeContainer.Add(r1);
  nodeContainer.Add(r2);
  nodeContainer.Add(r3);
  nodeContainer.Add(r4);
  nodeContainer.Add(client);
  nodeContainer.Add(server);

  /* PointToPointHelper */
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  /* Stworzenie kontenera dla każdej pary połączonych */
  NetDeviceContainer client_r1, r1_r2, r1_r3, r1_r4, r2_r3, r2_r4, r3_r4, r4_server;
  client_r1 = pointToPoint.Install(client, r1); // 10.1.0.0
  r1_r2 = pointToPoint.Install(r1, r2); // 10.1.1.0
  r1_r3 = pointToPoint.Install(r1, r3); // 10.1.2.0
  r1_r4 = pointToPoint.Install(r1, r4); // 10.1.3.0
  r2_r3 = pointToPoint.Install(r2, r3); // 10.1.4.0
  r2_r4 = pointToPoint.Install(r2, r4); // 10.1.5.0
  r3_r4 = pointToPoint.Install(r3, r4); // 10.1.6.0
  r4_server = pointToPoint.Install(r4, server); // 10.1.7.0

  NetDeviceContainer netDeviceContainers[8] = {client_r1, r1_r2, r1_r3, r1_r4, r2_r3, r2_r4, r3_r4, r4_server};

  /* Instalujemy zestaw protokołów na wszystkich node'ach */
  InternetStackHelper stack;
  stack.InstallAll();

  /* Przypisujemy adresy dla każdej podsieci (każda podsieć w naszym wypadku zawiera dla node'y) */
  Ipv4AddressHelper address;

  for (int i = 0; i < 8; i++) {
    std::string base = "10.1." + std::to_string(i) + ".0";
    address.SetBase(base.c_str(), "255.255.255.0");
    address.Assign(netDeviceContainers[i]);
  }

  /*
    Konfigurujemy routing.
    Tylko jedna ścieżka client -> R1 -> R4 -> server.
  */
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> staticRouting_client = ipv4RoutingHelper.GetStaticRouting(client->GetObject<Ipv4>());
  Ptr<Ipv4StaticRouting> staticRouting_r1 = ipv4RoutingHelper.GetStaticRouting(r1->GetObject<Ipv4>());
  Ptr<Ipv4StaticRouting> staticRouting_r4 = ipv4RoutingHelper.GetStaticRouting(r4->GetObject<Ipv4>());

  staticRouting_client->AddHostRouteTo(Ipv4Address("10.1.7.2"), 1);
  staticRouting_r1->AddHostRouteTo(Ipv4Address("10.1.7.2"), 4);
  staticRouting_r4->AddHostRouteTo(Ipv4Address("10.1.7.2"), 4);

  /* Ustawiamy server */
  UdpEchoServerHelper echoServer(9);

  ApplicationContainer serverApps = echoServer.Install(server);
  serverApps.Start(Seconds(1.0));
  serverApps.Stop(Seconds(10.0));

  /* Ustawiamy klient */
  UdpEchoClientHelper echoClient(Ipv4Address("10.1.7.2"), 9);
  echoClient.SetAttribute("MaxPackets", UintegerValue(1));
  echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
  echoClient.SetAttribute("PacketSize", UintegerValue(1024));

  ApplicationContainer clientApps = echoClient.Install(client);
  clientApps.Start(Seconds(2.0));
  clientApps.Stop(Seconds(10.0));

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
