// CentralUnit.cpp
#include "CentralUnit.hpp"
#include <iostream>
#include "PDcp.hpp"
#include "Utils.hpp"
#include "LatencyModel.hpp"
#include <thread>
#include <algorithm>
#include <chrono>

CentralUnit::CentralUnit(PacketBuffer* f1cBuffer, PacketBuffer* f1uBuffer, int optionType) {
    this->f1cBuffer = f1cBuffer; // DU -> CU (uplink to CU)
    this->f1uBuffer = f1uBuffer; // CU -> DU (downlink from CU)

    this->optionType = optionType;
    pdcp_ = std::make_unique<pdcp::PDcp>("CU-PDCP");
    pdcp_->setPcapLogger(&pcapLogger);

    // Same latency model as DU by default
    latency.fiberOneWayUs = 50;     // 50 us
    latency.packetizationUs = 30;   // 0.03 ms
    latency.jitterUs = 10;          // +/- 10 us
    latency.fronthaulMbps = 10000;  // 10 Gbps link

    state = RrcState::RRC_IDLE;
}

void CentralUnit::changeOptionType(int newType){
    optionType = newType;
}

// Main packet poller: handle any incoming packet from DU
void CentralUnit::checkForPackets() {
    // If there's no packet, return quickly
    if (f1cBuffer->empty()) return;

    std::cout << "[CU] Waiting for DU->CU packets...\n";
    auto optPacket = f1cBuffer->getPacket(); // blocks until packet or returns nullptr
    if (!optPacket) return;

    std::cout << "[CU] Packet received from DU\n";

    // Process the uplink packet similarly to how DU does its processing
    auto processed = processUpLink(*optPacket);
    if (processed.empty()) {
        std::cout << "[CU] Processed packet empty (dropped or no payload)\n";
        return;
    }

    // Interpret PDCP payload for control messages (RRC) or hand to application
    if (auto payload = pdcp_->onReceive(processed)) {
        pdcp::PDcp::Bytes msg = *payload;

        // Simple RRC handshake ops (matching your existing logic)
        if (msg == pdcp::PDcp::Bytes{0x40, 0x12}) {
            receiveRrcConnectionRequest();
            return;
        } else if (msg == pdcp::PDcp::Bytes{0x43, 0x34}) {
            receiveRrcConnectionComplete();
            return;
        } else {
            // If we're connected, maybe send downlink data or forward elsewhere
            if (state == RrcState::RRC_CONNECTED) {
                // send an application-level response down to DU/UE
                sendDummyData();
                return;
            }
        }
    }

    // If the packet contains a payload that should be forwarded down, encapsulate and send
    auto outPacket = pdcp_->encapsulate(processed);
    if (!outPacket.empty()) {
        applyLatency(outPacket.size()); // apply end-to-end-ish latency for the downlink send
        std::cout << "[CU] Forwarding processed packet to DU\n";
        f1uBuffer->sendPacket(outPacket);
    }
}

void CentralUnit::sendRrcSetup() {
    pdcp::PDcp::Bytes payload = {0x50, 0xAA};  // RRC Setup
    auto pdcpPacket = pdcp_->encapsulate(payload);

    applyLatency(pdcpPacket.size());

    std::cout << "[CU] Sending RRC Setup\n";
    f1uBuffer->sendPacket(pdcpPacket);

    state = RrcState::RRC_SETUP_SENT;
}

void CentralUnit::sendRrcRelease() {
    std::cout << "[CU] Attempting to send RRC Release\n";
    if (state == RrcState::RRC_CONNECTED) {

        pdcp::PDcp::Bytes payload = {0x5F, 0x21};
        auto pdcpPacket = pdcp_->encapsulate(payload);

        applyLatency(pdcpPacket.size());

        std::cout << "Sent RRCRelease From CU\n";

        f1uBuffer->sendPacket(pdcpPacket);

        state = RrcState::RRC_IDLE;
    }
}

void CentralUnit::sendDummyData() {
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
                << "] [CU -> DU/UE] sent Dummy Data (" << dataSize << " bytes)\n";

        applyLatency(pdcpPacket.size());
        f1uBuffer->sendPacket(pdcpPacket);
    }
}

void CentralUnit::receiveRrcConnectionComplete() {
    std::cout << "CentralUnit received RRCConnectionComplete\n";
    state = RrcState::RRC_CONNECTED;
    sendRrcRelease();
}

void CentralUnit::receiveRrcConnectionRequest() {
    std::cout << "[CU] Received RRC Request\n";
    sendRrcSetup();
    state = RrcState::RRC_SETUP_SENT;
}

// --- Processing helpers that mimic DistributedUnit behavior ---

pdcp::PDcp::Bytes CentralUnit::processUpLink(pdcp::PDcp::Bytes& raw) {
    int fronthaulDelay = computeFronthaulDelayUs(raw.size());
    int uuDelay        = computeUuDelayUs();
    int processDelay   = computeProcessingDelayUs();

    int totalUs = fronthaulDelay + uuDelay + processDelay;

    // Simulate time spent processing at CU similar to DU
    std::this_thread::sleep_for(std::chrono::microseconds(totalUs));

    // Same payload decoding logic as DU
    if (auto payload = pdcp_->onReceive(raw))
        return pdcp_->encapsulate(*payload);

    return {};
}

pdcp::PDcp::Bytes CentralUnit::processDownlink(pdcp::PDcp::Bytes& raw) {
    int fronthaulDelay = computeFronthaulDelayUs(raw.size());
    int uuDelay        = computeUuDelayUs();
    int processDelay   = computeProcessingDelayUs();

    int totalUs = fronthaulDelay + uuDelay + processDelay;

    std::this_thread::sleep_for(std::chrono::microseconds(totalUs));

    if (auto payload = pdcp_->onReceive(raw))
        return pdcp_->encapsulate(*payload);

    return {};
}

// Fronthaul serialization + jitter model (same as DU)
int CentralUnit::computeFronthaulDelayUs(size_t sizeBytes)
{
    double bits = sizeBytes * 8.0;

    double serialization = (bits / (latency.fronthaulMbps * 1'000'000.0)) 
                           * 1'000'000.0; // microseconds

    int jitter = (rand() % (latency.jitterUs * 2)) - latency.jitterUs;

    return latency.fiberOneWayUs + latency.packetizationUs +
           (int)serialization + jitter;
}

int CentralUnit::computeProcessingDelayUs()
{
    switch (optionType) {
        case 2: return 1000;   // PDCP (1 ms) - you can tune this
        case 6: return 2000;   // Moderate (2 ms)
        case 7: return 3500;   // Option 7.1 high-PHY heavy (3.5 ms realistic)
        default: return 500;
    }
}

int CentralUnit::computeUuDelayUs()
{
    switch (optionType) {
        case 2: return 100;
        case 6: return 300;
        case 7: return 500;    // lower PHY in DU for 7.1 => CU Uu smaller, but fronthaul involved
        default: return 200;
    }
}

// Apply full latency (used when CU initiates a downlink send)
void CentralUnit::applyLatency(int sizeBytes)
{
    int fronthaul = computeFronthaulDelayUs(sizeBytes);
    int proc      = computeProcessingDelayUs();
    int uu        = computeUuDelayUs();

    int total = fronthaul + proc + uu;

    std::this_thread::sleep_for(std::chrono::microseconds(total));
}
