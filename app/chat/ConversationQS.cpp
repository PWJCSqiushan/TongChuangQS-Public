#include "ConversationQS.h"

namespace qs {
std::string ConversationQS::preview() const {
    if (messages.empty()) return "";
    const ChatMessageQS& m = messages.back();
    std::string text = m.text;
    for (char& c : text) {
        if (c == '\n') c = ' ';
    }
    if (m.isGroup() && !m.outgoing) return m.fromNick + "：" + text;
    return text;
}
}  // namespace qs
