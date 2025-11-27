#include "DistributedUnit.hpp"
#include <iostream>
#include "PDcp.hpp"
#include "Utils.hpp"

DistributedUnit::DistributedUnit(PacketBuffer* f1cBuffer, PacketBuffer* f2cBuffer, PacketBuffer* f1uBuffer, PacketBuffer* f2uBuffer) {
    this->f1cBuffer = f1cBuffer;
    this->f1uBuffer = f1uBuffer;
    
    pdcp_ = std::make_unique<pdcp::PDcp>("DU-PDCP");
    // Use DU's own pcapLogger
    pdcp_->setPcapLogger(&pcapLogger);
}

void DistributedUnit::checkForPackets() {
    checkForUePackets();
    //checkForCuPackets();
}

void DistributedUnit::checkForUePackets() {
    if (!f1uBuffer->empty()) {
        std::cout << "[DU] Waiting for UE packets...\n";
        auto optPacket = f1uBuffer->getPacket(); // this BLOCKS until packet arrives
        std::cout << "[DU] UE packet received\n";
        if (!optPacket) return;

        auto payload = pdcp_->onReceive(*optPacket);
        if (!payload) return;


        auto pdcpPacket = pdcp_->encapsulate(*payload);

        std::cout << "[DU] Forwarding UE packet to CU\n";
        f2uBuffer->sendPacket(pdcpPacket);   // send to CU
    }
}
void DistributedUnit::checkForCuPackets() {
    if (!f1cBuffer->empty()) {
        std::cout << "[DU] Waiting for CU packets...\n";
        auto optPacket = f1cBuffer->getPacket(); // this BLOCKS until packet arrives
        std::cout << "[DU] CU packet received\n";
        if (!optPacket) return;

        auto payload = pdcp_->onReceive(*optPacket);
        if (!payload) return;


        auto pdcpPacket = pdcp_->encapsulate(*payload);

        std::cout << "[DU] Forwarding CU packet to UE\n";
        f2cBuffer->sendPacket(pdcpPacket);   // back to UE
    }
}