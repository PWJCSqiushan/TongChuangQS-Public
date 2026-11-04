#include "ChatMessageQS.h"

namespace qs {

std::string ChatMessageQS::conversationKey(const std::string& owner) const {
    if (isGroup()) return "g:" + groupId;
    return "p:" + (from == owner ? to : from);
}

std::string ChatMessageQS::statusText() const {
    if (!outgoing) return "";
    switch (status) {
        case MessageStatusQS::Pending:
            return isGroup() ? "发送中" : "对方离线，上线后自动送达";
        case MessageStatusQS::Sent:
            if (isGroup()) {
                return "已送达 " + std::to_string(ackedBy.size()) + "/" + std::to_string(recipients) +
                       (pendingTo.empty() ? "" : "，" + std::to_string(pendingTo.size()) + "人离线待补发");
            }
            return "已发送";
        case MessageStatusQS::Delivered:
            return isGroup() ? "全部送达" : "已送达";
        case MessageStatusQS::Received:
            return "";
    }
    return "";
}

namespace {
const char* statusKey(MessageStatusQS s) {
    switch (s) {
        case MessageStatusQS::Pending: return "pending";
        case MessageStatusQS::Sent: return "sent";
        case MessageStatusQS::Delivered: return "delivered";
        case MessageStatusQS::Received: return "received";
    }
    return "pending";
}
MessageStatusQS statusFrom(const std::string& s) {
    if (s == "sent") return MessageStatusQS::Sent;
    if (s == "delivered") return MessageStatusQS::Delivered;
    if (s == "received") return MessageStatusQS::Received;
    return MessageStatusQS::Pending;
}
}  // namespace

JsonQS ChatMessageQS::toJson() const {
    JsonQS j = toWire();
    j["outgoing"] = outgoing;
    j["status"] = statusKey(status);
    j["pendingTo"] = pendingTo;
    j["ackedBy"] = ackedBy;
    j["recipients"] = recipients;
    return j;
}

ChatMessageQS ChatMessageQS::fromJson(const JsonQS& j) {
    ChatMessageQS m = fromWire(j);
    m.outgoing = j.value("outgoing", false);
    m.status = statusFrom(j.value("status", std::string("pending")));
    m.pendingTo = j.value("pendingTo", std::vector<std::string>());
    m.ackedBy = j.value("ackedBy", std::vector<std::string>());
    m.recipients = j.value("recipients", 1);
    return m;
}

JsonQS ChatMessageQS::toWire() const {
    return JsonQS{{"id", id},       {"service", service},     {"from", from}, {"fromNick", fromNick},
                  {"to", to},       {"group", groupId},       {"groupName", groupName},
                  {"text", text},   {"time", time}};
}

ChatMessageQS ChatMessageQS::fromWire(const JsonQS& j) {
    ChatMessageQS m;
    m.id = j.value("id", std::string());
    m.service = j.value("service", std::string());
    m.from = j.value("from", std::string());
    m.fromNick = j.value("fromNick", std::string());
    m.to = j.value("to", std::string());
    m.groupId = j.value("group", std::string());
    m.groupName = j.value("groupName", std::string());
    m.text = j.value("text", std::string());
    m.time = j.value("time", static_cast<std::int64_t>(0));
    return m;
}

}  // namespace qs
