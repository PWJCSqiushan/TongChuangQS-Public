#include "FrameCodecQS.h"

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace qs {

namespace {

bool isContinuationByte(const unsigned char value) noexcept {
    return (value & 0xC0U) == 0x80U;
}

}  // namespace

std::vector<std::uint8_t> FrameCodecQS::encode(const std::string& payload) {
    if (payload.empty()) {
        throw std::runtime_error("消息不能为空");
    }
    if (payload.size() > kMaxPayloadBytes) {
        throw std::runtime_error("消息过长");
    }
    if (!isValidUtf8(payload)) {
        throw std::runtime_error("消息不是有效的 UTF-8 文本");
    }

    const auto length = static_cast<std::uint32_t>(payload.size());
    std::vector<std::uint8_t> frame;
    frame.reserve(payload.size() + 4U);
    frame.push_back(static_cast<std::uint8_t>((length >> 24U) & 0xFFU));
    frame.push_back(static_cast<std::uint8_t>((length >> 16U) & 0xFFU));
    frame.push_back(static_cast<std::uint8_t>((length >> 8U) & 0xFFU));
    frame.push_back(static_cast<std::uint8_t>(length & 0xFFU));
    frame.insert(frame.end(), payload.begin(), payload.end());
    return frame;
}

bool FrameCodecQS::tryDecode(std::vector<std::uint8_t>& buffer,
                             std::string& payload) {
    if (buffer.size() < 4U) {
        return false;
    }

    const auto length = (static_cast<std::uint32_t>(buffer[0]) << 24U) |
                        (static_cast<std::uint32_t>(buffer[1]) << 16U) |
                        (static_cast<std::uint32_t>(buffer[2]) << 8U) |
                        static_cast<std::uint32_t>(buffer[3]);
    if (length == 0U || length > kMaxPayloadBytes) {
        throw std::runtime_error("收到的消息长度无效");
    }

    const auto frameSize = static_cast<std::size_t>(length) + 4U;
    if (buffer.size() < frameSize) {
        return false;
    }

    std::string decoded(
        reinterpret_cast<const char*>(buffer.data() + 4U),
        static_cast<std::size_t>(length));
    if (!isValidUtf8(decoded)) {
        throw std::runtime_error("收到的消息不是有效的 UTF-8 文本");
    }

    buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(frameSize));
    payload = std::move(decoded);
    return true;
}

bool FrameCodecQS::isValidUtf8(const std::string& value) noexcept {
    const auto* bytes = reinterpret_cast<const unsigned char*>(value.data());
    const std::size_t size = value.size();
    std::size_t index = 0U;

    while (index < size) {
        const unsigned char first = bytes[index];
        if (first <= 0x7FU) {
            ++index;
            continue;
        }

        if (first >= 0xC2U && first <= 0xDFU) {
            if (index + 1U >= size || !isContinuationByte(bytes[index + 1U])) {
                return false;
            }
            index += 2U;
            continue;
        }

        if (first == 0xE0U) {
            if (index + 2U >= size || bytes[index + 1U] < 0xA0U ||
                bytes[index + 1U] > 0xBFU ||
                !isContinuationByte(bytes[index + 2U])) {
                return false;
            }
            index += 3U;
            continue;
        }

        if (first >= 0xE1U && first <= 0xECU) {
            if (index + 2U >= size || !isContinuationByte(bytes[index + 1U]) ||
                !isContinuationByte(bytes[index + 2U])) {
                return false;
            }
            index += 3U;
            continue;
        }

        if (first == 0xEDU) {
            if (index + 2U >= size || bytes[index + 1U] < 0x80U ||
                bytes[index + 1U] > 0x9FU ||
                !isContinuationByte(bytes[index + 2U])) {
                return false;
            }
            index += 3U;
            continue;
        }

        if (first >= 0xEEU && first <= 0xEFU) {
            if (index + 2U >= size || !isContinuationByte(bytes[index + 1U]) ||
                !isContinuationByte(bytes[index + 2U])) {
                return false;
            }
            index += 3U;
            continue;
        }

        if (first == 0xF0U) {
            if (index + 3U >= size || bytes[index + 1U] < 0x90U ||
                bytes[index + 1U] > 0xBFU ||
                !isContinuationByte(bytes[index + 2U]) ||
                !isContinuationByte(bytes[index + 3U])) {
                return false;
            }
            index += 4U;
            continue;
        }

        if (first >= 0xF1U && first <= 0xF3U) {
            if (index + 3U >= size || !isContinuationByte(bytes[index + 1U]) ||
                !isContinuationByte(bytes[index + 2U]) ||
                !isContinuationByte(bytes[index + 3U])) {
                return false;
            }
            index += 4U;
            continue;
        }

        if (first == 0xF4U) {
            if (index + 3U >= size || bytes[index + 1U] < 0x80U ||
                bytes[index + 1U] > 0x8FU ||
                !isContinuationByte(bytes[index + 2U]) ||
                !isContinuationByte(bytes[index + 3U])) {
                return false;
            }
            index += 4U;
            continue;
        }

        return false;
    }

    return true;
}

}  // namespace qs
