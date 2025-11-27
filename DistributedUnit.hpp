#pragma once

#include <string>
#include <fstream>
#include "PcapLogger.hpp"
#include "PDcp.hpp"
#include "RrcState.hpp"
#include "PacketBuffer.hpp"
#include "LatencyModel.hpp"

class DistributedUnit {
private:
    std::unique_ptr<pdcp::PDcp> pdcp_;
    std::ofstream logFile;
    PcapLogger pcapLogger{"../Logs/ue_rrc.pcap"};

    //Packet Queue
    PacketBuffer* f1cBuffer; // Control plane
    PacketBuffer* f2cBuffer; // Control plane Output
    PacketBuffer* f1uBuffer; // Data plane (if you add PDCP later)
    PacketBuffer* f2uBuffer; // Data plane Output (if you add PDCP later)
    PacketBuffer* theirBuffer;

    int optionType;
    LatencyModel latency;

    //private functions
    pdcp::PDcp::Bytes processDownlink(pdcp::PDcp::Bytes& raw);
    pdcp::PDcp::Bytes proccessUpLink(pdcp::PDcp::Bytes& raw);
    int computeFronthaulDelayUs(size_t packetSizeBytes);
    int computeUuDelayUs();
    int computeProcessingDelayUs();

public:
    DistributedUnit(PacketBuffer* f1cBuffer, PacketBuffer* f2cBuffer, PacketBuffer* f1uBuffer, PacketBuffer* f2uBuffer, int optionType);

    void checkForPackets();

    void checkForUePackets();

    void checkForCuPackets();

    
    [[maybe_unused]] RrcState getState() const;
};
