#include "DistributedUnit.hpp"
#include <iostream>
#include "PDcp.hpp"
#include "Utils.hpp"
#include "LatencyModel.hpp"
#include <thread>

DistributedUnit::DistributedUnit(PacketBuffer* f1cBuffer, PacketBuffer* f2cBuffer, PacketBuffer* f1uBuffer, PacketBuffer* f2uBuffer, int optionType) {
    this->f1cBuffer = f1cBuffer; // UE -> DU (uplink from UE)
    this->f2cBuffer = f2cBuffer; // DU -> CU (uplink to CU)
    this->f1uBuffer = f1uBuffer; // CU -> DU (downlink from CU)
    this->f2uBuffer = f2uBuffer; // DU -> UE (downlink to UE)
    
    this->optionType = optionType;


    // Default fronthaul model (you can tune)
    latency.fiberOneWayUs = 50;     // 50 microseconds
    latency.packetizationUs = 30;   // ~0.03 ms
    latency.jitterUs = 10;          // +/- 10 µs
    latency.fronthaulMbps = 10000;  // 10 Gbps

    pdcp_ = std::make_unique<pdcp::PDcp>("DU-PDCP");
    // Use DU's own pcapLogger
    pdcp_->setPcapLogger(&pcapLogger);
}

void DistributedUnit::changeOptionType(int newType){
    optionType = newType;
}

void DistributedUnit::checkForPackets() {
    checkForUePackets();
    checkForCuPackets();
}

void DistributedUnit::checkForCuPackets() {
    if(f1uBuffer->empty()) return;
    std::cout << "[DU] Waiting for CU packets...\n";
    auto optPacket = f1uBuffer->getPacket(); // this BLOCKS until packet arrives
    
    if (!optPacket) return;
    std::cout << "[DU] CU packet received\n";

    auto processed = processDownlink(*optPacket);
    if (processed.empty()) return;
    std::cout << "[DU] Forwarding CU packet to UE\n";
    f2uBuffer->sendPacket(processed);

    
    
}
void DistributedUnit::checkForUePackets() {
    if(f1cBuffer->empty()) return;
    std::cout << "[DU] Waiting for UE packets...\n";
    auto optPacket = f1uBuffer->getPacket(); // this BLOCKS until packet arrives
    
    if (!optPacket) return;
    std::cout << "[DU] UE packet received\n";

    auto processed = proccessUpLink(*optPacket);
    if (processed.empty()) return;
    std::cout << "[DU] Forwarding UE packet to CU\n";
    f2uBcffer->sendPacket(processed);

    
}

pdcp::PDcp::Bytes DistributedUnit::proccessUpLink(pdcp::PDcp::Bytes& raw){
    int fronthaulDelay = computeFronthaulDelayUs(raw.size());
    int uuDelay        = computeUuDelayUs();
    int processDelay   = computeProcessingDelayUs();

    int totalUs = fronthaulDelay + uuDelay + processDelay;

    std::this_thread::sleep_for(std::chrono::microseconds(totalUs));

    // Same payload decoding logic
    if (auto payload = pdcp_->onReceive(raw)) 
        return pdcp_->encapsulate(*payload);

    return {};
}
pdcp::PDcp::Bytes DistributedUnit::processDownlink(pdcp::PDcp::Bytes& raw)
{
    int fronthaulDelay = computeFronthaulDelayUs(raw.size());
    int uuDelay        = computeUuDelayUs();
    int processDelay   = computeProcessingDelayUs();

    int totalUs = fronthaulDelay + uuDelay + processDelay;

    std::this_thread::sleep_for(std::chrono::microseconds(totalUs));

    // Same payload decoding logic
    if (auto payload = pdcp_->onReceive(raw)) 
        return pdcp_->encapsulate(*payload);

    return {};
}

int DistributedUnit::computeFronthaulDelayUs(size_t packetSizeBytes)
{
    double bits = packetSizeBytes * 8.0;

    double serialization = (bits / (latency.fronthaulMbps * 1'000'000.0)) * 1'000'000; 
    // in microseconds

    int jitter = (rand() % (latency.jitterUs * 2)) - latency.jitterUs;

    return latency.fiberOneWayUs + latency.packetizationUs +
           (int)serialization + jitter;
}

int DistributedUnit::computeUuDelayUs()
{
    switch (optionType)
    {
        case 2:   return 300;  // High latency (PDCP high-level)
        case 6:   return 150;  // Medium (RLC locally processed)
        case 7:   return 60;   // Low latency (PHY split)
        default:  return 200;
    }
}

int DistributedUnit::computeProcessingDelayUs()
{
    switch (optionType)
    {
        case 2: return 200;  // DU does very little
        case 6: return 500;  // RLC processing
        case 7: return 800;  // PHY processing
        default: return 300;
    }
}