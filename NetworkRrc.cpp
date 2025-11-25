#include "NetworkRrc.hpp"
#include <iostream>
#include <fstream>
#include "PDcp.hpp"
#include "PcapLogger.hpp"
#include "Utils.hpp"
#include <thread>

NetworkRrc::NetworkRrc(PacketBuffer* myBuffer, PacketBuffer* theirBuffer) : pcapLogger("network.pcap") {  // Provide a filename here
    this->myBuffer = myBuffer;
    this->theirBuffer = theirBuffer;

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
        sendRrcConnectionSetup();
    }
}

void NetworkRrc::sendRrcConnectionSetup() {
    if (state == RrcState::RRC_CONNECTING) {

        pdcp::PDcp::Bytes payload = {0x50, 0xAA};
        auto pdcpPacket = pdcp_->encapsulate(payload);

        pcapLogger.logPacket(pdcpPacket, "PDCP (RRC Connection Setup)");

        logFile << "[" << getCurrentTimestamp() << "] [Network → UE] Sent RRCConnectionSetup\n";
        std::cout << "Sent RRCConnectionSetup\n";

        theirBuffer->sendPacket(pdcpPacket);

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

        theirBuffer->sendPacket(pdcpPacket);

        state = RrcState::RRC_IDLE;
    }
}

void NetworkRrc::checkForPackets() {
    auto optPacket = myBuffer->waitForPacket();
    if (!optPacket) {
        std::cout << "No packet available in buffer.\n";
        return;
    }

    auto payload = pdcp_->onReceive(*optPacket);  // Assuming payload is a valid packet

    if (!payload) {
        std::cout << "No payload received from PDCP.\n";
        return;
    }

    std::cout << "Payload received: ";
    for (auto byte : *payload) {
        std::cout << std::hex << static_cast<int>(byte) << " ";
    }
    std::cout << std::dec << "\n";  // reset to decimal for other outputs

    // Process payload here
    auto data = *payload;

    if (data == Bytes{0x40, 0x12}) {
        receiveRrcConnectionRequest();
    } else if (data == Bytes{0x50, 0xAA}) {
        receiveRrcConnectionComplete();
    }
}


// Move the log function inside the class, making it a member function
void NetworkRrc::log(const std::string& message) {
    logFile << "[" << getCurrentTimestamp() << "] " << message << std::endl;
}

RrcState NetworkRrc::getState() const {
    return state;
}

