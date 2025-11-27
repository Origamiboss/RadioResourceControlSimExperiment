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
        pdcp::PDcp::Bytes p = f1uBuffer->getPacket();
        auto decap = pdcp_->onReceive(p);
        if (!decap) continue;

        auto msg = *decap;


        auto pdcpPacket = pdcp_->encapsulate(msg);

        std::cout << "[DU] Forwarding UE packet to CU\n";
        f1cBuffer->sendPacket(pdcpPacket);   // send to CU
    }
}
void DistributedUnit::checkForCuPackets() {
    while (f1cBuffer->empty()) {
        pdcp::PDcp::Bytes p = f1cBuffer->getPacket();

        auto decap = pdcp_->onReceive(p);
        if (!decap) continue;

        auto msg = *decap;


        auto pdcpPacket = pdcp_->encapsulate(msg);

        std::cout << "[DU] Forwarding CU packet to UE\n";
        f1uBuffer->sendPacket(pdcpPacket);   // back to UE
    }
}