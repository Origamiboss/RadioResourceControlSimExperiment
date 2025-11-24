#pragma once

#include <string>
#include <fstream>
#include "PcapLogger.hpp"
#include "PDcp.hpp"
    


enum class RrcState {
    RRC_IDLE, RRC_CONNECTING, RRC_CONNECTED
};

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

    [[maybe_unused]] RrcState getState() const;
};