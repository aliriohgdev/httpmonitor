#pragma once

#include <chrono>
#include <memory>
#include <functional>
#include <variant>
#include <string>

namespace capture {

    // Forward declaration
    struct PacketData;

    // Callback type
    using PacketCallback = std::function<void(const PacketData&)>;

    struct CaptureStats {
        uint64_t packets_processed{0};
        std::chrono::seconds duration{0};
    };

    using CaptureResult = std::variant<CaptureStats, std::string>;

    class PacketCapture {
    public:
        virtual ~PacketCapture() = default;

        PacketCapture(const PacketCapture&) = delete;
        PacketCapture& operator=(const PacketCapture&) = delete;
        PacketCapture(PacketCapture&&) = default;
        PacketCapture& operator=(PacketCapture&&) = default;

        virtual CaptureResult startCapture(std::chrono::seconds duration, PacketCallback callback) = 0;
        virtual void stopCapture() noexcept = 0;

    protected:
        PacketCapture() = default;
    };

    using CreateCaptureResult = std::variant<std::unique_ptr<PacketCapture>, std::string>;
    CreateCaptureResult createPacketCapture();

} // namespace capture