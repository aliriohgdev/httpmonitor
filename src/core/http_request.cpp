#include "core/http_request.h"

namespace core {

    constexpr HttpRequest::HttpRequest(
        Host host,
        Protocol protocol,
        std::chrono::steady_clock::time_point timestamp
    ) noexcept
        : m_host(std::move(host))
        , m_protocol(protocol)
        , m_timestamp(timestamp)
    {}

    constexpr const Host& HttpRequest::host() const noexcept {
        return m_host;
    }

    constexpr Protocol HttpRequest::protocol() const noexcept {
        return m_protocol;
    }

    constexpr auto HttpRequest::timestamp() const noexcept {
        return m_timestamp;
    }

} // namespace core