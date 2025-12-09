#pragma once

#include <queue>
#include <vector>
#include <cstdint>
#include <optional>
#include <mutex>
#include <condition_variable>
#include <atomic>

using Bytes = std::vector<uint8_t>;

class PacketBuffer {
private:
    std::queue<Bytes> buffer;
    std::mutex mutex_;
    std::condition_variable cond_var_;
    size_t maxSize = 1000;                // Limit queue capacity
    std::atomic<size_t> droppedPackets{0};

public:
    // Put a packet into the buffer and notify waiting threads
    void sendPacket(const std::vector<uint8_t>& raw) {
        std::unique_lock<std::mutex> lock(mutex_);

        // Check if queue is full
        if (buffer.size() >= maxSize) {
            droppedPackets++;   // count as dropped
            std::cout << "[PacketBuffer] Packet dropped! Total dropped: " 
                      << droppedPackets.load() << "\n";
            return;             // do NOT push packet
        }

        buffer.push(raw);  // Insert packet normally
        lock.unlock();     // Unlock before notifying
        cond_var_.notify_all();
    }

    // Try to get a packet; returns nullopt if empty
    std::optional<Bytes> getPacket() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (buffer.empty()) {
            return std::nullopt;
        }

        Bytes pkt = buffer.front();
        buffer.pop();
        return pkt;
    }

    // Wait for a new packet to arrive
    std::optional<Bytes> waitForPacket() {
        std::unique_lock<std::mutex> lock(mutex_);

        cond_var_.wait(lock, [this] { return !buffer.empty(); });

        Bytes pkt = buffer.front();
        buffer.pop();
        return pkt;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mutex_);
        return buffer.empty();
    }

    // Allow access to dropped packet count
    size_t getDroppedCount() const {
        return droppedPackets.load();
    }

    // Allow changing buffer size (optional)
    void setMaxSize(size_t size) {
        maxSize = size;
    }
};
