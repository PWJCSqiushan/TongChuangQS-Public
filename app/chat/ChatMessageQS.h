#pragma once

#include "JsonQS.h"

#include <cstdint>
#include <string>
#include <vector>

namespace qs {

/** 消息状态：待发送（对方不在线，上线后自动补发）→ 已发送 → 已送达（收到对方回执）。 */
enum class MessageStatusQS { Pending, Sent, Delivered, Received };

/**
 * 一条聊天消息（私聊或群聊）。
 * 保存在本人的聊天记录中；群消息额外记录还没送到的成员（pendingTo）和已回执的成员数。
 */
class ChatMessageQS {
public:
    std::string id;
    std::string service;
    std::string from;
    std::string fromNick;
    std::string to;          // 私聊：对方账号；群聊：接收者账号（收到时）或空（自己发出时）
    std::string groupId;     // 非空表示群消息
    std::string groupName;
    std::string text;
    std::int64_t time = 0;
    bool outgoing = false;
    MessageStatusQS status = MessageStatusQS::Pending;
    std::vector<std::string> pendingTo;  // 尚未发出的接收者（离线）
    std::vector<std::string> ackedBy;    // 已回执的接收者
    int recipients = 1;

    bool isGroup() const noexcept { return !groupId.empty(); }
    /** 所属会话：私聊为 "p:对方账号"，群聊为 "g:群号"。 */
    std::string conversationKey(const std::string& owner) const;
    std::string statusText() const;

    JsonQS toJson() const;
    static ChatMessageQS fromJson(const JsonQS& j);
    /** 线路上传输的格式（不含本地状态字段）。 */
    JsonQS toWire() const;
    static ChatMessageQS fromWire(const JsonQS& j);
};

}  // namespace qs
