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
            uint16_t srcPort
        ) noexcept;

        [[nodiscard]] static bool isHttpRequest(std::string_view payload) noexcept;

    private:
        [[nodiscard]] static std::optional<Host> extractHostFromHttp(std::string_view payload) noexcept;
        [[nodiscard]] static std::optional<Host> extractSni(std::string_view payload) noexcept;

        static constexpr std::string_view HTTP_METHODS[] = {
            "GET ", "POST ", "PUT ", "DELETE ", "HEAD ",
            "OPTIONS ", "PATCH ", "TRACE ", "CONNECT "
        };
    };

} // namespace core