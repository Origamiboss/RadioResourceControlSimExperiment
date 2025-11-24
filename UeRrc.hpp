#pragma once

#include <string>
#include <fstream>
#include "PcapLogger.hpp"
#include "PDcp.hpp"
#include "RrcState.hpp"


class UeRrc {
private:
    std::unique_ptr<pdcp::PDcp> pdcp_;
    RrcState state = RrcState::RRC_IDLE;
    std::ofstream logFile;
    PcapLogger pcapLogger{"../Logs/ue_rrc.pcap"};

public:
    UeRrc();

    ~UeRrc();

    void sendRrcConnectionRequest();

    void receiveRrcConnectionSetup();

    void sendRrcConnectionComplete();

    void receiveRrcRelease();

    void receiveFromNetwork(const std::vector<uint8_t>& rawPacket);

    void log(const std::string& msg);
    
    [[maybe_unused]] RrcState getState() const;
};
