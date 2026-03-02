#include "io/cli_parser.h"
#include <cctype>
#include <algorithm>
#include <ranges>
#include <cstdlib>

namespace io {

CliParser::CliParser(int argc, const char* const argv[]) noexcept {
    m_args.reserve(argc - 1);
    for (int i = 1; i < argc; ++i) {
        m_args.emplace_back(argv[i]);
    }
}

bool CliParser::is_log_arg(std::string_view arg) noexcept {
    return arg.starts_with("/L:") && arg.size() > 3;
}

bool CliParser::is_numeric(std::string_view str) noexcept {
    return !str.empty() && std::ranges::all_of(str, [](char c) {
        return std::isdigit(static_cast<unsigned char>(c));
    });
}

std::optional<std::string> CliParser::extract_log_path(
    std::string_view arg) noexcept {

    if (!is_log_arg(arg)) {
        return std::nullopt;
    }
    return std::string(arg.substr(3));
}

ParseResult CliParser::parse() const noexcept {
    MonitorConfig config;
    bool has_time = false;
    bool has_log = false;
    int parsed_args = 0;

    for (const auto& arg : m_args) {
        // Try to parse as log argument (only first one)
        if (!has_log) {
            if (auto path = extract_log_path(arg); path.has_value()) {
                config.log_file = std::move(path.value());
                has_log = true;
                parsed_args++;
                continue;
            }
        }

        // Try to parse as monitor time (only first numeric value)
        if (!has_time) {
            if (is_numeric(arg)) {
                char* end_ptr;
                long val = std::strtol(arg.data(), &end_ptr, 10);

                if (end_ptr == arg.data() || val <= 0) {
                    return std::string("Monitor time must be a positive integer");
                }

                config.monitor_time = std::chrono::seconds(val);
                has_time = true;
                parsed_args++;
                continue;
            } else {
                // We expected a number but got something else
                return std::string("Argument must be numeric: ") + std::string(arg);
            }
        }

        // If we get here, there are extra arguments
        return std::string("Unexpected extra argument: ") + std::string(arg);
    }

    // Final validations
    if (!has_time) {
        return std::string("Monitor time is required (numeric value)");
    }

    if (parsed_args > 2) {
        return std::string("Too many arguments. Usage: http-monitor [/L:<logfile>] <seconds>");
    }

    return config;
}

} // namespace io