#include "io/cli_parser.h"
#include "io/logger.h"
#include "capture/packet_capture.h"
#include "capture/packet_data.h"
#include "core/http_parser.h"
#include "core/statistics.h"
#include <memory>
#include <utility>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <fstream>

class HttpMonitor {
public:
    explicit HttpMonitor(
        std::unique_ptr<io::Logger> logger,
        std::unique_ptr<capture::PacketCapture> capture
    ) noexcept
        : m_logger(std::move(logger))
          , m_capture(std::move(capture))
          , m_stats(std::make_unique<core::Statistics>()) {
    }

    HttpMonitor(const HttpMonitor &) = delete;

    HttpMonitor &operator=(const HttpMonitor &) = delete;

    HttpMonitor(HttpMonitor &&) = default;

    HttpMonitor &operator=(HttpMonitor &&) = default;

    ~HttpMonitor() = default;

    [[nodiscard]] int run(int argc, const char *const argv[]) const {
        io::CliParser parser(argc, argv);
        auto result = parser.parse();

        if (std::holds_alternative<std::string>(result)) {
            m_logger->error(std::get<std::string>(result));
            m_logger->info("Usage: http-monitor [/L:<logfile>] <seconds>");
            return 1;
        }

        const auto &config = std::get<io::MonitorConfig>(result);

        m_logger->info("Starting HTTP monitor for " +
                       std::to_string(config.monitor_time.count()) + " seconds");

        if (config.has_log_file()) {
            m_logger->info("Logging to: " + config.log_file.value());
        }

        m_logger->info("Starting packet capture...");

        auto capture_result = m_capture->startCapture(
            config.monitor_time,
            [this](const capture::PacketData &packet) {
                if (!packet.isHttpTraffic()) return;

                auto host = core::HttpParser::extractHost(
                    packet.payload(),
                    packet.destPort(),
                    packet.sourcePort()
                );

                if (host.has_value()) {
                    m_stats->recordHost(host.value());
                }
            }
        );

        if (std::holds_alternative<std::string>(capture_result)) {
            m_logger->error("Capture failed: " + std::get<std::string>(capture_result));
            return 1;
        }

        const auto& stats = std::get<capture::CaptureStats>(capture_result);

        std::string summary = generateSummary(stats, config.log_file);
        m_logger->info(summary);

        if (config.has_log_file()) {
            saveToFile(config.log_file.value(), summary);
        }

        return 0;
    }

private:
    std::string generateSummary(
        const capture::CaptureStats &captureStats,
        const std::optional<std::string> &logFile
    ) const noexcept {
        std::ostringstream oss;
        auto totalRequests = m_stats->totalRequests();

        oss << "\n=== HTTP Monitor Summary ===\n";
        oss << "Duration: " << captureStats.duration.count() << " seconds\n";
        oss << "Packets processed: " << captureStats.packets_processed << "\n";
        oss << "Total HTTP/HTTPS requests: " << totalRequests << "\n";

        if (logFile.has_value()) {
            oss << "Log file: " << logFile.value() << "\n";
        }

        if (totalRequests > 0) {
            auto topHosts = m_stats->getTopHosts(10);

            if (!topHosts.empty()) {
                oss << "\nTop 10 requested hosts:\n";

                size_t maxHostWidth = 0;
                uint64_t maxCount = 0;
                for (const auto &[host, count]: topHosts) {
                    maxHostWidth = std::max(maxHostWidth, host.get().size());
                    maxCount = std::max(maxCount, count);
                }
                maxHostWidth = std::min(maxHostWidth, size_t(40));

                for (const auto &[host, count]: topHosts) {
                    std::string hostStr = host.get();
                    if (hostStr.size() > maxHostWidth) {
                        hostStr = hostStr.substr(0, maxHostWidth - 3) + "...";
                    }

                    int barLength = 0;
                    if (maxCount > 0) {
                        if (maxCount > 1000) {
                            double ratio = std::log(static_cast<double>(count + 1)) /
                                           std::log(static_cast<double>(maxCount + 1));
                            barLength = static_cast<int>(ratio * 50);
                        } else {
                            barLength = (count * 50) / maxCount;
                        }
                    }

                    std::string bar(barLength, '*');

                    oss << "  " << std::left << std::setw(maxHostWidth + 2) << hostStr
                            << std::right << std::setw(8) << count << "  " << bar << "\n";
                }

                oss << "\n  * Max count: " << maxCount;
                if (maxCount > 1000) oss << " (log scale)";
                oss << "\n";
            }
        } else {
            oss << "\nNo HTTP/HTTPS requests detected\n";
            oss << "Try: curl http://example.com or curl https://github.com\n";
        }

        oss << "============================\n";
        return oss.str();
    }

    void saveToFile(const std::string &filename, const std::string &content) const noexcept {
        std::ofstream file(filename, std::ios::app);
        if (file.is_open()) {
            file << content << std::endl;
            m_logger->info("Summary appended to: " + filename);
        } else {
            m_logger->error("Failed to open log file: " + filename);
        }
    }

    std::unique_ptr<io::Logger> m_logger;
    std::unique_ptr<capture::PacketCapture> m_capture;
    std::unique_ptr<core::Statistics> m_stats;
};

int main(int argc, const char* const argv[]) {
    auto logger = io::create_console_logger();

    auto capture_result = capture::createPacketCapture();
    if (std::holds_alternative<std::string>(capture_result)) {
        logger->error("Failed to create packet capture: " +
                      std::get<std::string>(capture_result));
        return 1;
    }

    HttpMonitor monitor(
        std::move(logger),
        std::move(std::get<std::unique_ptr<capture::PacketCapture> >(capture_result))
    );

    return monitor.run(argc, argv);
}
