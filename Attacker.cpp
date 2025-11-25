#include <vector>
#include <cstdint>
#include <cstdlib>
#include "PDcp.hpp"


Attacker(PacketBuffer* targetBuffer, int sizeOfPackets) {
    this->targetBuffer = targetBuffer;
    this->sizeOfPackets = sizeOfPackets;

    pdcp_ = std::make_unique<pdcp::PDcp>("Network-PDCP");

    pdcp_->setDeliverCallback([this](const pdcp::PDcp::Bytes& payload) {
        this->log("[PDCP] Delivered payload of size " + std::to_string(payload.size()));
        });

}

//A brute force method for fuzzing packets
pdcp::PDcp::Bytes createFuzzingPackets(int numOfBytes) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 255);

    pdcp::PDcp::Bytes payload(numOfBytes);

    for (int i = 0; i < numOfBytes; i++) {
        payload[i] = static_cast<uint8_t>(dist(gen));
    }

    return payload;
}
void attackTargetBuffer() {
    while (true) {
        auto randomPayload = createFuzzingPackets(sizeOfPackets);
        auto pdcpPacket = pdcp_->encapsulate(randomPayload);
        attackTargetBuffer->sendPacket(pdcpPacket);
    }
}