#include "WeShopStoreConsoleQS.h"

#include "WeShopServiceQS.h"

namespace qs {

void WeShopStoreConsoleQS::run(ConsoleContextQS& c) {
    while (!c.io.eof()) {
        std::lock_guard<std::recursive_mutex> guard(c.lock);
        auto& shop = dynamic_cast<WeShopServiceQS&>(c.svc());
        const std::string me = c.me();
        c.io.title("我的小店");
        for (const auto& p : shop.productsOf(me)) {
            c.io.print("  " + p.id + "  " + p.name + "  " + p.priceText() + "  库存 " + std::to_string(p.stock));
        }
        c.io.print("  —— 客户的小店 ——");
        for (const auto& p : shop.customerProducts(me)) {
            c.io.print("  " + p.id + "  " + p.name + "  " + p.priceText() + "  库存 " + std::to_string(p.stock) + "  （" +
                       shop.nicknameOf(p.owner) + "）");
        }
        c.io.print("  1. 上架商品  2. 下架商品  3. 下单购买  4. 我的订单  0. 返回");
        const int pick = c.io.readChoice("请选择：", 0, 4);
        if (pick == 0) return;
        if (pick == 1) {
            const std::string name = c.io.readLine("商品名称：");
            const std::string desc = c.io.readLine("描述：");
            const int price = c.io.readChoice("价格（分）：", 1, 10000000);
            const int stock = c.io.readChoice("库存：", 1, 100000);
            c.show(shop.listProduct(me, name, desc, price, stock));
        } else if (pick == 2) {
            c.show(shop.removeProduct(me, c.io.readLine("商品编号：")));
        } else if (pick == 3) {
            const std::string id = c.io.readLine("商品编号：");
            c.show(shop.buy(me, id, c.io.readChoice("数量：", 1, 999)));
        } else {
            for (const auto& o : shop.ordersOf(me)) {
                c.io.print("  " + o.date + "  " + (o.buyer == me ? "买入 " : "卖出 ") + o.productName + " ×" +
                           std::to_string(o.quantity));
            }
        }
    }
}

QS_REGISTER_CONSOLE_FEATURE(WeShopStoreConsoleQS, "weshop.store")

}  // namespace qs
