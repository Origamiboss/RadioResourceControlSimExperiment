/**
 * @class UeRrc
 * @brief Implements UE-side RRC protocol state machine
 */

#include "UeRrc.hpp"
#include <iostream>
#include "PDcp.hpp"
#include "PcapLogger.hpp"
#include "Utils.hpp"


UeRrc::UeRrc() {
    logFile.open("../Logs/ue_rrc_log.txt");
    logFile << "[" << getCurrentTimestamp() << "] UE RRC Layer initialized (State: IDLE)\n";

    // -------------------------------
    // Initialize PDCP
    // -------------------------------
    pdcp_ = std::make_unique<pdcp::PDcp>("UE-PDCP");

    pdcp_->setDeliverCallback([this](const pdcp::PDcp::Bytes &payload) {
        this->log("[PDCP] Delivered payload of size " + std::to_string(payload.size()));
    });

    // Use UE's own pcapLogger
    pdcp_->setPcapLogger(&pcapLogger);
}

UeRrc::~UeRrc() {
    logFile.close();
}

void UeRrc::sendRrcConnectionRequest() {
    if (state == RrcState::RRC_IDLE) {
        state = RrcState::RRC_CONNECTING;

        pdcp::PDcp::Bytes payload = {0x40, 0x12};
        auto pdcpPacket = pdcp_->encapsulate(payload);

        pcapLogger.logPacket(pdcpPacket, "PDCP (RRC Connection Request)");

        logFile << "[" << getCurrentTimestamp() << "] [UE → Network] sent RRCConnectionRequest\n";
        std::cout << "Sent RRCConnectionRequest\n";
    }
}

void UeRrc::receiveRrcConnectionSetup() {
    if (state == RrcState::RRC_CONNECTING) {
        state = RrcState::RRC_CONNECTED;

        logFile << "[" << getCurrentTimestamp() << "] [Network → UE] Received RRCConnectionSetup\n";
        std::cout << "Received RRCConnectionSetup\n";
    }
}

void UeRrc::sendRrcConnectionComplete() {
    if (state == RrcState::RRC_CONNECTED) {

        pdcp::PDcp::Bytes payload = {0x43, 0x34};
        auto pdcpPacket = pdcp_->encapsulate(payload);

        pcapLogger.logPacket(pdcpPacket, "PDCP (RRC Connection Complete)");

        logFile << "[" << getCurrentTimestamp() << "] [UE → Network] sent RRCConnectionComplete\n";
        std::cout << "Sent RRCConnectionComplete\n";
    }
}

void UeRrc::receiveRrcRelease() {
    if (state == RrcState::RRC_CONNECTED) {
        state = RrcState::RRC_IDLE;

        logFile << "[" << getCurrentTimestamp() << "] [Network → UE] RRC Release received, back to IDLE\n";
        std::cout << "Received RRCRelease\n";
    }
}

void UeRrc::receiveFromNetwork(const std::vector<uint8_t>& rawPacket) {
    pdcp_->onReceive(rawPacket);
}

void UeRrc::log(const std::string& msg) {
    logFile << "[" << getCurrentTimestamp() << "] " << msg << std::endl;
}

[[maybe_unused]] RrcState UeRrc::getState() const {
    return state;
}
