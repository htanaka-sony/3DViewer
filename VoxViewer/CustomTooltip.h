#ifndef CUSTOMTOOLTIP_H
#define CUSTOMTOOLTIP_H

#include <QToolTip>

#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class CustomTooltipWidget : public QWidget {
    QTimer hideTimer;

public:
    CustomTooltipWidget(const QString& text, QWidget* parent = nullptr)
        : QWidget(nullptr, Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)    // ツールチップ風
    {
        setFocusPolicy(Qt::NoFocus);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);

        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_ShowWithoutActivating, true);
        setWindowFlag(Qt::WindowDoesNotAcceptFocus);
        auto* lbl = new QLabel(text, this);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet("background:#ffffe0;border:1px solid #888;padding:4px;font-size:13px;color:#222;");
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(lbl);
        setLayout(layout);
        adjustSize();
        hideTimer.setSingleShot(true);
        connect(&hideTimer, &QTimer::timeout, this, &CustomTooltipWidget::close);
    }

    void showAt(const QPoint& globalPos, int msec)
    {
        move(globalPos);
        show();
        hideTimer.start(msec);
    }
};

class CustomTooltipManager {
public:
    static void show(const QString& text, int msec = 1200, int padding = 0, bool useDpiScale = true)
    {
        close();
        if (msec <= 0) {
            return;
        }

        auto* tipWidget = new CustomTooltipWidget(text);
        tipWidget->adjustSize();
        int tipHeight = tipWidget->height();

        // DPIスケール適用したパディング
        int padY = dpi_scaled_offset(padding, useDpiScale);    // たとえば 6px

        // カーソルの"先端"から上方向にウィジェット高さ+paddingだけずらす
        QPoint cursorPos = QCursor::pos();
        QPoint tipPos    = cursorPos + QPoint(0, -(tipHeight + padY));
        lastTip          = tipWidget;
        lastTip->showAt(tipPos, msec);

        QObject::connect(lastTip, &CustomTooltipWidget::destroyed, []() { lastTip = nullptr; });
    }

    static void close()
    {
        if (lastTip) {
            lastTip->close();
            lastTip = nullptr;
        }
    }

private:
    static int dpi_scaled_offset(int base = 6, bool useDpiScale = true)
    {
        if (!useDpiScale) return base;
        auto* screen = QGuiApplication::primaryScreen();
        if (!screen) return base;
        qreal scale = screen->logicalDotsPerInch() / 96.0;
        return int(base * scale);
    }
    static inline CustomTooltipWidget* lastTip = nullptr;
};

#endif    // CUSTOMTOOLTIP_H
