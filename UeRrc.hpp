#pragma once

#include <string>
#include <fstream>
#include "PcapLogger.hpp"
#include "PDcp.hpp"
#include "RrcState.hpp"
#include "PacketBuffer.hpp"

class UeRrc {
private:
    std::unique_ptr<pdcp::PDcp> pdcp_;
    RrcState state = RrcState::RRC_IDLE;
    std::ofstream logFile;
    PcapLogger pcapLogger{"../Logs/ue_rrc.pcap"};

    //Packet Queue
    PacketBuffer* myBuffer;
    PacketBuffer* theirBuffer;

public:
    UeRrc(PacketBuffer* myBuffer, PacketBuffer* theirBuffer);

    ~UeRrc();

    void sendRrcConnectionRequest();

    void receiveRrcConnectionSetup();

    void sendRrcConnectionComplete();

    void sendDummyData();

    void receiveRrcRelease();

    void checkForPackets();

    RrcState getState();

    void log(const std::string& msg);
    
    [[maybe_unused]] RrcState getState() const;
};
