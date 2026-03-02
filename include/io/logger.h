#pragma once

#include <memory>
#include <string_view>

namespace io {

    class Logger {
    public:
        virtual ~Logger() = default;

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        Logger(Logger&&) = default;
        Logger& operator=(Logger&&) = default;

        virtual void info(std::string_view message) const = 0;
        virtual void error(std::string_view message) const = 0;

    protected:
        Logger() = default;
    };

    [[nodiscard]] std::unique_ptr<Logger> create_console_logger() noexcept;

} // namespace io