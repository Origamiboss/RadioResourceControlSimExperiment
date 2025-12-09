#include <random>
#include <thread>      // <- for sleep_for
#include <chrono>
#include <iostream>
#include "Attacker.hpp"

Attacker::Attacker(PacketBuffer* targetBuffer, int sizeOfPackets,
                   std::atomic<bool>* runFlag, AttackMode mode)
    : targetBuffer_(targetBuffer),
      sizeOfPackets_(sizeOfPackets),
      running(runFlag),
      mode_(mode)
{
    pdcp_ = std::make_unique<pdcp::PDcp>("Network-PDCP");

    pdcp_->setDeliverCallback(
        [this](const pdcp::PDcp::Bytes& payload) {
            this->log("[PDCP] Delivered payload size " +
                      std::to_string(payload.size()));
        }
    );
}

pdcp::PDcp::Bytes Attacker::createFuzzingPackets(int numOfBytes) {

    // If this is the first fuzz packet, initialize all bytes to 0x00
    if (!lastFuzzedBytes || lastFuzzedBytes->size() != (size_t)numOfBytes) {
        lastFuzzedBytes = pdcp::PDcp::Bytes(numOfBytes, 0x00);
        return *lastFuzzedBytes;
    }

    // Increment like a big-endian integer
    for (int i = numOfBytes - 1; i >= 0; --i) {
        (*lastFuzzedBytes)[i]++;

        // If it didn't overflow, we stop
        if ((*lastFuzzedBytes)[i] != 0x00)
            break;

        // Else, it overflowed (0xFF -> 0x00), continue incrementing the next byte
    }

    return *lastFuzzedBytes;
}

void Attacker::DoSAttackStep() {
    pdcp::PDcp::Bytes payload = {0xFF, 0xFF};   // very small, very fast to spam
    auto packet = pdcp_->encapsulate(payload);
    targetBuffer_->sendPacket(packet);
}

void Attacker::FuzzAttackStep() {
    auto randomPayload = createFuzzingPackets(sizeOfPackets_);
    auto packet = pdcp_->encapsulate(randomPayload);
    targetBuffer_->sendPacket(packet);
}

void Attacker::attackTargetBuffer() {

    while (running->load()) {

        switch (mode_) {
            case AttackMode::DOS:
                DoSAttackStep();
                break;

            case AttackMode::FUZZ:
                FuzzAttackStep();
                break;

            case AttackMode::HYBRID:
                // 75% DoS, 25% fuzzing
                if (rand() % 4 == 0)
                    FuzzAttackStep();
                else
                    DoSAttackStep();
                break;
        }

        // Prevent CPU burn
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
}

void Attacker::log(const std::string& msg) {
    std::cout << msg << std::endl;
}
