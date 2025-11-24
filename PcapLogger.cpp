#include "PcapLogger.hpp"
#include <pcap/dlt.h>
#include <stdexcept>
#include <ctime>
#include <iostream>
#include <iomanip>


PcapLogger::PcapLogger(const std::string& filename) {
    pcap_handle = pcap_open_dead(DLT_RAW, 65535);
    if (!pcap_handle) throw std::runtime_error("Failed to create pcap handle");

    pcap_dumper = pcap_dump_open(pcap_handle, filename.c_str());
    if (!pcap_dumper) throw std::runtime_error("Failed to open pcap file");
}

void PcapLogger::logPacket(const std::vector<uint8_t>& packet, const std::string& comment) {
    pcap_pkthdr header;
    header.ts.tv_sec = std::time(nullptr);
    header.ts.tv_usec = 0;
    header.caplen = static_cast<bpf_u_int32>(packet.size());
    header.len = static_cast<bpf_u_int32>(packet.size());
  
    printPacketHex(packet);
  
    pcap_dump((u_char*)pcap_dumper, &header, packet.data());
}

void PcapLogger::logRawPacket(const std::vector<uint8_t>& packet, const std::string& comment) {
    pcap_pkthdr header;
    header.ts.tv_sec = std::time(nullptr);
    header.ts.tv_usec = 0;
    header.caplen = static_cast<bpf_u_int32>(packet.size());
    header.len = static_cast<bpf_u_int32>(packet.size());
    
    printPacketHex(packet);
    
    pcap_dump((u_char*)pcap_dumper, &header, packet.data());
}



// Define the printPacketHex function in the .cpp file
void PcapLogger::printPacketHex(const std::vector<uint8_t>& packet) {
    for (size_t i = 0; i < packet.size(); ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << (int)packet[i] << " ";
        if ((i + 1) % 16 == 0)  // Newline every 16 bytes
            std::cout << std::endl;
    }
    std::cout << std::dec << std::endl;  // Reset to decimal after printing hex
}



PcapLogger::~PcapLogger() {
    if (pcap_dumper) pcap_dump_close(pcap_dumper);
    if (pcap_handle) pcap_close(pcap_handle);
}
