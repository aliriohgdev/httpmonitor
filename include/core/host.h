#pragma once

#include <string>
#include <string_view>
#include <compare>
#include <algorithm>
#include <cctype>

namespace core {

    class Host {
    public:
        constexpr Host() noexcept = default;
        explicit Host(std::string name) noexcept;
        explicit Host(std::string_view name) noexcept;

        [[nodiscard]] const std::string& get() const noexcept;
        [[nodiscard]] bool empty() const noexcept;

        auto operator<=>(const Host&) const = default;

        [[nodiscard]] size_t hash() const noexcept;

        // Nuevo: Obtener host normalizado (minúsculas, sin www)
        [[nodiscard]] std::string normalized() const noexcept;

    private:
        std::string m_name;

        // Helper para normalizar
        static std::string normalize(const std::string& input) noexcept;
    };

} // namespace core

template<>
struct std::hash<core::Host> {
    size_t operator()(const core::Host& host) const noexcept {
        return host.hash();
    }
};