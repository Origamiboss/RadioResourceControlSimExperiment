#pragma once

#include <string>
#include <fstream>
#include <memory>
#include "PcapLogger.hpp"
#include "PDcp.hpp"
#include "RrcState.hpp"
#include "PacketBuffer.hpp"
#include "LatencyModel.hpp"

class CentralUnit {
private:
    std::unique_ptr<pdcp::PDcp> pdcp_;
    RrcState state = RrcState::RRC_IDLE;
    std::ofstream logFile;
    PcapLogger pcapLogger{"../Logs/ue_rrc.pcap"};

    // Packet queues
    PacketBuffer* f1cBuffer; // DU -> CU (uplink)
    PacketBuffer* f1uBuffer; // CU -> DU (downlink)

    int optionType;
    LatencyModel latency;

    // Delay model
    int computeFronthaulDelayUs(size_t sizeBytes);
    int computeProcessingDelayUs();
    int computeUuDelayUs();
    void applyLatency(int sizeBytes);

    // New processing helpers
    pdcp::PDcp::Bytes processUpLink(pdcp::PDcp::Bytes& raw);
    pdcp::PDcp::Bytes processDownlink(pdcp::PDcp::Bytes& raw);

public:
    CentralUnit(PacketBuffer* f1cBuffer, PacketBuffer* f1uBuffer, int optionType);

    void changeOptionType(int newType);

    void checkForPackets();

    void sendRrcSetup();
    void sendRrcRelease();
    void sendDummyData();

    void receiveRrcConnectionRequest();
    void receiveRrcConnectionComplete();

    [[maybe_unused]] RrcState getState() const { return state; }
};
