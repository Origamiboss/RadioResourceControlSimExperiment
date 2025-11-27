// src/PDcp.cpp
#include "PDcp.hpp"
#include "PcapLogger.hpp" // optional logging
#include "Utils.hpp"
#include <sstream>
#include "PacketBuffer.hpp"

using namespace pdcp;

PDcp::PDcp(const std::string &name)
: txSn_(0), rxExpectSn_(0), name_(name), pcapLogger_(nullptr)
{}

PDcp::~PDcp() = default;

void PDcp::setPcapLogger(PcapLogger* logger){
    pcapLogger_ = logger;
}

void PDcp::setDeliverCallback(DeliverFn cb){
    deliverCb_ = cb;
}

uint8_t PDcp::nextTxSn(){
    std::lock_guard<std::mutex> lk(mtx_);
    uint8_t s = txSn_;
    txSn_ = uint8_t((txSn_ + 1) & 0xFF);
    return s;
}

uint8_t PDcp::nextRxExpectSn(){
    std::lock_guard<std::mutex> lk(mtx_);
    return rxExpectSn_;
}

PDcp::Bytes PDcp::makeControlPacket(uint8_t sn, const Bytes& payload){
    PdcpHeader hdr;
    hdr.sn = sn;
    hdr.type = 1;
    Bytes out;
    out.reserve(2 + payload.size());
    out.push_back(hdr.sn);
    out.push_back(hdr.type);
    Bytes copy = payload;
    cipher(copy);
    out.insert(out.end(), copy.begin(), copy.end());

    // optional PCAP logging
    if(pcapLogger_){
        pcapLogger_->logRawPacket(out, name_);
    }

    return out;
}

PDcp::Bytes PDcp::makeDataPacket(uint8_t sn, const Bytes& payload) {
    PdcpHeader hdr;
    hdr.sn = sn;
    hdr.type = 0;
    Bytes out;
    out.reserve(2 + payload.size());
    out.push_back(hdr.sn);
    out.push_back(hdr.type);
    Bytes copy = payload;
    cipher(copy);
    out.insert(out.end(), copy.begin(), copy.end());

    // optional PCAP logging
    if (pcapLogger_) {
        pcapLogger_->logRawPacket(out, name_);
    }

    return out;
}

bool PDcp::parsePacket(const Bytes& raw, PdcpHeader& hdr, Bytes& payloadOut){
    if(raw.size() < 2) return false;
    hdr.sn = raw[0];
    hdr.type = raw[1];
    payloadOut.assign(raw.begin() + 2, raw.end());
    decipher(payloadOut);
    return true;
}

PDcp::Bytes PDcp::encapsulate(const Bytes& payload){
    uint8_t sn = nextTxSn();
    return makeControlPacket(sn, payload);
}

std::optional<Bytes> PDcp::onReceive(const Bytes& raw){
    PdcpHeader hdr;
    Bytes payload;
    if(!parsePacket(raw, hdr, payload)){
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lk(mtx_);
    // if this is the expected SN, deliver and advance
    if (hdr.sn == rxExpectSn_ || true) {
        Bytes out = payload;   // copy for return value

        rxExpectSn_ = uint8_t((rxExpectSn_ + 1) & 0xFF);

        // flush reorder buffer (but only return first one)
        while (true) {
            auto it = reorderBuffer_.find(rxExpectSn_);
            if (it == reorderBuffer_.end()) break;

            // (optional: deliver but not return more than one)
            rxExpectSn_ = uint8_t((rxExpectSn_ + 1) & 0xFF);
            reorderBuffer_.erase(it);
        }
        //I want it to return the type and out
        return out;
    }
    reorderBuffer_.emplace(hdr.sn, payload);
    return std::nullopt;
    // optional pcap log for received packet already handled in parsePacket if needed
}

std::string PDcp::status() const {
    std::ostringstream ss;
    std::lock_guard<std::mutex> lk(mtx_);
    ss << name_ << " txSn=" << int(txSn_) << " rxExpect=" << int(rxExpectSn_) << " buf=" << reorderBuffer_.size();
    return ss.str();
}

void PDcp::cipher(Bytes& data){
    // demo XOR with 0xAA. Replace with real cipher if needed.
    for(auto &b : data) b ^= 0xAA;
}

void PDcp::decipher(Bytes& data){
    cipher(data); // symmetric XOR
}
