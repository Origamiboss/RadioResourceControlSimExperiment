#include <vector>
#include <cstdint>
#include <cstdlib>
#include <random>          // <-- REQUIRED for random_device + mt19937
#include <memory>          // <-- REQUIRED for std::make_unique
#include <string>          // <-- REQUIRED for std::to_string
#include <iostream>

#include "Attacker.hpp"    // <-- MUST include your class declaration
#include "PDcp.hpp"

Attacker::Attacker(PacketBuffer* targetBuffer, int sizeOfPackets)
    : targetBuffer_(targetBuffer), sizeOfPackets_(sizeOfPackets)
{
    pdcp_ = std::make_unique<pdcp::PDcp>("Network-PDCP");

    pdcp_->setDeliverCallback([this](const pdcp::PDcp::Bytes& payload) {
        this->log("[PDCP] Delivered payload of size " + std::to_string(payload.size()));
    });
}

//A brute force method for fuzzing packets
pdcp::PDcp::Bytes Attacker::createFuzzingPackets(int numOfBytes) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 255);

    pdcp::PDcp::Bytes payload(numOfBytes);

    for (int i = 0; i < numOfBytes; i++) {
        payload[i] = static_cast<uint8_t>(dist(gen));
    }

    return payload;
}

void Attacker::attackTargetBuffer() {
    while (true) {
        auto randomPayload = createFuzzingPackets(sizeOfPackets_);
        auto pdcpPacket = pdcp_->encapsulate(randomPayload);

        // FIX: use targetBuffer_, NOT attackTargetBuffer
        targetBuffer_->sendPacket(pdcpPacket);
    }
}
void Attacker::DoSAttack(){
    while (true) {
        auto payload = static_cast<uint8_t>{0x00, 0x00};
        auto pdcpPacket = pdcp_->encapsulate(payload);

        // FIX: use targetBuffer_, NOT attackTargetBuffer
        targetBuffer_->sendPacket(pdcpPacket);
    }
}
void Attacker::log(const std::string& msg) {
    std::cout << msg << std::endl;
}

