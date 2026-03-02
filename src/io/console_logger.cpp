#include "io/logger.h"
#include <iostream>
#include <syncstream>

namespace io {

    class ConsoleLogger final : public Logger {
    public:
        void info(std::string_view message) const override {
            std::osyncstream{std::cout} << "[INFO] " << message << '\n';
        }

        void error(std::string_view message) const override {
            std::osyncstream{std::cerr} << "[ERROR] " << message << '\n';
        }
    };

    std::unique_ptr<Logger> create_console_logger() noexcept {
        return std::make_unique<ConsoleLogger>();
    }

} // namespace io