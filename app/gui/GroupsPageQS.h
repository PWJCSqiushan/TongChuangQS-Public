#pragma once

#include "FeaturePageQS.h"

#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTabBar>
#include <QVBoxLayout>

namespace qs {

/**
 * "群"页面：只依赖 GroupCapabilityQS，界面上的可用按钮全部由群当前的管理模式决定
 * （例如微信模式下没有"申请加入""设为管理员""创建讨论组"）。
 * 右侧详情展示"群特色"说明，并提供"切换管理模式"——成员数据保持不变。
 */
class GroupsPageQS : public FeaturePageQS {
public:
    GroupsPageQS(ShellContextQS& context, QWidget* parent);

    void refresh() override;
    int badgeCount() const override;
    void activate(const std::string& groupId) override;

private:
    void rebuildList();
    void showGroup(const std::string& groupId);
    void showEmpty();
    void updateSelection();
    void createGroup();
    void clearDetail();

    QTabBar* tabs_;
    QLineEdit* search_;
    QListWidget* list_;
    QWidget* detail_;
    QVBoxLayout* detailLayout_;
    std::string selected_;
};

}  // namespace qs
