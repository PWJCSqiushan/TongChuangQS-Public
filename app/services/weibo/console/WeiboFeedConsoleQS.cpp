#include "WeiboFeedConsoleQS.h"

#include "DateQS.h"
#include "WeiboServiceQS.h"

namespace qs {

void WeiboFeedConsoleQS::run(ConsoleContextQS& c) {
    while (!c.io.eof()) {
        std::lock_guard<std::recursive_mutex> guard(c.lock);
        auto& weibo = dynamic_cast<WeiboServiceQS&>(c.svc());
        const std::string me = c.me();
        c.io.title("微博首页 · 关注 " + std::to_string(weibo.follows().followingCount(me)) + " · 粉丝 " +
                   std::to_string(weibo.follows().followerCount(me)));
        const auto posts = weibo.timeline(me);
        for (std::size_t i = 0; i < posts.size() && i < 10; ++i) {
            const auto& p = posts[i];
            c.io.print("  " + std::to_string(i + 1) + ". " + weibo.nicknameOf(p.author) + "  " + DateQS::shortText(p.time));
            c.io.print("     " + p.text + "   ♥ " + std::to_string(p.likes.size()) + (p.likes.count(me) ? "（已赞）" : ""));
        }
        c.io.print("  —— 1. 发微博  2. 点赞/取消  3. 删除我的微博  0. 返回");
        const int pick = c.io.readChoice("请选择：", 0, 3);
        if (pick == 0) return;
        if (pick == 1) c.show(weibo.post(me, c.io.readLine("说点什么：")));
        if (pick == 2 || pick == 3) {
            if (posts.empty()) continue;
            const int n = c.io.readChoice("第几条：", 1, static_cast<int>(std::min<std::size_t>(posts.size(), 10)));
            const std::string id = posts[static_cast<std::size_t>(n - 1)].id;
            c.show(pick == 2 ? weibo.toggleLike(me, id) : weibo.deletePost(me, id));
        }
    }
}

QS_REGISTER_CONSOLE_FEATURE(WeiboFeedConsoleQS, "weibo.feed")

}  // namespace qs
