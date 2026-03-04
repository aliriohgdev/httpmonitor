#include "core/http_parser.h"
#include <cstring>
#include <cctype>
#include <algorithm>
#include <arpa/inet.h>
#include <iostream>  // Temporal para debug

namespace core {

bool HttpParser::isHttpRequest(std::string_view payload) noexcept {
    if (payload.empty()) return false;

    for (const auto& method : HTTP_METHODS) {
        if (payload.size() >= method.size() &&
            memcmp(payload.data(), method.data(), method.size()) == 0) {
            return true;
        }
    }
    return false;
}

std::optional<Host> HttpParser::extractHost(
    std::string_view payload,
    uint16_t destPort,
    uint16_t srcPort) noexcept {

    // HTTP (puerto 80)
    if (destPort == 80 || srcPort == 80) {
        return extractHostFromHttp(payload);
    }

    // HTTPS (puerto 443)
    if (destPort == 443 || srcPort == 443) {
        return extractSni(payload);
    }

    return std::nullopt;
}

std::optional<Host> HttpParser::extractHostFromHttp(std::string_view payload) noexcept {
    // Buscar "Host:" en el payload
    constexpr std::string_view HOST_HEADER = "\r\nHost:";
    constexpr std::string_view HOST_HEADER2 = "\nHost:";

    size_t pos = payload.find(HOST_HEADER);
    if (pos == std::string_view::npos) {
        pos = payload.find(HOST_HEADER2);
    }
    if (pos == std::string_view::npos) {
        // También buscar al principio del payload (primera línea)
        pos = payload.find("Host:");
        if (pos == std::string_view::npos) return std::nullopt;
    }

    pos = payload.find_first_not_of(": \t\r\n", pos + (payload[pos] == '\r' ? 7 : 5));
    if (pos == std::string_view::npos) return std::nullopt;

    auto end = payload.find_first_of("\r\n", pos);
    if (end == std::string_view::npos) {
        end = payload.size();
    }

    std::string_view host = payload.substr(pos, end - pos);

    while (!host.empty() && std::isspace(static_cast<unsigned char>(host.front()))) {
        host.remove_prefix(1);
    }
    while (!host.empty() && std::isspace(static_cast<unsigned char>(host.back()))) {
        host.remove_suffix(1);
    }

    if (const auto colon = host.find(':'); colon != std::string_view::npos) {
        host = host.substr(0, colon);
    }

    if (!host.empty()) {
        return Host(std::string(host));
    }

    return std::nullopt;
}

std::optional<Host> HttpParser::extractSni(std::string_view payload) noexcept {
    // Implementación de SNI (igual que antes)
    if (payload.size() < 5) return std::nullopt;

    if (static_cast<uint8_t>(payload[0]) != 0x16) {
        return std::nullopt;
    }

    size_t pos = 5;

    while (pos + 4 < payload.size()) {
        if (static_cast<uint8_t>(payload[pos]) == 0x01) {
            pos++;

            if (pos + 3 > payload.size()) return std::nullopt;

            pos += 3;

            if (pos + 34 > payload.size()) return std::nullopt;
            pos += 34;

            const auto session_id_len = static_cast<uint8_t>(payload[pos++]);
            pos += session_id_len;

            if (pos + 2 > payload.size()) return std::nullopt;
            uint16_t cipher_len = (static_cast<uint8_t>(payload[pos]) << 8) |
                                   static_cast<uint8_t>(payload[pos+1]);
            pos += 2 + cipher_len;

            if (pos >= payload.size()) return std::nullopt;
            uint8_t comp_len = static_cast<uint8_t>(payload[pos++]);
            pos += comp_len;

            if (pos + 2 > payload.size()) return std::nullopt;
            uint16_t ext_len = (static_cast<uint8_t>(payload[pos]) << 8) |
                                static_cast<uint8_t>(payload[pos+1]);
            pos += 2;

            size_t ext_end = pos + ext_len;
            while (pos + 4 <= ext_end && pos < payload.size()) {
                uint16_t ext_type = (static_cast<uint8_t>(payload[pos]) << 8) |
                                     static_cast<uint8_t>(payload[pos+1]);
                uint16_t ext_data_len = (static_cast<uint8_t>(payload[pos+2]) << 8) |
                                         static_cast<uint8_t>(payload[pos+3]);
                pos += 4;

                if (ext_type == 0x0000) {
                    if (pos + 2 > payload.size()) return std::nullopt;

                    pos += 2;

                    if (pos >= payload.size() || static_cast<uint8_t>(payload[pos]) != 0x00) {
                        return std::nullopt;
                    }
                    pos++;

                    if (pos + 2 > payload.size()) return std::nullopt;
                    uint16_t name_len = (static_cast<uint8_t>(payload[pos]) << 8) |
                                         static_cast<uint8_t>(payload[pos+1]);
                    pos += 2;

                    if (pos + name_len <= payload.size()) {
                        return Host(std::string(payload.substr(pos, name_len)));
                    }
                }
                pos += ext_data_len;
            }
            break;
        }
        pos++;
    }

    return std::nullopt;
}

} // namespace core