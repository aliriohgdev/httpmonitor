#include "core/http_parser.h"
#include <cstring>
#include <arpa/inet.h>

namespace core {

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
    constexpr std::string_view HOST_HEADER = "Host:";

    size_t pos = payload.find(HOST_HEADER);
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }

    pos += HOST_HEADER.size();

    pos = payload.find_first_not_of(" \t", pos);
    if (pos == std::string_view::npos) return std::nullopt;

    size_t end = pos;
    while (end < payload.size() && payload[end] != '\r' &&
           payload[end] != '\n' && payload[end] != ' ' && payload[end] != '\t') {
        end++;
    }

    std::string_view host = payload.substr(pos, end - pos);

    if (auto colon = host.find(':'); colon != std::string_view::npos) {
        host = host.substr(0, colon);
    }

    return host.empty() ? std::nullopt : std::optional<Host>(Host(std::string(host)));
}

std::optional<Host> HttpParser::extractSni(std::string_view payload) noexcept {
    const uint8_t* data = reinterpret_cast<const uint8_t*>(payload.data());
    size_t size = payload.size();
    size_t pos = 0;

    // Validar tamaño mínimo y tipo de registro
    if (size < TLS_RECORD_HEADER_SIZE || data[0] != TLS_HANDSHAKE) {
        return std::nullopt;
    }

    pos = TLS_RECORD_HEADER_SIZE;

    // Buscar Client Hello
    while (pos + 4 < size) {
        if (data[pos] == TLS_CLIENT_HELLO) {
            pos++; // Saltar tipo de handshake

            // Longitud del Client Hello (3 bytes)
            if (pos + 3 > size) return std::nullopt;
            pos += 3;

            // Saltar versión (2) + random (32)
            if (!safeAdvance(pos, TLS_VERSION_SIZE + TLS_RANDOM_SIZE, size)) {
                return std::nullopt;
            }

            // Session ID
            if (pos >= size) return std::nullopt;
            uint8_t session_id_len = data[pos++];
            if (!safeAdvance(pos, session_id_len, size)) return std::nullopt;

            // Cipher Suites
            if (pos + 2 > size) return std::nullopt;
            uint16_t cipher_len = readUint16(data + pos);
            if (!safeAdvance(pos, 2 + cipher_len, size)) return std::nullopt;

            // Compression Methods
            if (pos >= size) return std::nullopt;
            uint8_t comp_len = data[pos++];
            if (!safeAdvance(pos, comp_len, size)) return std::nullopt;

            // Extensiones
            if (pos + 2 > size) return std::nullopt;
            uint16_t ext_len = readUint16(data + pos);
            pos += 2;

            size_t ext_end = pos + ext_len;
            while (pos + 4 <= ext_end && pos < size) {
                uint16_t ext_type = readUint16(data + pos);
                uint16_t ext_data_len = readUint16(data + pos + 2);
                pos += 4;

                // Buscar extensión SNI
                if (ext_type == TLS_SNI_EXTENSION) {
                    // SNI List length (2 bytes)
                    if (pos + 2 > ext_end) return std::nullopt;
                    pos += 2;

                    // Server name type (debe ser 0x00 para host_name)
                    if (pos >= ext_end || data[pos] != TLS_SNI_HOSTNAME_TYPE) {
                        return std::nullopt;
                    }
                    pos++;

                    // Server name length
                    if (pos + 2 > ext_end) return std::nullopt;
                    uint16_t name_len = readUint16(data + pos);
                    pos += 2;

                    // Server name (el host)
                    if (pos + name_len <= ext_end) {
                        return Host(std::string(
                            reinterpret_cast<const char*>(data + pos),
                            name_len
                        ));
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