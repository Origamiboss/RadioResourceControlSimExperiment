#include "DistributedUnit.hpp"
#include <iostream>
#include "PDcp.hpp"
#include "Utils.hpp"

DistributedUnit::DistributedUnit(PacketBuffer* f1cBuffer, PacketBuffer* f1uBuffer) {
    this->f1cBuffer = f1cBuffer;
    this->f1uBuffer = f1uBuffer;
    
    pdcp_ = std::make_unique<pdcp::PDcp>("DU-PDCP");
    // Use DU's own pcapLogger
    pdcp_->setPcapLogger(&pcapLogger);
}

void DistributedUnit::checkForPackets() {
    checkForUePackets();
    checkForCuPackets();
}

void DistributedUnit::checkForUePackets() {
    while (f1uBuffer->empty()) {
        std::cout << "[DU] Waiting for UE packets...\n";
        auto optPacket = f1uBuffer->getPacket(); // this BLOCKS until packet arrives
        if (!optPacket) continue;

        auto payload = pdcp_->onReceive(*optPacket);
        if (!payload) continue;

        auto &data = *payload;


        auto pdcpPacket = pdcp_->encapsulate(data);

        std::cout << "[DU] Forwarding UE packet to CU\n";
        f1cBuffer->sendPacket(pdcpPacket);   // send to CU
    }
}
void DistributedUnit::checkForCuPackets() {
    while (f1cBuffer->empty()) {
        std::cout << "[DU] Waiting for CU packets...\n";
        auto optPacket = f1cBuffer->getPacket(); // this BLOCKS until packet arrives
        if (!optPacket) continue;

        auto payload = pdcp_->onReceive(*optPacket);
        if (!payload) continue;

        auto &data = *payload;


        auto pdcpPacket = pdcp_->encapsulate(data);

        std::cout << "[DU] Forwarding CU packet to UE\n";
        f1uBuffer->sendPacket(pdcpPacket);   // back to UE
    }
}