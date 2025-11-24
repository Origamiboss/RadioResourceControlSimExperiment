#pragma once

#include <queue>
#include <vector>
#include <cstdint>
#include <optional>

using Bytes = std::vector<uint8_t>;

class PacketBuffer {
private:
    std::queue<Bytes> buffer;

public:
    // Put a packet into the buffer
    void sendPacket(const std::vector<uint8_t>& raw) {
        buffer.push(raw);
    }

    // Try to get a packet; returns nullopt if empty
    std::optional<Bytes> getPacket() {
        if (buffer.empty()) {
            return std::nullopt;  // Return an empty optional if the buffer is empty
        }
        
        Bytes pkt = buffer.front();  // Copy the packet from the front of the queue
        buffer.pop();  // Remove the packet from the queue
        return pkt;  // Return the copied packet as an optional
    }

    bool empty() const {
        return buffer.empty();
    }
};

