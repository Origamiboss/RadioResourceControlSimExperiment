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
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 255);

    pdcp::PDcp::Bytes payload(numOfBytes);
    for (int i = 0; i < numOfBytes; i++)
        payload[i] = static_cast<uint8_t>(dist(gen));

    return payload;
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
