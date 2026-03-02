# HTTP Monitor

A command-line tool that monitors HTTP traffic on a machine and generates statistics with histograms.

## Description

This program monitors HTTP activity on the machine for a specified time and prints a summary including:
- Total number of HTTP requests detected
- Top 10 requested hosts with histogram visualization

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
   bash
```bash
# Clone the repository
git clone https://github.com/yourusername/http-monitor.git
cd http-monitor

# Create build directory
mkdir build && cd build

# Configure with CMake (adjust PcapPlusPlus path if needed)
cmake -DCMAKE_PREFIX_PATH="/home/linuxbrew/.linuxbrew/opt/pcapplusplus" -DCMAKE_BUILD_TYPE=Debug ..

# Build
make

# Or using cmake directly
cmake --build .
```

## Usage
```bash
# Basic syntax
./http-monitor [/L:<logfile>] <seconds>

# Examples:

# Monitor for 30 seconds (output to console)
./http-monitor 30

# Monitor for 60 seconds and save results to file
./http-monitor /L:results.log 60

# Monitor with custom log path
./http-monitor /L:/tmp/monitor.log 120
```

### Command Line Arguments

| Argument       | Description               | Required |
|----------------|---------------------------|----------|
| `/L:<logfile>`  | Optional log file path     | No       |
| `<seconds>`     | Monitoring time in seconds | Yes      |

#### Exit Codes

| Code | Meaning                                     |
|------|---------------------------------------------|
| 0    | Success                                     |
| 1    | Error (invalid arguments, missing time, etc.) |

### Project Structure
```text
├── include/ # Public headers
│ ├── core/ # Domain value types
│ │ ├── host.h # Host class declaration
│ │ └── http_request.h # HTTP request entity
│ └── io/ # I/O interfaces
│ ├── cli_parser.h # Command line parser
│ └── logger.h # Logger interface
├── src/ # Implementation files
│ ├── core/ # Core domain implementations
│ │ ├── host.cpp # Host implementation
│ │ └── http_request.cpp # HTTP request implementation
│ └── io/ # I/O implementations
│ ├── cli_parser.cpp # CLI parser implementation
│ └── console_logger.cpp # Console logger
├── CMakeLists.txt # Build configuration
├── README.md # This file
```