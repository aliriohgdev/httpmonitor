#pragma once

#include "core/host.h"
#include <chrono>

namespace core {

    enum class Protocol {
        HTTP,
        HTTPS
    };

    class HttpRequest {
    public:
        constexpr HttpRequest(
            Host host,
            Protocol protocol,
            std::chrono::steady_clock::time_point timestamp
        ) noexcept;

        [[nodiscard]] constexpr const Host& host() const noexcept;
        [[nodiscard]] constexpr Protocol protocol() const noexcept;
        [[nodiscard]] constexpr auto timestamp() const noexcept;

    private:
        Host m_host;
        Protocol m_protocol;
        std::chrono::steady_clock::time_point m_timestamp;
    };

} // namespace core