#pragma once

#include "core/host.h"
#include <string_view>
#include <optional>
#include <cstdint>

namespace core {

    class HttpParser {
    public:
        [[nodiscard]] static std::optional<Host> extractHost(
            std::string_view payload,
            uint16_t destPort,
            uint16_t srcPort) noexcept;

    private:
        [[nodiscard]] static uint16_t readUint16(const uint8_t* data) noexcept {
            return (static_cast<uint16_t>(data[0]) << 8) | data[1];
        }

        [[nodiscard]] static uint32_t readUint24(const uint8_t* data) noexcept {
            return (static_cast<uint32_t>(data[0]) << 16) |
                   (static_cast<uint32_t>(data[1]) << 8) |
                   data[2];
        }

        [[nodiscard]] static bool safeAdvance(size_t& pos, size_t advance, size_t maxSize) noexcept {
            if (pos + advance > maxSize) return false;
            pos += advance;
            return true;
        }

        [[nodiscard]] static std::optional<Host> extractHostFromHttp(std::string_view payload) noexcept;
        [[nodiscard]] static std::optional<Host> extractSni(std::string_view payload) noexcept;

        static constexpr uint8_t TLS_HANDSHAKE = 0x16;
        static constexpr uint8_t TLS_CLIENT_HELLO = 0x01;
        static constexpr uint16_t TLS_SNI_EXTENSION = 0x0000;
        static constexpr uint8_t TLS_SNI_HOSTNAME_TYPE = 0x00;

        static constexpr size_t TLS_RECORD_HEADER_SIZE = 5;
        static constexpr size_t TLS_RANDOM_SIZE = 32;
        static constexpr size_t TLS_VERSION_SIZE = 2;
    };

} // namespace core