#include "capture/packet_capture.h"
#include "capture/packet_data.h"

// Desactivar temporalmente el warning de flexible array de PcapPlusPlus
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <pcapplusplus/PcapLiveDevice.h>
#include <pcapplusplus/PcapLiveDeviceList.h>
#include <pcapplusplus/Packet.h>
#include <pcapplusplus/IPv4Layer.h>
#include <pcapplusplus/TcpLayer.h>
#include <pcapplusplus/HttpLayer.h>
#pragma GCC diagnostic pop

// System includes
#include <arpa/inet.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#include <memory>
#include <cstring>

namespace capture {

//=============================================================================
// PacketData Implementation
//=============================================================================
class PcapPacketDataImpl : public PacketData {
public:
    PcapPacketDataImpl(pcpp::Packet* packet, const pcpp::RawPacket* rawPacket)
        : m_packet(packet)
        , m_rawPacket(rawPacket)
        , m_timestamp(std::chrono::steady_clock::now())
    {}

    std::chrono::steady_clock::time_point timestamp() const noexcept override {
        return m_timestamp;
    }

    uint16_t sourcePort() const noexcept override {
        auto tcp = m_packet->getLayerOfType<pcpp::TcpLayer>();
        if (tcp) {
            return ntohs(tcp->getTcpHeader()->portSrc);
        }
        return 0;
    }

    uint16_t destPort() const noexcept override {
        auto tcp = m_packet->getLayerOfType<pcpp::TcpLayer>();
        if (tcp) {
            return ntohs(tcp->getTcpHeader()->portDst);
        }
        return 0;
    }

    bool isHttp() const noexcept override {
        return m_packet->getLayerOfType<pcpp::HttpRequestLayer>() != nullptr;
    }

    bool isHttps() const noexcept override {
        uint16_t port = destPort();
        return port == 443 || port == 8443;
    }

    std::string_view sourceIP() const noexcept override {
        auto ip = m_packet->getLayerOfType<pcpp::IPv4Layer>();
        if (ip) {
            m_sourceIP = ip->getSrcIPAddress().toString();
            return m_sourceIP;
        }
        return {};
    }

    std::string_view destIP() const noexcept override {
        auto ip = m_packet->getLayerOfType<pcpp::IPv4Layer>();
        if (ip) {
            m_destIP = ip->getDstIPAddress().toString();
            return m_destIP;
        }
        return {};
    }

    std::string_view payload() const noexcept override {
        auto tcp = m_packet->getLayerOfType<pcpp::TcpLayer>();
        if (tcp && tcp->getLayerPayloadSize() > 0) {
            return std::string_view(
                reinterpret_cast<const char*>(tcp->getLayerPayload()),
                tcp->getLayerPayloadSize()
            );
        }
        return {};
    }

private:
    pcpp::Packet* m_packet;
    const pcpp::RawPacket* m_rawPacket;
    std::chrono::steady_clock::time_point m_timestamp;
    mutable std::string m_sourceIP;
    mutable std::string m_destIP;
};

//=============================================================================
// OnPacket callback class
//=============================================================================
class PacketCallbackHandler {
public:
    PacketCallbackHandler(PacketCallback cb, std::atomic<bool>& isCapturing, std::atomic<uint64_t>& count)
        : m_callback(std::move(cb))
        , m_isCapturing(isCapturing)
        , m_packetCount(count) {}

    void onPacket(pcpp::RawPacket* rawPacket, pcpp::PcapLiveDevice* /*dev*/, void* /*cookie*/) {
        if (!m_isCapturing) {
            return;
        }

        // Parse packet
        pcpp::Packet parsedPacket(rawPacket);

        // Create packet data and call callback
        PcapPacketDataImpl packetData(&parsedPacket, rawPacket);
        if (m_callback) {
            m_callback(packetData);
        }

        m_packetCount++;
    }

    static void staticOnPacket(pcpp::RawPacket* rawPacket, pcpp::PcapLiveDevice* dev, void* cookie) {
        auto* handler = static_cast<PacketCallbackHandler*>(cookie);
        if (handler) {
            handler->onPacket(rawPacket, dev, nullptr);
        }
    }

private:
    PacketCallback m_callback;
    std::atomic<bool>& m_isCapturing;
    std::atomic<uint64_t>& m_packetCount;
};

//=============================================================================
// Capture Implementation
//=============================================================================
class PcapPlusPlusCapture : public PacketCapture {
public:
    PcapPlusPlusCapture() {
        // Get all devices
        const auto& devices = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDevicesList();

        // Find first non-loopback device
        for (const auto& dev : devices) {
            if (!dev->getLoopback()) {
                m_device = dev;
                break;
            }
        }

        // If no device found, take first one
        if (!m_device && !devices.empty()) {
            m_device = devices.front();
        }
    }

    ~PcapPlusPlusCapture() override {
        stopCapture();
        if (m_device && m_device->isOpened()) {
            m_device->close();
        }
    }

    // Non-copyable, movable
    PcapPlusPlusCapture(const PcapPlusPlusCapture&) = delete;
    PcapPlusPlusCapture& operator=(const PcapPlusPlusCapture&) = delete;

    PcapPlusPlusCapture(PcapPlusPlusCapture&& other) noexcept
        : m_device(std::exchange(other.m_device, nullptr))
        , m_isCapturing(other.m_isCapturing.load())
        , m_captureThread(std::move(other.m_captureThread))
        , m_packetCount(other.m_packetCount.load())
        , m_callbackHandler(std::move(other.m_callbackHandler)) {}

    PcapPlusPlusCapture& operator=(PcapPlusPlusCapture&& other) noexcept {
        if (this != &other) {
            stopCapture();
            m_device = std::exchange(other.m_device, nullptr);
            m_isCapturing = other.m_isCapturing.load();
            m_captureThread = std::move(other.m_captureThread);
            m_packetCount = other.m_packetCount.load();
            m_callbackHandler = std::move(other.m_callbackHandler);
        }
        return *this;
    }

    CaptureResult startCapture(std::chrono::seconds duration, PacketCallback callback) override {
        if (!m_device) {
            return std::string("No network device available");
        }

        // Open device
        if (!m_device->isOpened()) {
            if (!m_device->open()) {
                return std::string("Failed to open device: ") + m_device->getName();
            }
        }

        m_isCapturing = true;
        m_packetCount = 0;

        // Create callback handler
        m_callbackHandler = std::make_unique<PacketCallbackHandler>(
            std::move(callback),
            std::ref(m_isCapturing),
            std::ref(m_packetCount)
        );

        // Start capture thread
        m_captureThread = std::thread([this, duration]() {
            auto startTime = std::chrono::steady_clock::now();

            // Start capturing with callback
            m_device->startCapture(
                PacketCallbackHandler::staticOnPacket,
                m_callbackHandler.get()
            );

            // Wait for duration
            std::this_thread::sleep_for(duration);

            // Stop capture
            m_device->stopCapture();
            m_isCapturing = false;

            // Calculate stats
            auto endTime = std::chrono::steady_clock::now();
            CaptureStats stats;
            stats.packets_processed = m_packetCount.load();
            stats.duration = std::chrono::duration_cast<std::chrono::seconds>(
                endTime - startTime
            );
            m_lastStats = stats;
        });

        // Wait for thread to finish
        if (m_captureThread.joinable()) {
            m_captureThread.join();
        }

        return m_lastStats;
    }

    void stopCapture() noexcept override {
        m_isCapturing = false;
        if (m_device && m_device->isOpened()) {
            m_device->stopCapture();
        }
        if (m_captureThread.joinable()) {
            m_captureThread.join();
        }
    }

private:
    pcpp::PcapLiveDevice* m_device{nullptr};
    std::atomic<bool> m_isCapturing{false};
    std::thread m_captureThread;
    std::atomic<uint64_t> m_packetCount{0};
    CaptureStats m_lastStats;
    std::unique_ptr<PacketCallbackHandler> m_callbackHandler;
};

//=============================================================================
// Factory function
//=============================================================================
CreateCaptureResult createPacketCapture() {
    auto capture = std::make_unique<PcapPlusPlusCapture>();
    return capture;
}

} // namespace capture