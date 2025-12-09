#pragma once
#include <atomic>
#include <memory>
#include <vector>
#include "PacketBuffer.hpp"
#include "PDcp.hpp"

class Attacker {
public:
    enum AttackMode {
        DOS = 1,
        FUZZ = 2,
        HYBRID = 3
    };

    Attacker(PacketBuffer* targetBuffer, int sizeOfPackets,
             std::atomic<bool>* runFlag, AttackMode mode = DOS);

    void attackTargetBuffer();     // Unified attack
    void DoSAttackStep();          // One DoS send
    void FuzzAttackStep();         // One fuzzing send

private:
    PacketBuffer* targetBuffer_;
    int sizeOfPackets_;
    std::atomic<bool>* running;
    AttackMode mode_;

    std::optional<pdcp::PDcp::Bytes> lastFuzzedBytes;

    std::unique_ptr<pdcp::PDcp> pdcp_;

    pdcp::PDcp::Bytes createFuzzingPackets(int numOfBytes);
    void log(const std::string& msg);
};
