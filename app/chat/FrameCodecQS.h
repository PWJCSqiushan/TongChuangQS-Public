#pragma once

#include <cstdint>
#include <string>
#include <vector>

/**
 * Length-prefixed UTF-8 frame codec used by TcpPeerQS.
 *
 * A frame contains a four-byte network-order payload length followed by a
 * UTF-8 payload.  The decoder consumes one complete frame and leaves any
 * following bytes in the caller-owned buffer, so both split and coalesced TCP
 * reads are supported.
 */
namespace qs {

class FrameCodecQS final {
public:
    static constexpr std::uint32_t kMaxPayloadBytes = 262144U;

    static std::vector<std::uint8_t> encode(const std::string& payload);

    /**
     * Decode one frame when it is complete.  Returns false when more bytes are
     * needed.  The buffer and output string are unchanged on an incomplete
     * frame.  Invalid lengths and invalid UTF-8 throw std::runtime_error.
     */
    static bool tryDecode(std::vector<std::uint8_t>& buffer,
                          std::string& payload);

    /** Return whether value is well-formed UTF-8, without throwing. */
    static bool isValidUtf8(const std::string& value) noexcept;
};

}  // namespace qs
