#pragma once

#include <chrono>
#include <optional>
#include <string>

namespace capture {

    struct PacketData {
        virtual ~PacketData() = default;

        [[nodiscard]] virtual std::chrono::steady_clock::time_point timestamp() const noexcept = 0;
        [[nodiscard]] virtual uint16_t sourcePort() const noexcept = 0;
        [[nodiscard]] virtual uint16_t destPort() const noexcept = 0;
        [[nodiscard]] virtual bool isHttp() const noexcept = 0;
        [[nodiscard]] virtual bool isHttps() const noexcept = 0;
        [[nodiscard]] virtual std::string_view payload() const noexcept = 0;

        [[nodiscard]] bool isHttpTraffic() const noexcept {
            return isHttp() || isHttps();
        }
    };

} // namespace capture