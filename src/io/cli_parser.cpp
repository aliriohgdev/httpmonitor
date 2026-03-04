#include "io/cli_parser.h"

#include <charconv>
#include <system_error>

namespace io {

    CliParser::CliParser(int argc, const char* const argv[]) noexcept {
        if (argc > 1) {
            m_args.reserve(static_cast<std::size_t>(argc - 1));
            for (int i = 1; i < argc; ++i) {
                m_args.emplace_back(argv[i]);
            }
        }
    }

    bool CliParser::is_log_arg(std::string_view arg) noexcept {
        return arg.starts_with("/L:") && arg.size() > 3;
    }

    std::optional<std::string>
    CliParser::extract_log_path(std::string_view arg) noexcept {
        if (!is_log_arg(arg)) {
            return std::nullopt;
        }
        return std::string(arg.substr(3));
    }

    std::optional<std::chrono::seconds>
    CliParser::parse_seconds(std::string_view arg) noexcept {

        int value = 0;

        auto [ptr, ec] = std::from_chars(
            arg.data(),
            arg.data() + arg.size(),
            value
        );

        if (ec != std::errc{} ||
            ptr != arg.data() + arg.size() ||
            value <= 0) {
            return std::nullopt;
            }

        return std::chrono::seconds(value);
    }

    ParseResult CliParser::parse() const noexcept {

        MonitorConfig config;
        bool time_set = false;

        for (const auto& arg : m_args) {

            // Log argument
            if (auto path = extract_log_path(arg); path) {
                if (config.log_file) {
                    return "Log file specified more than once";
                }
                config.log_file = std::move(*path);
                continue;
            }

            // Time argument
            if (auto seconds = parse_seconds(arg); seconds) {
                if (time_set) {
                    return "Monitor time specified more than once";
                }
                config.monitor_time = *seconds;
                time_set = true;
                continue;
            }

            return "Invalid argument: " + std::string(arg);
        }

        if (!time_set) {
            return "Monitor time is required (positive integer)";
        }

        return config;
    }

} // namespace io