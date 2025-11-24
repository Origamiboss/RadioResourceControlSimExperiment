/**
 * @class PcapLogger
 * @brief Handles packet capture (PCAP) logging for protocol analysis
 *
 * Creates Wireshark-compatible packet captures of RRC protocol messages.
 * Uses DLT_RAW link type for simplicity in simulation environment.
 */

#pragma once
#include <pcap.h>
#include <vector>
#include <string>

class PcapLogger {
private:
    pcap_t* pcap_handle;
    pcap_dumper_t* pcap_dumper;
    void printPacketHex(const std::vector<uint8_t>& packet);
public:
    PcapLogger(const std::string& filename);
    ~PcapLogger();
    void logPacket(const std::vector<uint8_t>& packet, const std::string& comment);
    void logRawPacket(const std::vector<uint8_t>& packet, const std::string& comment);
};
