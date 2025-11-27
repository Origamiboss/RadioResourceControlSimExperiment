#pragma once

#include <string>
#include <fstream>
#include "PcapLogger.hpp"
#include "PDcp.hpp"
#include "RrcState.hpp"
#include "PacketBuffer.hpp"

class CentralUnit {
private:
    std::unique_ptr<pdcp::PDcp> pdcp_;
    RrcState state = RrcState::RRC_IDLE;
    std::ofstream logFile;
    PcapLogger pcapLogger{"../Logs/ue_rrc.pcap"};

    //Packet Queue
    PacketBuffer* f1cBuffer; // To NetworkRrc
    PacketBuffer* f1uBuffer; // To UE
    PacketBuffer* theirBuffer;

    int optionType;

    
    int computeFronthaulDelayUs(size_t sizeBytes);
    int computeProcessingDelayUs();
    int computeUuDelayUs();
    void applyLatency(int sizeBytes);
public:
    CentralUnit(PacketBuffer* f1cBuffer, PacketBuffer* f1uBuffer, int optionType);

    void checkForPackets();

    void sendRrcSetup();

    void receiveRrcConnectionComplete();

    void sendRrcRelease();

    void sendRrcConnectionSetup();

    void receiveRrcConnectionRequest();
    
    [[maybe_unused]] RrcState getState() const;
};
