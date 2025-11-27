#include "DistributedUnit.hpp"
#include <iostream>
#include "PDcp.hpp"
#include "Utils.hpp"

DistributedUnit::DistributedUnit(PacketBuffer* f1cBuffer, PacketBuffer* d1cBuffer, PacketBuffer* f1uBuffer) {
    this->f1cBuffer = f1cBuffer;
    this->f1uBuffer = f1uBuffer;
    this->d1cBuffer = d1cBuffer;
    
    pdcp_ = std::make_unique<pdcp::PDcp>("DU-PDCP");
    // Use DU's own pcapLogger
    pdcp_->setPcapLogger(&pcapLogger);
}

void DistributedUnit::checkForPackets() {
    checkForUePackets();
    checkForCuPackets();
}

void DistributedUnit::checkForUePackets() {
    while (ueBuffer->hasPacket()) {
        PDcp::Bytes p = ueBuffer->getPacket();
        std::cout << "[DU] Forwarding UE packet to CU\n";
        f1cBuffer->sendPacket(p);   // send to CU
    }
}
void DistributedUnit::checkForCuPackets() {
    while (f1cBuffer->hasPacket()) {
        PDcp::Bytes p = f1cBuffer->getPacket();
        std::cout << "[DU] Forwarding CU packet to UE\n";
        ueBuffer->sendPacket(p);   // back to UE
    }
}