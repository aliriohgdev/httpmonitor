#include "core/host.h"
#include <algorithm>
#include <cctype>

namespace core {

    Host::Host(std::string name) noexcept
        : m_name(std::move(name)) {}  // Temporal: sin normalización para debug

    Host::Host(std::string_view name) noexcept
        : m_name(name) {}  // Temporal: sin normalización

    const std::string& Host::get() const noexcept {
        return m_name;
    }

    bool Host::empty() const noexcept {
        return m_name.empty();
    }

    size_t Host::hash() const noexcept {
        return std::hash<std::string>{}(m_name);
    }

    std::string Host::normalized() const noexcept {
        std::string result = m_name;

        // Convertir a minúsculas
        for (char& c : result) {
            c = std::tolower(static_cast<unsigned char>(c));
        }

        // Quitar "www." si está al principio
        if (result.size() > 4 && result.substr(0, 4) == "www.") {
            result = result.substr(4);
        }

        // Quitar puntos al final
        while (!result.empty() && result.back() == '.') {
            result.pop_back();
        }

        return result;
    }

} // namespace core