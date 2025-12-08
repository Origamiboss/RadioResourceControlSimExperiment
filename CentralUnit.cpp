#include "CentralUnit.hpp"
#include <iostream>
#include "PDcp.hpp"
#include "Utils.hpp"

CentralUnit::CentralUnit(PacketBuffer* f1cBuffer, PacketBuffer* f1uBuffer, int optionType) {
    this->f1cBuffer = f1cBuffer;
    this->f1uBuffer = f1uBuffer;
    
    this->optionType = optionType;
    pdcp_ = std::make_unique<pdcp::PDcp>("DU-PDCP");
    // Use DU's own pcapLogger
    pdcp_->setPcapLogger(&pcapLogger);

    // Same latency model as DU
    latency.fiberOneWayUs = 50;     // 50 us
    latency.packetizationUs = 30;   // 0.03 ms
    latency.jitterUs = 10;          // +/- 10 us
    latency.fronthaulMbps = 10000;  // 10 Gbps link
}


void CentralUnit::checkForPackets() {
    if(!f1cBuffer->empty()){
        std::cout << "[CU] Waiting for packets...\n";
        auto optPacket = f1cBuffer->getPacket();
        if (!optPacket) return;

        std::cout << "[CU] Packet received\n";

        // Apply realistic fronthaul + processing latency
        applyLatency(optPacket->size());

        auto payload = pdcp_->onReceive(*optPacket);
        if (!payload) return;

        auto msg = *payload;

        if (msg == pdcp::PDcp::Bytes{0x40, 0x12}) {
            receiveRrcConnectionRequest();
        }
        else if (msg == pdcp::PDcp::Bytes{0x43, 0x34}) {
            receiveRrcConnectionComplete();
        }else if (state == RrcState::RRC_CONNECTED) {
            sendDummyData();
        }
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
                << "] [Network -> UE] sent Dummy Data (" << dataSize << " bytes)\n";

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
        case 2: return 8000;   // CU does PDCP+RRC heavy: 8 ms
        case 6: return 2000;   // CU does MAC scheduling: 2 ms
        case 7: return 200;    // CU mostly control: 0.2 ms
        default: return 500;
    }
}

int CentralUnit::computeUuDelayUs()
{
    switch (optionType) {
        case 2: return 300;
        case 6: return 150;
        case 7: return 60;
        default: return 200;
    }
}

void CentralUnit::applyLatency(int sizeBytes)
{
    int fronthaul = computeFronthaulDelayUs(sizeBytes);
    int proc      = computeProcessingDelayUs();
    int uu        = computeUuDelayUs();

    int total = fronthaul + proc + uu;

    std::this_thread::sleep_for(std::chrono::microseconds(total));
}


