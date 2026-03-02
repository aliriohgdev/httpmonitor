#pragma once

#include "core/host.h"
#include <vector>
#include <unordered_map>
#include <atomic>
#include <shared_mutex>

namespace core {

    class Statistics {
    public:
        Statistics() = default;

        Statistics(const Statistics&) = delete;
        Statistics& operator=(const Statistics&) = delete;
        Statistics(Statistics&&) = default;
        Statistics& operator=(Statistics&&) = default;

        void recordHost(const Host& host) noexcept;
        [[nodiscard]] uint64_t totalRequests() const noexcept;
        [[nodiscard]] std::vector<std::pair<Host, uint64_t>> getTopHosts(size_t n) const noexcept;

    private:
        mutable std::shared_mutex m_mutex;
        std::unordered_map<Host, uint64_t> m_hostCounts;
        uint64_t m_totalRequests{0};
    };

} // namespace core