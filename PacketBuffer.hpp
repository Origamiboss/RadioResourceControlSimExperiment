#pragma once

#include <queue>
#include <vector>
#include <cstdint>
#include <optional>
#include <mutex>
#include <condition_variable>

using Bytes = std::vector<uint8_t>;

class PacketBuffer {
private:
    std::queue<Bytes> buffer;
    std::mutex mutex_;
    std::condition_variable cond_var_;

public:
    // Put a packet into the buffer and notify waiting threads
    void sendPacket(const std::vector<uint8_t>& raw) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            buffer.push(raw);  // Add packet to the buffer
        }
        cond_var_.notify_all();  // Notify all waiting threads
    }

    // Try to get a packet; returns nullopt if empty
    std::optional<Bytes> getPacket() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (buffer.empty()) {
            return std::nullopt;  // Return an empty optional if the buffer is empty
        }

        Bytes pkt = buffer.front();  // Copy the packet from the front of the queue
        buffer.pop();  // Remove the packet from the queue
        return pkt;  // Return the copied packet as an optional
    }

    // Wait for a new packet to arrive; returns the packet when available
    std::optional<Bytes> waitForPacket() {
        std::unique_lock<std::mutex> lock(mutex_);
        
        // Wait until there's a packet available
        cond_var_.wait(lock, [this] { return !buffer.empty(); });

        Bytes pkt = buffer.front();  // Copy the packet from the front of the queue
        buffer.pop();  // Remove the packet from the queue
        return pkt;  // Return the copied packet as an optional
    }

    // Non-const because it accesses the mutex
    bool empty() {
        std::lock_guard<std::mutex> lock(mutex_);
        return buffer.empty();
    }
};

