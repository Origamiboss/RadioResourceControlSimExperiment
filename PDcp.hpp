// include/PDcp.hpp
#pragma once

#include <vector>
#include <cstdint>
#include <map>
#include <mutex>
#include <functional>
#include <memory>
#include <string>
#include <optional>

class PcapLogger; // forward

namespace pdcp {

struct PdcpHeader {
    uint8_t sn;       // simple 8-bit sequence number (wraps)
    uint8_t type;     // 0 = data, 1 = control (reserved)
};

class PDcp {
public:
    using Bytes = std::vector<uint8_t>;
    using DeliverFn = std::function<void(const Bytes& payload)>;

    PDcp(const std::string &name = "PDCP");
    ~PDcp();

    // configure logger (optional)
    void setPcapLogger(PcapLogger* logger);

    // set callback that receives decapsulated payload (upper layer)
    void setDeliverCallback(DeliverFn cb);

    // UE/Network calls this to send payload downwards (PDCP -> RLC/PHY)
    Bytes encapsulate(const Bytes& payload);

    // call this when PDCP receives a raw packet from lower layers
    std::optional<Bytes> onReceive(const Bytes& raw);

    std::optional<Bytes> getPacketForForward(const Bytes& raw);

    // debugging
    std::string status() const;

private:
    uint8_t nextTxSn();
    uint8_t nextRxExpectSn();

    Bytes makeControlPacket(uint8_t sn, const Bytes& payload);
    Bytes makeDataPacket(uint8_t sn, const Bytes& payload);
    bool parsePacket(const Bytes& raw, PdcpHeader& hdr, Bytes& payloadOut);

    // very small "reordering" buffer keyed by SN (for demo)
    std::map<uint8_t, Bytes> reorderBuffer_;
    uint8_t txSn_;
    uint8_t rxExpectSn_;
    std::string name_;
    PcapLogger* pcapLogger_;
    DeliverFn deliverCb_;
    mutable std::mutex mtx_;

    // basic placeholder for "cipher" - no real crypto (demo only)
    void cipher(Bytes& data);
    void decipher(Bytes& data);
};

} // namespace pdcp
