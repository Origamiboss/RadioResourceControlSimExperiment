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
    std::optional<std::vector<uint8_t>& getPacket() {
        if (buffer.empty()) {
            return std::nullopt;
        }
        Bytes pkt = buffer.front();
        buffer.pop();
        return pkt;
    }

    bool empty() const {
        return buffer.empty();
    }
};
