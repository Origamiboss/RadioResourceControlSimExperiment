#pragma once
#include <memory>
#include "PacketBuffer.hpp"
#include "PDcp.hpp"

class Attacker {
public:
    std::atomic<bool>* running;
    Attacker(PacketBuffer* targetBuffer, int sizeOfPackets);

    pdcp::PDcp::Bytes createFuzzingPackets(int numOfBytes);
    void attackTargetBuffer();
    void DoSAttack();

private:
    PacketBuffer* targetBuffer_;      // <-- correct name
    int sizeOfPackets_;               // <-- match .cpp usage
    std::unique_ptr<pdcp::PDcp> pdcp_;
    
    void log(const std::string& msg); // <-- if this exists
};

