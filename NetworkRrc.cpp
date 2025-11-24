#include "NetworkRrc.hpp"
#include <iostream>
#include "PDcp.hpp"
#include "PcapLogger.hpp"

std::string getCurrentTimestamp();

NetworkRrc::NetworkRrc() {
    logFile.open("../Logs/network_rrc_log.txt");
    logFile << "[" << getCurrentTimestamp() << "] Network RRC Layer initialized (State: IDLE)\n";

    // -------------------------------
    // Initialize PDCP
    // -------------------------------
    pdcp_ = std::make_unique<pdcp::PDcp>("Network-PDCP");

    pdcp_->setDeliverCallback([this](const pdcp::PDcp::Bytes &payload) {
        this->log("[PDCP] Delivered payload of size " + std::to_string(payload.size()));
    });

    // Use network's own pcapLogger
    pdcp_->setPcapLogger(&pcapLogger);
}

NetworkRrc::~NetworkRrc() {
    logFile.close();
}

void NetworkRrc::receiveRrcConnectionRequest() {
    if (state == RrcState::RRC_IDLE) {
        state = RrcState::RRC_CONNECTING;

        logFile << "[" << getCurrentTimestamp() << "] [UE → Network] Received RRCConnectionRequest\n";
        std::cout << "Network received RRCConnectionRequest\n";
    }
}

void NetworkRrc::sendRrcConnectionSetup() {
    if (state == RrcState::RRC_CONNECTING) {

        pdcp::PDcp::Bytes payload = {0x50, 0xAA};
        auto pdcpPacket = pdcp_->encapsulate(payload);

        pcapLogger.logPacket(pdcpPacket, "PDCP (RRC Connection Setup)");

        logFile << "[" << getCurrentTimestamp() << "] [Network → UE] Sent RRCConnectionSetup\n";
        std::cout << "Sent RRCConnectionSetup\n";

        state = RrcState::RRC_CONNECTED;
    }
}

void NetworkRrc::receiveRrcConnectionComplete() {
    if (state == RrcState::RRC_CONNECTED) {

        logFile << "[" << getCurrentTimestamp() << "] [UE → Network] Received RRCConnectionComplete\n";
        std::cout << "Network received RRCConnectionComplete\n";
    }
}

void NetworkRrc::sendRrcRelease() {
    if (state == RrcState::RRC_CONNECTED) {

        pdcp::PDcp::Bytes payload = {0x5F, 0x21};
        auto pdcpPacket = pdcp_->encapsulate(payload);

        pcapLogger.logPacket(pdcpPacket, "PDCP (RRC Release)");

        logFile << "[" << getCurrentTimestamp() << "] [Network → UE] Sent RRCRelease\n";
        std::cout << "Sent RRCRelease\n";

        state = RrcState::RRC_IDLE;
    }
}

void NetworkRrc::receiveFromUe(const std::vector<uint8_t>& rawPacket) {
    pdcp_->onReceive(rawPacket);
}

RrcState NetworkRrc::getState() const {
    return state;
}
