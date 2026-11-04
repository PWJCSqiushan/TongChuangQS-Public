#include "WeShopStorePageQS.h"
#include "LayoutUtilQS.h"

#include "AvatarQS.h"
#include "FormDialogQS.h"
#include "IconQS.h"
#include "ThemeQS.h"
#include "WeShopServiceQS.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>

namespace qs {

namespace {
QString q(const std::string& s) { return QString::fromStdString(s); }
QLabel* label(const QString& t, const char* name, QWidget* p) {
    auto* l = new QLabel(t, p);
    l->setObjectName(name);
    l->setWordWrap(true);
    return l;
}
}  // namespace

WeShopStorePageQS::WeShopStorePageQS(ShellContextQS& context, QWidget* parent) : FeaturePageQS(context, parent) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setObjectName("ContentPanel");
    body_ = new QWidget(scroll);
    body_->setObjectName("ContentPanel");
    body_->setAttribute(Qt::WA_StyledBackground);
    auto* center = new QHBoxLayout(body_);
    center->setContentsMargins(0, 24, 0, 24);
    column_ = new QVBoxLayout();
    column_->setSpacing(14);
    center->addStretch(1);
    center->addLayout(column_);
    center->addStretch(1);
    scroll->setWidget(body_);
    outer->addWidget(scroll);
    rebuild();
}

void WeShopStorePageQS::refresh() { rebuild(); }

void WeShopStorePageQS::rebuild() {
    LayoutUtilQS::clear(column_);
    auto& shop = dynamic_cast<WeShopServiceQS&>(context_.svc());
    const std::string me = context_.me();
    const int width = 720;
    const auto productCard = [&](const WeShopProductQS& p, bool mine) {
        auto* card = new QWidget(body_);
        card->setObjectName("Card");
        card->setAttribute(Qt::WA_StyledBackground);
        card->setFixedSize(226, 168);
        auto* l = new QVBoxLayout(card);
        l->setContentsMargins(16, 14, 16, 12);
        l->setSpacing(4);
        l->addWidget(label(q(p.name), "RowTitle", card));
        l->addWidget(label(q(p.description), "SubTitle", card));
        l->addStretch(1);
        auto* price = label(q(p.priceText()), "", card);
        price->setStyleSheet(QString("color:%1;font-size:18px;font-weight:700;").arg(context_.accent.name()));
        l->addWidget(price);
        auto* foot = new QHBoxLayout();
        foot->addWidget(label(mine ? QString::fromUtf8("库存 %1").arg(p.stock)
                                   : QString::fromUtf8("%1 · 库存 %2").arg(q(shop.nicknameOf(p.owner))).arg(p.stock),
                              "SubTitle", card),
                        1);
        auto* action = new QPushButton(mine ? QString::fromUtf8("下架") : QString::fromUtf8("下单"), card);
        action->setObjectName(mine ? "LinkButton" : "SoftButton");
        action->setEnabled(mine || p.stock > 0);
        foot->addWidget(action);
        l->addLayout(foot);
        const std::string id = p.id;
        const QString name = q(p.name);
        connect(action, &QPushButton::clicked, this, [this, id, mine, name] {
            auto& s = dynamic_cast<WeShopServiceQS&>(context_.svc());
            if (mine) {
                if (FormDialogQS::confirm(this, QString::fromUtf8("下架商品"), QString::fromUtf8("确定下架「%1」？").arg(name)))
                    context_.show(s.removeProduct(context_.me(), id));
            } else {
                QMap<QString, QString> out;
                if (!FormDialogQS::ask(this, QString::fromUtf8("下单"), name, {{"n", QString::fromUtf8("数量"), "", "1"}}, out,
                                       QString::fromUtf8("确认下单")))
                    return;
                context_.show(s.buy(context_.me(), id, out["n"].toInt()));
            }
            context_.refreshAll();
        });
        return card;
    };
    const auto section = [&](const QString& title, QPushButton* extra) {
        auto* row = new QHBoxLayout();
        auto* t = label(title, "PanelTitle", body_);
        row->addWidget(t, 1);
        if (extra) row->addWidget(extra);
        auto* holder = new QWidget(body_);
        holder->setFixedWidth(width);
        holder->setLayout(row);
        column_->addWidget(holder);
    };

    auto* add = new QPushButton(IconQS::icon("plus", Qt::white, 16), QString::fromUtf8("上架商品"), body_);
    add->setObjectName("PrimaryButton");
    connect(add, &QPushButton::clicked, this, [this] {
        QMap<QString, QString> out;
        if (!FormDialogQS::ask(this, QString::fromUtf8("上架商品"), QString::fromUtf8("商品会出现在你所有客户的「客户的小店」中"),
                               {{"name", QString::fromUtf8("商品名称"), ""},
                                {"desc", QString::fromUtf8("描述"), QString::fromUtf8("可留空")},
                                {"price", QString::fromUtf8("价格（元）"), "9.90"},
                                {"stock", QString::fromUtf8("库存"), "10"}},
                               out, QString::fromUtf8("上架")))
            return;
        const int cents = static_cast<int>(out["price"].toDouble() * 100 + 0.5);
        context_.show(dynamic_cast<WeShopServiceQS&>(context_.svc())
                          .listProduct(context_.me(), out["name"].toStdString(), out["desc"].toStdString(), cents, out["stock"].toInt()));
        context_.refreshAll();
    });
    section(QString::fromUtf8("我的小店"), add);
    const auto addGrid = [&](const std::vector<WeShopProductQS>& list, bool mine, const QString& empty) {
        auto* holder = new QWidget(body_);
        holder->setFixedWidth(width);
        auto* grid = new QGridLayout(holder);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setSpacing(12);
        int i = 0;
        for (const auto& p : list) {
            grid->addWidget(productCard(p, mine), i / 3, i % 3);
            ++i;
        }
        if (list.empty()) grid->addWidget(label(empty, "SubTitle", holder), 0, 0);
        column_->addWidget(holder);
    };
    addGrid(shop.productsOf(me), true, QString::fromUtf8("还没有上架商品"));
    section(QString::fromUtf8("客户的小店"), nullptr);
    addGrid(shop.customerProducts(me), false, QString::fromUtf8("客户还没有上架商品"));
    section(QString::fromUtf8("订单"), nullptr);
    auto* orders = new QWidget(body_);
    orders->setObjectName("Card");
    orders->setAttribute(Qt::WA_StyledBackground);
    orders->setFixedWidth(width);
    auto* ol = new QVBoxLayout(orders);
    ol->setContentsMargins(20, 14, 20, 14);
    const auto list = shop.ordersOf(me);
    if (list.empty()) ol->addWidget(label(QString::fromUtf8("暂无订单"), "SubTitle", orders));
    for (const auto& o : list) {
        const bool buy = o.buyer == me;
        ol->addWidget(label(QString::fromUtf8("%1　%2　%3 ×%4　¥%5.%6　%7")
                                .arg(q(o.date), buy ? QString::fromUtf8("买入") : QString::fromUtf8("卖出"), q(o.productName))
                                .arg(o.quantity)
                                .arg(o.totalCents / 100)
                                .arg(o.totalCents % 100, 2, 10, QChar('0'))
                                .arg(q(buy ? "卖家 " + shop.nicknameOf(o.seller) : "买家 " + shop.nicknameOf(o.buyer))),
                            "", orders));
    }
    column_->addWidget(orders);
    column_->addStretch(1);
}

QS_REGISTER_GUI_FEATURE(WeShopStorePageQS, "weshop.store")

}  // namespace qs
