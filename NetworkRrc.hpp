#ifndef NETWORKRRC_HPP
#define NETWORKRRC_HPP

#include <fstream>  // for std::ofstream
#include <vector>   // for std::vector
#include "PcapLogger.hpp"  // Include this for full definition of PcapLogger
#include "PDcp.hpp"   // Assuming PDcp is required for PDcp class
#include "RrcState.hpp"
#include "PacketBuffer.hpp"

class NetworkRrc {
public:
    NetworkRrc(PacketBuffer* myBuffer, PacketBuffer* theirBuffer);
    ~NetworkRrc();

    void receiveRrcConnectionRequest();
    void sendRrcConnectionSetup();
    void receiveRrcConnectionComplete();
    void sendRrcRelease();
    void checkForPackets();

    RrcState getState() const;

private:
    void log(const std::string& message);  // log function

    RrcState state = RrcState::RRC_IDLE;  // State initialization
    std::ofstream logFile;  // Log file
    PcapLogger pcapLogger;  // Full definition of PcapLogger should be available
    std::unique_ptr<pdcp::PDcp> pdcp_;  // PDCP instance

    //Packet Queue
    PacketBuffer myBuffer;
    PacketBuffer theirBuffer;
};

#endif // NETWORKRRC_HPP

