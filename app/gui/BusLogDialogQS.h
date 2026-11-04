#pragma once

#include <QDialog>

class QListWidget;

namespace qs {

class PlatformQS;

/** 平台日志：列出服务总线上的每一次服务间请求（谁 → 谁、什么请求、结果）。答辩时用来说明"服务之间只通过接口协作"。 */
class BusLogDialogQS : public QDialog {
public:
    BusLogDialogQS(PlatformQS& platform, QWidget* parent);

private:
    void reload();
    PlatformQS& platform_;
    QListWidget* list_;
};

}  // namespace qs
