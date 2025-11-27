#include "CentralUnit.hpp"
#include <iostream>
#include "PDcp.hpp"
#include "Utils.hpp"

CentralUnit::CentralUnit(PacketBuffer* f1cBuffer, PacketBuffer* f1uBuffer) {
    this->f1cBuffer = f1cBuffer;
    this->f1uBuffer = f1uBuffer;
    
    pdcp_ = std::make_unique<pdcp::PDcp>("DU-PDCP");
    // Use DU's own pcapLogger
    pdcp_->setPcapLogger(&pcapLogger);
}


void CentralUnit::checkForPackets() {
    std::cout << "[CU] Waiting for packets...\n";
    auto optPacket = f1cBuffer->waitForPacket(); // this BLOCKS until packet arrives
    std::cout << "[CU] Packet received\n";
    if (!optPacket) continue;

    auto payload = pdcp_->onReceive(*optPacket);
    if (!payload) continue;

    auto msg = *payload;

    if (msg == pdcp::PDcp::Bytes{0x40, 0x12}) {
        receiveRrcConnectionRequest();
    }
    else if (msg == pdcp::PDcp::Bytes{0x43, 0x34}) {
        receiveRrcConnectionComplete();
    }
}
void CentralUnit::sendRrcSetup() {
    pdcp::PDcp::Bytes payload = {0x50, 0xAA};  // RRC Setup
    auto pdcpPacket = pdcp_->encapsulate(payload);

    std::cout << "[CU] Sending RRC Setup\n";
    f1uBuffer->sendPacket(pdcpPacket);

    state = RrcState::RRC_SETUP_SENT;
}

void CentralUnit::sendRrcRelease() {
    if (state == RrcState::RRC_CONNECTED) {

        pdcp::PDcp::Bytes payload = {0x5F, 0x21};
        auto pdcpPacket = pdcp_->encapsulate(payload);

        std::cout << "Sent RRCRelease From CU\n";

        f1uBuffer->sendPacket(pdcpPacket);

        state = RrcState::RRC_IDLE;
    }
}
void CentralUnit::sendRrcConnectionSetup() {

    pdcp::PDcp::Bytes payload = {0x50, 0xAA};
    auto pdcpPacket = pdcp_->encapsulate(payload);

    std::cout << "Sent RRCConnectionSetup From CU\n";

    f1uBuffer->sendPacket(pdcpPacket);

    
    
}

void CentralUnit::receiveRrcConnectionComplete() {
    
    std::cout << "CentralUnit received RRCConnectionComplete\n";
    state = RrcState::RRC_CONNECTED;
}
void CentralUnit::receiveRrcConnectionRequest() {
    std::cout << "[CU] Received RRC Request\n";
    sendRrcConnectionSetup();
    state = RrcState::RRC_SETUP_SENT;
}