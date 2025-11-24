#include "NetworkRrc.hpp"
#include <iostream>
#include <fstream>
#include "PDcp.hpp"
#include "PcapLogger.hpp"
#include "Utils.hpp"


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
    }
}

void NetworkRrc::sendRrcConnectionSetup() {
    if (state == RrcState::RRC_CONNECTING) {

        pdcp::PDcp::Bytes payload = {0x50, 0xAA};
        auto pdcpPacket = pdcp_->encapsulate(payload);

        pcapLogger.logPacket(pdcpPacket, "PDCP (RRC Connection Setup)");

        logFile << "[" << getCurrentTimestamp() << "] [Network → UE] Sent RRCConnectionSetup\n";
        std::cout << "Sent RRCConnectionSetup\n";

        theirBuffer.sendPacket(pdcpPacket);

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

        theirBuffer.sendPacket(pdcpPacket);

        state = RrcState::RRC_IDLE;
    }
}

void NetworkRrc::checkForPackets() {
    //Check for a packet
    if(myBuffer.empty()) return;
    auto optPacket = myBuffer.getPacket();
    std::optional<Bytes> payload = pdcp_->onReceive(optPacket);
    
    if(!payload) return;
    
    //Data is already unencrypted

    auto data = *payload;
    //Determine if the packet is ours
    if (data == Bytes{0x40, 0x12}) {
        receiveRrcConnectionRequest();
    }
    else if (data == Bytes{0x43, 0x34}) {
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

