#ifndef NETWORKRRC_HPP
#define NETWORKRRC_HPP

#include <fstream>  // for std::ofstream
#include <vector>   // for std::vector
#include "PcapLogger.hpp"  // Include this for full definition of PcapLogger
#include "PDcp.hpp"   // Assuming PDcp is required for PDcp class
#include "RrcState.hpp"

class NetworkRrc {
public:
    NetworkRrc();
    ~NetworkRrc();

    void receiveRrcConnectionRequest();
    void sendRrcConnectionSetup();
    void receiveRrcConnectionComplete();
    void sendRrcRelease();
    void receiveFromUe(const std::vector<uint8_t>& rawPacket);

    RrcState getState() const;

private:
    void log(const std::string& message);  // log function

    RrcState state = RrcState::RRC_IDLE;  // State initialization
    std::ofstream logFile;  // Log file
    PcapLogger pcapLogger;  // Full definition of PcapLogger should be available
    std::unique_ptr<pdcp::PDcp> pdcp_;  // PDCP instance
};

#endif // NETWORKRRC_HPP

