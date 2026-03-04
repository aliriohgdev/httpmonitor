#include "core/statistics.h"
#include <algorithm>
#include <mutex>      // Para std::unique_lock
#include <shared_mutex> // Para std::shared_mutex

namespace core {
    void Statistics::recordHost(const Host &host) noexcept {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_hostCounts[host]++;
        m_totalRequests++;
    }

    uint64_t Statistics::totalRequests() const noexcept {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_totalRequests;
    }

    std::vector<std::pair<Host, uint64_t> > Statistics::getTopHosts(size_t n) const noexcept {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        std::vector<std::pair<Host, uint64_t> > hosts;
        hosts.reserve(m_hostCounts.size());

        for (const auto &[host, count]: m_hostCounts) {
            hosts.emplace_back(host, count);
        }

        const auto sortUntil = hosts.begin() +
                               static_cast<std::ptrdiff_t>(std::min(n, m_hostCounts.size()));

        std::ranges::partial_sort(hosts, sortUntil
                                  ,
                                  [](const auto &a, const auto &b) noexcept {
                                      return a.second > b.second;
                                  }
        );

        return hosts;
    }
} // namespace core
