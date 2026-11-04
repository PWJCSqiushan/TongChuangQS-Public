#include "ThemeQS.h"

#include <QApplication>
#include <QFont>
#include <QPalette>
#include <QStyleFactory>

namespace qs {

QColor ThemeQS::mix(const QColor& a, const QColor& b, double t) {
    return QColor::fromRgbF(static_cast<float>(a.redF() * (1 - t) + b.redF() * t),
                            static_cast<float>(a.greenF() * (1 - t) + b.greenF() * t),
                            static_cast<float>(a.blueF() * (1 - t) + b.blueF() * t));
}

void ThemeQS::applyBase(QApplication& app) {
    app.setStyle(QStyleFactory::create("Fusion"));
    QFont font("Microsoft YaHei UI");
    font.setPointSizeF(10);
    font.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(font);
    QPalette p = app.palette();
    p.setColor(QPalette::Window, QColor("#F5F6F8"));
    p.setColor(QPalette::Base, Qt::white);
    p.setColor(QPalette::Text, text());
    p.setColor(QPalette::WindowText, text());
    p.setColor(QPalette::ButtonText, text());
    p.setColor(QPalette::PlaceholderText, QColor("#A8ADB5"));
    p.setColor(QPalette::Highlight, QColor("#1677FF"));
    p.setColor(QPalette::HighlightedText, Qt::white);
    app.setPalette(p);
}

QString ThemeQS::styleSheet(const QColor& accent) {
    QString qss = R"QSS(
QWidget { color: #1F2329; }
QToolTip { background: #1F2329; color: white; border: none; padding: 4px 8px; border-radius: 6px; }

/* —— 三栏结构 —— */
#NavRail { background: #EEF0F3; border-right: 1px solid #E3E5E8; }
#ListPanel { background: #FFFFFF; border-right: 1px solid #ECEEF0; }
#ContentPanel { background: #F3F4F6; }
#PanelHeader { background: transparent; }
#PanelTitle { font-size: 17px; font-weight: 600; }
#SubTitle { color: #8A8F99; font-size: 12px; }
#SectionLabel { color: #8A8F99; font-size: 12px; font-weight: 600; padding: 10px 4px 4px 4px; }
#Divider { background: #ECEEF0; max-height: 1px; min-height: 1px; }

/* —— 输入框 —— */
QLineEdit, QPlainTextEdit, QTextEdit, QComboBox, QSpinBox {
  background: #F3F4F6; border: 1px solid transparent; border-radius: 10px; padding: 8px 12px;
  selection-background-color: @ACCENT@; }
QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus, QComboBox:focus, QSpinBox:focus {
  background: #FFFFFF; border: 1px solid @ACCENT@; }
QLineEdit#SearchBox { padding-left: 34px; border-radius: 12px; }
QComboBox::drop-down { border: none; width: 24px; }
QComboBox QAbstractItemView { background: white; border: 1px solid #E6E8EB; border-radius: 8px;
  selection-background-color: @SOFT@; selection-color: #1F2329; padding: 4px; }

/* —— 按钮 —— */
QPushButton { background: #FFFFFF; border: 1px solid #E1E4E8; border-radius: 10px; padding: 8px 16px; font-weight: 500; }
QPushButton:hover { background: #F5F6F8; }
QPushButton:pressed { background: #ECEEF1; }
QPushButton:disabled { color: #B8BCC3; background: #F7F8FA; border-color: #EEF0F2; }
QPushButton#PrimaryButton { background: @ACCENT@; color: white; border: none; }
QPushButton#PrimaryButton:hover { background: @ACCENT_HOVER@; }
QPushButton#PrimaryButton:pressed { background: @DARK@; }
QPushButton#PrimaryButton:disabled { background: @SOFT@; color: white; }
QPushButton#SoftButton { background: @SOFTER@; color: @DARK@; border: none; }
QPushButton#SoftButton:hover { background: @SOFT@; }
QPushButton#DangerButton { background: #FFFFFF; color: #E5484D; border: 1px solid #F5C9CB; }
QPushButton#DangerButton:hover { background: #FFF1F1; }
QPushButton#IconButton { background: transparent; border: none; border-radius: 10px; padding: 6px; }
QPushButton#IconButton:hover { background: #ECEEF1; }
QPushButton#LinkButton { background: transparent; border: none; color: @ACCENT@; padding: 4px 6px; }
QPushButton#LinkButton:hover { text-decoration: underline; }
QPushButton#ChipButton { background: #F3F4F6; border: 1px solid transparent; border-radius: 14px; padding: 5px 12px; color: #4A505A; }
QPushButton#ChipButton:hover { border-color: @ACCENT@; color: @ACCENT@; background: white; }
QPushButton#ChipButton:checked { background: @ACCENT@; color: white; }

/* —— 卡片 —— */
#Card { background: #FFFFFF; border: 1px solid #ECEEF0; border-radius: 14px; }
#AccentCard { background: @SOFTER@; border: 1px solid @SOFT@; border-radius: 14px; }

/* —— 列表 —— */
QListWidget { background: transparent; border: none; outline: none; }
QListWidget::item { border: none; border-radius: 12px; margin: 1px 6px; }
QListWidget::item:hover { background: #F5F6F8; }
QListWidget::item:selected { background: @ACCENT@; }
QListWidget#PlainList::item:selected { background: @SOFT@; }
#ListRow QLabel { background: transparent; }
#ListRow #RowTitle { font-size: 14px; font-weight: 500; }
#ListRow #RowSubtitle, #ListRow #RowTime { color: #8A8F99; font-size: 12px; }
#ListRow[selected="true"] #RowTitle, #ListRow[selected="true"] #RowSubtitle, #ListRow[selected="true"] #RowTime { color: #FFFFFF; }
QListWidget#PlainList #ListRow[selected="true"] #RowTitle { color: #1F2329; }
QListWidget#PlainList #ListRow[selected="true"] #RowSubtitle, QListWidget#PlainList #ListRow[selected="true"] #RowTime { color: #8A8F99; }
#Tag { background: @SOFTER@; color: @DARK@; border-radius: 6px; padding: 1px 6px; font-size: 11px; font-weight: 600; }
#ListRow[selected="true"] #Tag { background: rgba(255,255,255,0.92); color: @DARK@; }
#GrayTag { background: #F0F1F3; color: #6B7280; border-radius: 6px; padding: 1px 6px; font-size: 11px; }
#Badge { background: #F04438; color: white; border-radius: 9px; padding: 0px 5px; font-size: 11px; font-weight: 700; }

/* —— 选项卡 —— */
QTabWidget::pane { border: none; }
QTabBar::tab { background: transparent; border: none; padding: 8px 14px; color: #8A8F99; font-weight: 600; }
QTabBar::tab:selected { color: @ACCENT@; border-bottom: 2px solid @ACCENT@; }
QTabBar::tab:hover { color: #1F2329; }

/* —— 聊天 —— */
#ChatHeader { background: #FFFFFF; border-bottom: 1px solid #ECEEF0; }
#ChatScroll, #ChatCanvas { background: #F3F4F6; border: none; }
#BubbleIn { background: #FFFFFF; border-radius: 12px; border: 1px solid #EBEDF0; }
#BubbleOut { background: @BUBBLE@; border-radius: 12px; border: 1px solid @SOFT@; }
#BubbleText { background: transparent; }
#BubbleMeta { color: #9AA0A9; font-size: 11px; background: transparent; }
#SenderName { color: @DARK@; font-size: 12px; font-weight: 600; }
#DateChip { background: #C3C8D0; color: #FFFFFF; border-radius: 10px; padding: 2px 10px; font-size: 12px; }
#SystemNote { color: #8A8F99; font-size: 12px; }
#Composer { background: #FFFFFF; border-top: 1px solid #ECEEF0; }
QPlainTextEdit#ComposerEdit { background: #FFFFFF; border: 2px solid #E6E8EB; border-radius: 14px; padding: 8px 12px; font-size: 14px; }
QPlainTextEdit#ComposerEdit:focus { border: 2px solid @ACCENT@; }
#Hint { color: #A0A5AD; font-size: 11px; }
#EmptyTitle { color: #8A8F99; font-size: 15px; }

/* —— 导航栏 —— */
QToolButton#NavButton { background: transparent; border: none; border-radius: 12px; padding: 4px 2px; color: #5F6670; font-size: 12px; }
QToolButton#NavButton:hover { background: #E4E7EB; }
QToolButton#NavButton:checked { background: #FFFFFF; color: @ACCENT@; font-weight: 600; }

/* —— 滚动条 —— */
QScrollArea { border: none; background: transparent; }
QScrollBar:vertical { background: transparent; width: 8px; margin: 2px; }
QScrollBar::handle:vertical { background: #D3D6DB; border-radius: 4px; min-height: 30px; }
QScrollBar::handle:vertical:hover { background: #B9BDC4; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
QScrollBar:horizontal { height: 0; }

QCheckBox { spacing: 8px; }
QCheckBox::indicator { width: 18px; height: 18px; border-radius: 5px; border: 1px solid #C9CDD3; background: white; }
QCheckBox::indicator:checked { background: @ACCENT@; border-color: @ACCENT@; }
QMenu { background: white; border: 1px solid #E6E8EB; border-radius: 10px; padding: 6px; }
QMenu::item { padding: 7px 18px; border-radius: 6px; }
QMenu::item:selected { background: @SOFTER@; color: @DARK@; }
QMessageBox { background: white; }
QDialog { background: #FFFFFF; }
#Toast { background: rgba(31,35,41,0.92); color: white; border-radius: 12px; padding: 10px 18px; font-size: 13px; }
)QSS";
    qss.replace("@ACCENT_HOVER@", mix(accent, QColor("#FFFFFF"), 0.12).name());
    qss.replace("@ACCENT@", accent.name());
    qss.replace("@SOFTER@", softer(accent).name());
    qss.replace("@SOFT@", soft(accent).name());
    qss.replace("@DARK@", dark(accent).name());
    qss.replace("@BUBBLE@", mix(accent, QColor("#FFFFFF"), 0.88).name());
    return qss;
}

}  // namespace qs
