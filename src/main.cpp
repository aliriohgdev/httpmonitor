#include "io/cli_parser.h"
#include "io/logger.h"
#include <memory>
#include <utility>

class HttpMonitor {
public:
    explicit HttpMonitor(std::unique_ptr<io::Logger> logger) noexcept
        : m_logger(std::move(logger)) {}

    HttpMonitor(const HttpMonitor&) = delete;
    HttpMonitor& operator=(const HttpMonitor&) = delete;
    HttpMonitor(HttpMonitor&&) = default;
    HttpMonitor& operator=(HttpMonitor&&) = default;
    ~HttpMonitor() = default;

    [[nodiscard]] int run(int argc, const char* const argv[]) const {
        io::CliParser parser(argc, argv);
        auto result = parser.parse();

        if (std::holds_alternative<std::string>(result)) {
            m_logger->error(std::get<std::string>(result));
            m_logger->info("Usage: http-monitor [/L:<logfile>] <seconds>");
            return 1;
        }

        const auto& config = std::get<io::MonitorConfig>(result);

        m_logger->info("Starting HTTP monitor for " +
                      std::to_string(config.monitor_time.count()) + " seconds");

        if (config.has_log_file()) {
            m_logger->info("Logging to: " + config.log_file.value());
        }

        m_logger->info("Capture not yet implemented");

        return 0;
    }

private:
    std::unique_ptr<io::Logger> m_logger;
};

int main(int argc, const char* const argv[]) {
    auto logger = io::create_console_logger();
    HttpMonitor monitor(std::move(logger));
    return monitor.run(argc, argv);
}