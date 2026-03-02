#pragma once

#include <chrono>
#include <string_view>
#include <cstdint>

namespace capture {

    struct PacketData {
        virtual ~PacketData() = default;

        [[nodiscard]] virtual std::chrono::steady_clock::time_point timestamp() const noexcept = 0;
        [[nodiscard]] virtual uint16_t sourcePort() const noexcept = 0;
        [[nodiscard]] virtual uint16_t destPort() const noexcept = 0;
        [[nodiscard]] virtual bool isHttp() const noexcept = 0;
        [[nodiscard]] virtual bool isHttps() const noexcept = 0;
        [[nodiscard]] virtual std::string_view sourceIP() const noexcept = 0;
        [[nodiscard]] virtual std::string_view destIP() const noexcept = 0;
        [[nodiscard]] virtual std::string_view payload() const noexcept = 0;

        // Nuevos métodos helpers
        [[nodiscard]] bool isHttpTraffic() const noexcept {
            return isHttp() || isHttps();
        }

        [[nodiscard]] uint16_t serverPort() const noexcept {
            // Asume que el puerto del servidor es el no-ephemeral (>1024 es cliente)
            if (sourcePort() < 1024) return sourcePort();
            if (destPort() < 1024) return destPort();
            return destPort(); // fallback
        }
    };

} // namespace capture