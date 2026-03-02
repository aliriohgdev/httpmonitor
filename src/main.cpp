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
#include <cmath>      // ← NUEVO: para std::log

class HttpMonitor {
public:
    explicit HttpMonitor(
        std::unique_ptr<io::Logger> logger,
        std::unique_ptr<capture::PacketCapture> capture
    ) noexcept
        : m_logger(std::move(logger))
        , m_capture(std::move(capture))
        , m_stats(std::make_unique<core::Statistics>()) {}

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

        m_logger->info("Starting packet capture...");

        auto capture_result = m_capture->startCapture(
            config.monitor_time,
            [this](const capture::PacketData& packet) {
                if (!packet.isHttpTraffic()) return;

                auto host = core::HttpParser::extractHost(
                    packet.payload(),
                    packet.destPort(),
                    packet.sourcePort()
                );

                if (host.has_value()) {
                    m_stats->recordHost(host.value());

                    // Debug: ver qué hosts se están detectando
                    // m_logger->info("Detected: " + std::string(host->get()) +
                    //               " (" + std::string(packet.isHttp() ? "HTTP" : "HTTPS") + ")");
                }
            }
        );

        if (std::holds_alternative<std::string>(capture_result)) {
            m_logger->error("Capture failed: " + std::get<std::string>(capture_result));
            return 1;
        }

        const auto& stats = std::get<capture::CaptureStats>(capture_result);

        printSummary(stats);

        return 0;
    }

private:
    void printSummary(const capture::CaptureStats& captureStats) const noexcept {
        m_logger->info("\n=== HTTP Monitor Summary ===");
        m_logger->info("Capture duration: " + std::to_string(captureStats.duration.count()) + " seconds");
        m_logger->info("Total packets processed: " + std::to_string(captureStats.packets_processed));

        auto totalRequests = m_stats->totalRequests();
        m_logger->info("Total HTTP/HTTPS requests detected: " + std::to_string(totalRequests));

        if (totalRequests > 0) {
            auto topHosts = m_stats->getTopHosts(10);

            if (topHosts.empty()) {
                m_logger->info("  No hosts detected");
                return;
            }

            m_logger->info("\nTop 10 requested hosts:");

            // Calcular el ancho máximo para el dominio (para alinear)
            size_t maxHostWidth = 0;
            uint64_t maxCount = 0;
            for (const auto& [host, count] : topHosts) {
                maxHostWidth = std::max(maxHostWidth, host.get().size());
                maxCount = std::max(maxCount, count);
            }
            maxHostWidth = std::min(maxHostWidth, size_t(40)); // Limitar a 40 chars

            // Mostrar cada host con histograma
            for (const auto& [host, count] : topHosts) {
                // Formatear el dominio (truncar si es muy largo)
                std::string hostStr = host.get();
                if (hostStr.size() > maxHostWidth) {
                    hostStr = hostStr.substr(0, maxHostWidth - 3) + "...";
                }

                // Calcular barra del histograma (escala logarítmica para mejor visualización)
                int barLength = 0;
                if (maxCount > 0) {
                    // Usar escala logarítmica si hay mucha diferencia
                    if (maxCount > 1000) {
                        double ratio = std::log(static_cast<double>(count + 1)) /
                                      std::log(static_cast<double>(maxCount + 1));
                        barLength = static_cast<int>(ratio * 50);
                    } else {
                        barLength = (count * 50) / maxCount;
                    }
                }

                // Crear la barra
                std::string bar(barLength, '*');

                // Formatear línea alineada
                std::ostringstream line;
                line << "  " << std::left << std::setw(maxHostWidth + 2) << hostStr
                     << std::right << std::setw(8) << count << "  " << bar;

                m_logger->info(line.str());
            }

            // Mostrar leyenda
            m_logger->info("\n  * Histogram scale: max count = " + std::to_string(maxCount));
            if (maxCount > 1000) {
                m_logger->info("  * Using logarithmic scale for better visualization");
            }

        } else {
            m_logger->info("\nNo HTTP/HTTPS requests detected during capture period");
            m_logger->info("Try generating traffic:");
            m_logger->info("  curl http://example.com");
            m_logger->info("  curl https://github.com");
            m_logger->info("  curl https://www.google.com");
        }

        m_logger->info("============================\n");
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
        std::move(std::get<std::unique_ptr<capture::PacketCapture>>(capture_result))
    );

    return monitor.run(argc, argv);
}