#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <variant>

namespace io {

    struct MonitorConfig {
        std::optional<std::string> log_file;
        std::chrono::seconds monitor_time;

        [[nodiscard]] bool has_log_file() const noexcept {
            return log_file.has_value();
        }
    };

    using ParseResult = std::variant<MonitorConfig, std::string>;

    class CliParser {
    public:
        explicit CliParser(int argc, const char* const argv[]) noexcept;

        CliParser(const CliParser&) = delete;
        CliParser& operator=(const CliParser&) = delete;
        CliParser(CliParser&&) = default;
        CliParser& operator=(CliParser&&) = default;
        ~CliParser() = default;

        [[nodiscard]] ParseResult parse() const noexcept;

    private:
        std::vector<std::string_view> m_args;

        [[nodiscard]] static bool is_log_arg(std::string_view arg) noexcept;
        [[nodiscard]] static bool is_numeric(std::string_view str) noexcept;
        [[nodiscard]] static std::optional<std::string> extract_log_path(
            std::string_view arg) noexcept;
    };

} // namespace io