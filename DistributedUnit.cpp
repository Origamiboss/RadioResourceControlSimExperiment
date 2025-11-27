#include "DistributedUnit.hpp"
#include <iostream>
#include "PDcp.hpp"
#include "Utils.hpp"

DistributedUnit::DistributedUnit(PacketBuffer* f1cBuffer, PacketBuffer* f2cBuffer, PacketBuffer* f1uBuffer, PacketBuffer* f2uBuffer) {
    this->f1cBuffer = f1cBuffer; // UE -> DU (uplink from UE)
    this->f2cBuffer = f2cBuffer; // DU -> CU (uplink to CU)
    this->f1uBuffer = f1uBuffer; // CU -> DU (downlink from CU)
    this->f2uBuffer = f2uBuffer; // DU -> UE (downlink to UE)
    
    pdcp_ = std::make_unique<pdcp::PDcp>("DU-PDCP");
    // Use DU's own pcapLogger
    pdcp_->setPcapLogger(&pcapLogger);
}

void DistributedUnit::checkForPackets() {
    checkForUePackets();
    checkForCuPackets();
}

void DistributedUnit::checkForCuPackets() {
    std::cout << "[DU] Waiting for CU packets...\n";
    auto optPacket = f1uBuffer->waitForPacket(); // this BLOCKS until packet arrives
    
    if (!optPacket) return;
    std::cout << "[DU] CU packet received\n";
    auto payload = pdcp_->onReceive(*optPacket);
    if (!payload) return;


    auto pdcpPacket = pdcp_->encapsulate(*payload);

    std::cout << "[DU] Forwarding CU packet to UE\n";
    f2uBuffer->sendPacket(pdcpPacket);   // send to CU
        
    
}
void DistributedUnit::checkForUePackets() {
    std::cout << "[DU] Waiting for UE packets...\n";
    auto optPacket = f1cBuffer->waitForPacket(); // this BLOCKS until packet arrives
    
    if (!optPacket) return;
    std::cout << "[DU] UE packet received\n";
    auto payload = pdcp_->onReceive(*optPacket);
    if (!payload) return;


    auto pdcpPacket = pdcp_->encapsulate(*payload);

    std::cout << "[DU] Forwarding UE packet to CU\n";
    f2cBuffer->sendPacket(pdcpPacket);   // back to UE
    
}