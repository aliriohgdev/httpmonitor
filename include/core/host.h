#pragma once

#include <string>
#include <string_view>
#include <compare>

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

    private:
        std::string m_name;
    };

} // namespace core

template<>
struct std::hash<core::Host> {
    size_t operator()(const core::Host& host) const noexcept {
        return host.hash();
    }
};