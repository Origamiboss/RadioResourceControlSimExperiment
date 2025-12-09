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

    int dummyPacketCount = 0;
    

public:
    static constexpr int T300_MS = 200;   // Wait time for RRC Setup
    static constexpr int MAX_RRC_RETRIES = 4;

    UeRrc(PacketBuffer* myBuffer, PacketBuffer* theirBuffer);

    ~UeRrc();

    void sendRrcConnectionRequest();

    void receiveRrcConnectionSetup();

    void sendRrcConnectionComplete();

    void sendDummyData();

    void receiveRrcRelease();

    void checkForPackets();

    RrcState getState();

    int retrievedDummyPackets();

    void log(const std::string& msg);
    
    [[maybe_unused]] RrcState getState() const;
};
