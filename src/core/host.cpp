#include "core/host.h"

namespace core {

    Host::Host(std::string name) noexcept
        : m_name(std::move(name)) {}

    Host::Host(std::string_view name) noexcept
        : m_name(name) {}

    const std::string& Host::get() const noexcept {
        return m_name;
    }

    bool Host::empty() const noexcept {
        return m_name.empty();
    }

    size_t Host::hash() const noexcept {
        return std::hash<std::string>{}(m_name);
    }

} // namespace core