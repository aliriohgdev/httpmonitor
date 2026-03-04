# HTTP Monitor

A command-line tool that monitors HTTP and HTTPS traffic on a machine and generates statistics with histograms.

## Description

This program monitors HTTP and HTTPS activity on the machine for a specified time and prints a summary including:
- Total number of HTTP/HTTPS requests detected
- Top 10 requested hosts with histogram visualization

For **HTTP (port 80)**, the hostname is extracted directly from the `Host:` header in the request payload. For **HTTPS (port 443)**, the hostname is extracted from the **SNI (Server Name Indication)** field in the TLS ClientHello handshake — which travels unencrypted — without decrypting any traffic.

## Pre-built Binary

A ready-to-run binary is available directly in the repository as `http-monitor`.

```bash
# Clone and run — no build step required
git clone https://github.com/aliriohgdev/httpmonitor
cd httpmonitor
sudo ./http-monitor 30
```

> **Note:** Root privileges are required to capture network packets.

## Requirements

- **C++20** compatible compiler (GCC 11+, Clang 14+, or MSVC 2022+)
- **CMake 3.28** or higher
- **PcapPlusPlus** library
- **libpcap** development files

## Building

### 1. Install dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install libpcap-dev cmake build-essential
brew install pcapplusplus
```

### 2. Build the project

```bash
# Clone the repository
git clone https://github.com/aliriohgdev/httpmonitor
cd http-monitor

# Configure with CMake (adjust PcapPlusPlus path if needed)
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build
```

## Usage

```bash
# Basic syntax
./http-monitor [/L:<logfile>] <seconds>

# Monitor for 30 seconds (output to console)
./http-monitor 30

# Monitor for 60 seconds and save results to file
./http-monitor /L:results.log 60

# Monitor with custom log path
./http-monitor /L:/tmp/monitor.log 120
```

### Command Line Arguments

| Argument        | Description                | Required |
|-----------------|----------------------------|----------|
| `/L:<logfile>`  | Optional log file path     | No       |
| `<seconds>`     | Monitoring time in seconds | Yes      |

### Exit Codes

| Code | Meaning                                        |
|------|------------------------------------------------|
| 0    | Success                                        |
| 1    | Error (invalid arguments, missing time, etc.)  |

## Project Structure

```text
├── include/                    # Public headers
│   ├── core/                   # Domain value types
│   │   ├── host.h              # Host class declaration
│   │   └── http_request.h      # HTTP request entity
│   └── io/                     # I/O interfaces
│       ├── cli_parser.h        # Command line parser
│       └── logger.h            # Logger interface
├── src/                        # Implementation files
│   ├── core/                   # Core domain implementations
│   │   ├── host.cpp            # Host implementation
│   │   └── http_request.cpp    # HTTP request implementation
│   └── io/                     # I/O implementations
│       ├── cli_parser.cpp      # CLI parser implementation
│       └── console_logger.cpp  # Console logger
├── CMakeLists.txt              # Build configuration
├── http-monitor                # Pre-built binary
└── README.md                   # This file
```

## Algorithm Complexity Analysis

The histogram generation involves three distinct phases with different complexity characteristics.

### Phase 1 — Request accumulation (during capture)

Each incoming packet is processed and its hostname is inserted or incremented in an `unordered_map`. Access is protected by a `std::shared_mutex` using a `unique_lock` (exclusive write), allowing safe concurrent access from the capture thread while `getTopHosts()` can read concurrently via `shared_lock`:

```cpp
std::unique_lock<std::shared_mutex> lock(m_mutex);
m_hostCounts[host]++;   // O(1) amortized
m_totalRequests++;
```

```
unordered_map lookup + increment → O(1) amortized per request
Total for N requests             → O(N)
```

### Phase 2 — Top-k ranking (once, at report time)

When the capture ends, `getTopHosts(n)` copies the map entries to a vector and uses `std::ranges::partial_sort` to bring only the top `n` elements into sorted order:

```cpp
std::ranges::partial_sort(hosts, sortUntil, comparator); // O(H log k)
```

```
Copy map entries to vector             → O(H)
std::ranges::partial_sort up to k=10  → O(H log k)

Total ranking phase                    → O(H log k)
```

`partial_sort` only guarantees sorted order for the first `k` elements, making it strictly more efficient than a full `std::sort` which would be O(H log H).

### Phase 3 — Bar rendering with adaptive scale

This is the most nuanced phase. The bar length is not always computed the same way — the algorithm switches between **linear** and **logarithmic** scale depending on the magnitude of `maxCount`:

**Linear scale** — when `maxCount <= 1000`:
```cpp
barLength = (count * 50) / maxCount;    // O(1) per bar
```
A direct proportion: the top host gets 50 `*` characters and every other host is scaled relative to it. Simple and visually accurate for moderate traffic volumes.

**Logarithmic scale** — when `maxCount > 1000`:
```cpp
double ratio = std::log(count + 1) / std::log(maxCount + 1);
barLength = static_cast<int>(ratio * 50);   // O(1) per bar
```
When traffic is heavily skewed — for example, one host with 50,000 requests and another with 10 — linear scale would render the low-traffic host with a near-invisible bar. Log scale compresses the range, keeping all bars readable. The `+1` offset prevents `log(0)` on zero-count entries.

The threshold of 1,000 is a pragmatic choice: below it, linear differences between hosts are visually meaningful; above it, they are typically too large for a fixed-width bar to represent faithfully.

The rendering itself requires two passes over k elements:

```
Pass 1 — compute maxHostWidth and maxCount  → O(k)
Pass 2 — render each bar                   → O(k)

Total bar rendering                         → O(k) = O(10), constant in practice
```

### Summary

| Phase                      | Complexity       | Notes                                                    |
|----------------------------|------------------|----------------------------------------------------------|
| Accumulation               | O(N)             | N = total requests, O(1) per insert via `unordered_map`  |
| Ranking (`partial_sort`)   | O(H log k)       | H = unique hosts, k = 10                                 |
| Bar rendering              | O(k)             | Two passes over k ≤ 10 entries                           |
| **End-to-end**             | **O(N)**         | Accumulation dominates; H grows much slower than N       |

The dominant cost is accumulation O(N). The ranking runs once at the end and the bar rendering is effectively constant. The adaptive scale (linear vs. logarithmic) has no impact on time complexity — both branches are O(1) per bar — but significantly improves the visual quality of the output under high-skew traffic conditions.

## Limitations

- **HTTPS hostname detection relies on SNI only.** The hostname is extracted from the TLS ClientHello SNI extension, which is unencrypted. However, if a client does not send SNI (rare but possible), the HTTPS connection will not be counted.
- **Encrypted HTTPS payload is not inspected.** Only the destination hostname is recorded — URLs, paths, and request bodies within HTTPS sessions remain private.
- Requires **root/sudo** privileges to open a raw network capture device.
- Tested on **Linux** (Ubuntu 24.04+). macOS and Windows are not guaranteed to work.

## Third-Party Software

| Library       | Version | License | Purpose                                      | Link |
|---------------|---------|---------|----------------------------------------------|------|
| PcapPlusPlus  | 23.x+   | MIT     | Packet capture and protocol layer parsing    | [pcapplusplus.github.io](https://pcapplusplus.github.io/) |
| libpcap       | 1.9+    | BSD     | Low-level packet capture backend (required by PcapPlusPlus) | [tcpdump.org](https://www.tcpdump.org/) |

## AI Assistance

[Claude Code](https://claude.ai/code) (Anthropic) was used throughout the development of this project as a **pair programming assistant**.

The workflow was collaborative: architectural decisions, data structure choices, concurrency strategy, and overall design were driven by me. Claude Code was used to discuss tradeoffs, get a second opinion on implementation details, and accelerate the writing of boilerplate — in the same way one would use a knowledgeable colleague to think through a problem out loud.

Specific areas where Claude Code assisted:

- Discussing the dual HTTP/HTTPS capture strategy (Host header vs. SNI extraction)
- Reviewing thread safety decisions around `std::shared_mutex`
- Suggesting `std::ranges::partial_sort` over a full sort for the top-k ranking
- Drafting and iterating on sections of this README

All code was reviewed, understood, and validated by me before being committed. No code was blindly accepted.|