#pragma once
#include "PacketBuffer.hpp"
#include "PDcp.hpp"

class Attacker {
public:
	Attacker();
	//Exploit Utilities
	pdcp::PDcp::Bytes createFuzzingPackets(int numOfBytes);
	void attackTargetBuffer();
private:
	PacketBuffer* targetBuffer;
	int sizeOfPackets;
	std::unique_ptr<pdcp::PDcp> pdcp_;  // PDCP instance
};

