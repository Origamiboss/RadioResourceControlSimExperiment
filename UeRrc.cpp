/**
 * @class UeRrc
 * @brief Implements UE-side RRC protocol state machine
 */

#include "UeRrc.hpp"
#include <iostream>
#include "PDcp.hpp"
#include "PcapLogger.hpp"
#include "Utils.hpp"
#include <thread>

UeRrc::UeRrc(PacketBuffer* myBuffer, PacketBuffer* theirBuffer) {
    this->myBuffer = myBuffer;
    this->theirBuffer = theirBuffer;

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
        state = RrcState::RRC_REQUEST_SENT;

        pdcp::PDcp::Bytes payload = {0x40, 0x12};
        auto pdcpPacket = pdcp_->encapsulate(payload);

        pcapLogger.logPacket(pdcpPacket, "PDCP (RRC Connection Request)");

        logFile << "[" << getCurrentTimestamp() << "] [UE → Network] sent RRCConnectionRequest\n";
        std::cout << "Sent RRCConnectionRequest\n";

        theirBuffer->sendPacket(pdcpPacket);
    }
}

void UeRrc::receiveRrcConnectionSetup() {
    if (state == RrcState::RRC_REQUEST_SENT) {
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
        
        theirBuffer->sendPacket(pdcpPacket);
    }
}

void UeRrc::sendDummyData() {
    if (state == RrcState::RRC_CONNECTED) {
        const size_t dataSize = 1 * 1024 * 1024;  // 1 MB dummy packet
        pdcp::PDcp::Bytes payload;
        payload.resize(dataSize);

        // Fill with random bytes
        std::generate(payload.begin(), payload.end(), []() {
            return static_cast<uint8_t>(rand() % 256);
        });

        auto pdcpPacket = pdcp_->encapsulate(payload);
        pcapLogger.logPacket(pdcpPacket, "PDCP (Dummy Data)");

        logFile << "[" << getCurrentTimestamp() 
                << "] [UE → Network] sent Dummy Data (" << dataSize << " bytes)\n";

        std::cout << "Sent Dummy Data: " << dataSize << " bytes\n";

        theirBuffer->sendPacket(pdcpPacket);
    }
}

void UeRrc::receiveRrcRelease() {
    std::cout << "[UE] Received RRC Release\n";
    if (state == RrcState::RRC_CONNECTED) {
        state = RrcState::RRC_IDLE;
        logFile << "[" << getCurrentTimestamp() << "] [Network → UE] RRC Release received, back to IDLE\n";
    }
}

void UeRrc::checkForPackets() {
    if(myBuffer->empty()) return;
    std::cout << "[UE] Waiting for packets...\n";
    auto optPacket = myBuffer->getPacket();
    std::cout << "[UE] Got a packet\n";
    if (!optPacket) return;

    auto payload = pdcp_->onReceive(*optPacket);
    if (!payload) return;

    auto &data = *payload;

    if (data == Bytes{0x50, 0xAA}) {
        receiveRrcConnectionSetup();
        sendRrcConnectionComplete();
    }
    else if (data == Bytes{0x5F, 0x21}) {
        receiveRrcRelease();
    }else {
        cout << "[UE]: Recieved Dummy Data" << endl;
        dummyPacketCount++;
    }
}

RrcState UeRrc::getState() {
    return state;
}

int retrievedDummyPackets(){
    return dummyPacketCount;
}

void UeRrc::log(const std::string& msg) {
    logFile << "[" << getCurrentTimestamp() << "] " << msg << std::endl;
}

[[maybe_unused]] RrcState UeRrc::getState() const {
    return state;
}
