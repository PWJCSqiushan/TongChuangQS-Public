#pragma once

#include "FeaturePageQS.h"

#include <QVBoxLayout>

namespace qs {

/** 微商"小店"：我的商品（上架/下架）、客户的小店（下单）、订单记录。属于微商服务自己的界面代码。 */
class WeShopStorePageQS : public FeaturePageQS {
public:
    WeShopStorePageQS(ShellContextQS& context, QWidget* parent);
    void refresh() override;

private:
    void rebuild();
    QWidget* body_;
    QVBoxLayout* column_;
};

}  // namespace qs
