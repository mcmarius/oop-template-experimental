#include <iostream>
#include <array>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <sstream>
#include <iomanip>
#include "include/Example.h"

// PcapPlusPlus headers
#include "PcapLiveDeviceList.h"
#include "PcapLiveDevice.h"
#include "PcapFileDevice.h"
#include "Packet.h"
#include "EthLayer.h"
#include "IPv4Layer.h"
#include "TcpLayer.h"
#include "UdpLayer.h"
#include "PayloadLayer.h"
#include "IPLayer.h"
#include "PcapFilter.h"
#include "NetworkUtils.h"
#include "MacAddress.h"

// ============================================================================
// Example 1: List all network interfaces
// ============================================================================
int example_list_interfaces()
{
    std::cout << "=== Example 1: List Network Interfaces ===\n\n";

    const auto& devList = pcpp::PcapLiveDeviceList::getInstance();
    auto devices = devList.getPcapLiveDevicesList();

    std::cout << "Found " << devices.size() << " live device(s):\n\n";

    for (const auto* dev : devices) {
        std::cout << "  Interface: " << dev->getName() << "\n";
        std::cout << "    Description: " << dev->getDesc() << "\n";
        std::cout << "    MAC Address: " << dev->getMacAddress() << "\n";
        std::cout << "    MTU: " << dev->getMtu() << "\n";
        std::cout << "    Is Loopback: " << (dev->getLoopback() ? "yes" : "no") << "\n";

        const auto& ips = dev->getIPAddresses();
        if (!ips.empty()) {
            std::cout << "    IP Addresses:\n";
            for (const auto& addr : ips) {
                std::cout << "      - " << addr << "\n";
            }
        }
        std::cout << "\n";
    }

    return 0;
}

// ============================================================================
// Example 2: Create a pcap file with a synthetic Ethernet/IPv4/TCP packet
// ============================================================================
int example_create_pcap_file(const std::string& outputPath)
{
    std::cout << "=== Example 2: Create Synthetic Packet in PCAP ===\n\n";

    // --- Build a synthetic Ethernet + IPv4 + TCP + Payload packet ---
    pcpp::MacAddress srcMac("00:11:22:33:44:55");
    pcpp::MacAddress dstMac("AA:BB:CC:DD:EE:FF");
    pcpp::EthLayer ethLayer(srcMac, dstMac);

    pcpp::IPv4Address srcIp("192.168.1.100");
    pcpp::IPv4Address dstIp("93.184.216.34"); // example.com
    pcpp::IPv4Layer ipv4Layer(srcIp, dstIp);
    ipv4Layer.getIPv4Header()->timeToLive = 64;
    ipv4Layer.getIPv4Header()->protocol =
        static_cast<uint8_t>(pcpp::IPProtocolTypes::PACKETPP_IPPROTO_TCP);

    uint16_t srcPort = 12345;
    uint16_t dstPort = 80;
    pcpp::TcpLayer tcpLayer(srcPort, dstPort);

    // Simple HTTP GET payload
    std::string httpGet("GET / HTTP/1.1\r\nHost: example.com\r\n\r\n");
    pcpp::PayloadLayer payloadLayer(
        reinterpret_cast<const uint8_t*>(httpGet.c_str()),
        httpGet.size()
    );

 // Chain layers: Ethernet -> IPv4 -> TCP -> Payload
    pcpp::Packet packet(/* maxLen */ 1500);
    // Set EtherType to IPv4 (0x0800) BEFORE adding layer, since addLayer
    // writes the header data into the packet buffer
    ethLayer.getEthHeader()->etherType = 0x0800;
    packet.addLayer(&ethLayer);
    packet.addLayer(&ipv4Layer);
    packet.addLayer(&tcpLayer);
    packet.addLayer(&payloadLayer);

    // Calculate checksums
    ipv4Layer.computeCalculateFields();
    tcpLayer.computeCalculateFields();

    // --- Write to pcap file ---
    // PcapFileWriterDevice constructor takes LinkLayerType and TimestampPrecision
    pcpp::PcapFileWriterDevice writer(outputPath, pcpp::LINKTYPE_ETHERNET);
    if (!writer.open()) {
        std::cerr << "Error: Could not open output file: " << outputPath << "\n";
        return 1;
    }

    // Get the RawPacket from the Packet for writing
    const pcpp::RawPacket* rawPkt = packet.getRawPacket();
    if (writer.writePacket(*rawPkt)) {
        std::cout << "Successfully wrote 1 packet to " << outputPath << "\n";
    } else {
        std::cerr << "Error: Failed to write packet\n";
        writer.close();
        return 1;
    }

    writer.close();

    // Print packet info
    std::cout << "  Packet details:\n";
    std::cout << "    Ethernet: " << srcMac << " -> " << dstMac
              << " (EtherType: 0x" << std::hex
              << ethLayer.getEthHeader()->etherType << ")\n";
    std::cout << "    IPv4:     " << srcIp << " -> " << dstIp
              << " (TTL: " << std::dec
              << static_cast<int>(ipv4Layer.getIPv4Header()->timeToLive)
              << ", Protocol: "
              << static_cast<int>(ipv4Layer.getIPv4Header()->protocol)
              << ")\n";
    std::cout << "    TCP:      port " << srcPort << " -> " << dstPort
              << " (Flags: ";
    const auto* th = tcpLayer.getTcpHeader();
    if (th->finFlag) std::cout << "FIN ";
    if (th->synFlag) std::cout << "SYN ";
    if (th->rstFlag) std::cout << "RST ";
    if (th->pshFlag) std::cout << "PSH ";
    if (th->ackFlag) std::cout << "ACK ";
    if (th->urgFlag) std::cout << "URG ";
    std::cout << ")\n";
    std::cout << "    Payload:  " << httpGet.size() << " bytes (" << httpGet
              << ")\n";
    std::cout << "    Total packet size: " << rawPkt->getRawDataLen()
              << " bytes\n\n";

    return 0;
}

// ============================================================================
// Example 3: Read and parse a pcap file
// ============================================================================
int example_read_pcap_file(const std::string& inputPath)
{
    std::cout << "=== Example 3: Read and Parse PCAP File ===\n\n";

    pcpp::IFileReaderDevice* reader =
        pcpp::IFileReaderDevice::getReader(inputPath);
    if (!reader || !reader->open()) {
        std::cerr << "Error: Could not open file: " << inputPath << "\n";
        delete reader;
        return 1;
    }

    std::cout << "File: " << reader->getFileName() << "\n";

    // Cast to PcapFileReaderDevice to access getLinkLayerType()
    const auto* pcapReader = dynamic_cast<pcpp::PcapFileReaderDevice*>(reader);
    if (pcapReader) {
        std::cout << "Link layer type: "
                  << static_cast<int>(pcapReader->getLinkLayerType()) << "\n";
    }
    std::cout << "\n";

    int packetCount = 0;
    pcpp::RawPacket rawPacket;

    while (reader->getNextPacket(rawPacket)) {
        packetCount++;

       // Parse the full packet (no parseUntil limit)
        pcpp::Packet packet(&rawPacket);

        std::cout << "Packet #" << packetCount << ": "
                  << rawPacket.getRawDataLen() << " bytes, "
                  << "timestamp: " << rawPacket.getPacketTimeStamp().tv_sec
                  << "." << std::setfill('0') << std::setw(9)
                  << rawPacket.getPacketTimeStamp().tv_nsec << "\n";

        // Walk the layer chain
        std::cout << "  Layers: ";
        const pcpp::Layer* layer = packet.getFirstLayer();
        while (layer) {
            std::cout << static_cast<int>(layer->getProtocol());
            if (layer->getNextLayer()) std::cout << " -> ";
            layer = layer->getNextLayer();
        }
        std::cout << "\n";

        // Parse Ethernet
        auto* eth = packet.getLayerOfType<pcpp::EthLayer>();
        if (eth) {
            std::cout << "  Ethernet: " << eth->getSourceMac()
                      << " -> " << eth->getDestMac() << "\n";
        }

        // Parse IPv4
        auto* ipv4 = packet.getLayerOfType<pcpp::IPv4Layer>();
        if (ipv4) {
            std::cout << "  IPv4:     "
                      << ipv4->getSrcIPv4Address() << " -> "
                      << ipv4->getDstIPv4Address() << "\n";
            std::cout << "        Protocol: "
                      << static_cast<int>(ipv4->getIPv4Header()->protocol)
                      << " (TTL="
                      << static_cast<int>(ipv4->getIPv4Header()->timeToLive)
                      << ")\n";
        }

        // Parse TCP (needs full parse)
        pcpp::Packet fullPacket(&rawPacket);
        auto* tcp = fullPacket.getLayerOfType<pcpp::TcpLayer>();
        if (tcp) {
            std::cout << "  TCP:      port " << tcp->getSrcPort()
                      << " -> " << tcp->getDstPort()
                      << " (data offset="
                      << static_cast<int>(tcp->getTcpHeader()->dataOffset)
                      << ")\n";
        }

        // Parse UDP
        auto* udp = fullPacket.getLayerOfType<pcpp::UdpLayer>();
        if (udp) {
            std::cout << "  UDP:      port " << udp->getSrcPort()
                      << " -> " << udp->getDstPort() << "\n";
        }

        // Parse Payload
        auto* payload = fullPacket.getLayerOfType<pcpp::PayloadLayer>();
        if (payload) {
            std::cout << "  Payload:  " << payload->getPayloadLen()
                      << " bytes\n";
        }

        std::cout << "\n";

        // Limit to first 10 packets for demo
        if (packetCount >= 10) {
            std::cout << "  ... (showing first 10 of many packets)\n";
            break;
        }
    }

    pcpp::IPcapDevice::PcapStats stats;
    reader->getStatistics(stats);
    std::cout << "Total packets read: " << stats.packetsRecv << "\n\n";

    reader->close();
    delete reader;
    return 0;
}

// ============================================================================
// Example 4: Packet filtering with BPF filter
// ============================================================================
int example_packet_filter()
{
    std::cout << "=== Example 4: Packet Filtering ===\n\n";

    // Set up a BPF filter
    pcpp::BpfFilterWrapper filter;

    // Test various filter strings
    std::vector<std::string> filterTests = {
        "ip",
        "tcp",
        "udp",
        "tcp port 80",
        "ip host 192.168.1.100",
        "tcp and port 443",
    };

    for (const auto& filterStr : filterTests) {
        bool compiled = filter.setFilter(filterStr, pcpp::LINKTYPE_ETHERNET);
        std::cout << "  Filter '" << filterStr << "': "
                  << (compiled ? "compiled OK" : "compilation FAILED") << "\n";
    }

    // Compile a filter and match it against synthetic packets
    std::cout << "\n  Testing 'ip and tcp' against synthetic packets:\n";
    filter.setFilter("ip and tcp", pcpp::LINKTYPE_ETHERNET);

    // Build a synthetic IP+TCP packet
    pcpp::MacAddress smac("11:22:33:44:55:66");
    pcpp::MacAddress dmac("66:55:44:33:22:11");
    pcpp::EthLayer eth1(smac, dmac);
    pcpp::IPv4Layer ipv4_1(pcpp::IPv4Address("10.0.0.1"),
                           pcpp::IPv4Address("10.0.0.2"));
    ipv4_1.getIPv4Header()->protocol =
        static_cast<uint8_t>(pcpp::IPProtocolTypes::PACKETPP_IPPROTO_TCP);
    pcpp::TcpLayer tcp1(8080, 443);
    pcpp::Packet pkt1(1500);
    pkt1.addLayer(&eth1);
    pkt1.addLayer(&ipv4_1);
    pkt1.addLayer(&tcp1);
    ipv4_1.computeCalculateFields();

    pcpp::RawPacket* raw1 = pkt1.getRawPacket();
    bool matches1 = filter.matchPacketWithFilter(raw1);
    std::cout << "    matches 'ip and tcp' (TCP pkt): "
              << (matches1 ? "yes" : "no") << "\n";

    // Build a non-matching UDP packet
    pcpp::EthLayer eth2(smac, dmac);
    pcpp::IPv4Layer ipv4_2(pcpp::IPv4Address("10.0.0.1"),
                           pcpp::IPv4Address("10.0.0.2"));
    ipv4_2.getIPv4Header()->protocol =
        static_cast<uint8_t>(pcpp::IPProtocolTypes::PACKETPP_IPPROTO_UDP);
    pcpp::UdpLayer udp1(9000, 53);
    pcpp::Packet pkt2(1500);
    pkt2.addLayer(&eth2);
    pkt2.addLayer(&ipv4_2);
    pkt2.addLayer(&udp1);

    const pcpp::RawPacket* raw2 = pkt2.getRawPacket();
    bool matches2 = filter.matchPacketWithFilter(raw2);
    std::cout << "    matches 'ip and tcp' (UDP pkt): "
              << (matches2 ? "yes" : "no") << "\n\n";

    // Also test direct data matching (timespec version)
    std::cout << "  Direct data matching:\n";
    const uint8_t* data = raw1->getRawData();
    int len = raw1->getRawDataLen();
    timespec ts = raw1->getPacketTimeStamp();
    bool matchesDirect =
        filter.matchPacketWithFilter(data, static_cast<uint32_t>(len),
                                     ts,
                                     static_cast<uint16_t>(pcpp::LINKTYPE_ETHERNET));
    std::cout << "    TCP packet data matches 'ip and tcp': "
              << (matchesDirect ? "yes" : "no") << "\n\n";

    return 0;
}

// ============================================================================
// Example 5: Network utilities and address helpers
// ============================================================================
int example_network_utils()
{
    std::cout << "=== Example 5: Network & Address Utilities ===\n\n";

    // IP address comparison and manipulation
    std::cout << "IPv4 utilities:\n";
    pcpp::IPv4Address addr1("192.168.1.1");
    pcpp::IPv4Address addr2("192.168.1.100");
    pcpp::IPv4Address addr3("192.168.1.200");
    std::cout << "  " << addr1 << "\n";
    std::cout << "  " << addr2 << "\n";
    std::cout << "  " << addr3 << "\n";
    std::cout << "  " << addr2 << " < " << addr3 << ": "
              << (addr2 < addr3 ? "true" : "false") << "\n";
    std::cout << "  " << addr2 << " == " << addr2 << ": "
              << (addr2 == addr2 ? "true" : "false") << "\n\n";

    // IP address string parsing
    pcpp::IPAddress addr4("10.0.0.1");
    std::cout << "  Parsed address: " << addr4 << "\n";
    std::cout << "  Address family: "
              << (addr4.isIPv4() ? "IPv4" : "IPv6") << "\n\n";

    // MAC address helpers
    std::cout << "MAC address:\n";
    pcpp::MacAddress mac1("00:0A:95:D6:12:34");
    std::cout << "  " << mac1 << "\n";
    std::cout << "  Broadcast constant: " << pcpp::MacAddress::Broadcast
              << "\n";
    std::cout << "  Zero constant: " << pcpp::MacAddress::Zero << "\n";
    std::cout << "  MAC == Broadcast: "
              << (mac1 == pcpp::MacAddress::Broadcast ? "true" : "false")
              << "\n";
    std::cout << "  Raw bytes: ";
    const uint8_t* raw = mac1.getRawData();
    for (int i = 0; i < 6; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(raw[i]);
        if (i < 5) std::cout << ":";
    }
    std::cout << std::dec << "\n\n";

    // NetworkUtils singleton (ARP/DNS resolution utilities)
    std::cout << "NetworkUtils singleton:\n";
    std::cout << "  Default timeout: " << pcpp::NetworkUtils::DefaultTimeout
              << " ms\n";
    std::cout << "  (ARP resolution and DNS lookup available for live use)\n\n";

    return 0;
}

// ============================================================================
// Example 6: Read the synthetic file we wrote and verify round-trip
// ============================================================================
int example_roundtrip(const std::string& path)
{
    std::cout << "=== Example 6: Round-trip (write + re-read) ===\n\n";

    pcpp::IFileReaderDevice* reader =
        pcpp::IFileReaderDevice::getReader(path);
    if (!reader || !reader->open()) {
        std::cerr << "Error: Could not re-open " << path << "\n";
        delete reader;
        return 1;
    }

    pcpp::RawPacket rawPacket;
    int count = 0;
    while (reader->getNextPacket(rawPacket)) {
        count++;

        // Parse up to Payload layer
        pcpp::Packet packet(&rawPacket);

        std::cout << "Round-trip packet #" << count << ": "
                  << rawPacket.getRawDataLen() << " bytes\n";

        auto* eth = packet.getLayerOfType<pcpp::EthLayer>();
        auto* ipv4 = packet.getLayerOfType<pcpp::IPv4Layer>();
        auto* tcp = packet.getLayerOfType<pcpp::TcpLayer>();
        auto* payload = packet.getLayerOfType<pcpp::PayloadLayer>();

        if (eth) {
            std::cout << "  MAC: " << eth->getSourceMac()
                      << " -> " << eth->getDestMac() << "\n";
        }
        if (ipv4) {
            std::cout << "  IP: " << ipv4->getSrcIPv4Address()
                      << " -> " << ipv4->getDstIPv4Address() << "\n";
            std::cout << "  TTL: "
                      << static_cast<int>(ipv4->getIPv4Header()->timeToLive)
                      << ", Protocol: "
                      << static_cast<int>(ipv4->getIPv4Header()->protocol)
                      << "\n";
        }
        if (tcp) {
            std::cout << "  TCP: " << tcp->getSrcPort()
                      << " -> " << tcp->getDstPort() << "\n";
        }
        if (payload) {
            std::string data(
                reinterpret_cast<const char*>(payload->getPayload()),
                payload->getPayloadLen());
            std::cout << "  Payload (" << payload->getPayloadLen()
                      << " bytes): " << data << "\n";
        }
    }

    std::cout << "Read back " << count << " packet(s)\n\n";

    reader->close();
    delete reader;
    return (count == 1) ? 0 : 1;
}

// ============================================================================
// Main
// ============================================================================
int main()
{
    std::cout << "PcapPlusPlus MWE / Proof of Concept\n";
    std::cout << "=====================================\n\n";

    // --- Example 1: List interfaces ---
    example_list_interfaces();

    // --- Example 2: Create synthetic pcap ---
    std::string pcapPath = "mwe_test.pcap";
    int rc = example_create_pcap_file(pcapPath);
    if (rc != 0) {
        std::cerr << "Example 2 failed with code " << rc << "\n";
        return rc;
    }

    // --- Example 3: Read and parse ---
    example_read_pcap_file(pcapPath);

    // --- Example 4: Filtering ---
    example_packet_filter();

    // --- Example 5: Network utilities ---
    example_network_utils();

    // --- Example 6: Round-trip verification ---
    rc = example_roundtrip(pcapPath);
    if (rc != 0) {
        std::cerr << "Example 6 failed with code " << rc << "\n";
        return rc;
    }

    // Clean up generated pcap file
    // std::remove(pcapPath.c_str());  // Keep for debugging

    std::cout << "All examples passed!\n";
    std::cout << "PCAP file kept at: " << pcapPath << "\n";
    return 0;
}
