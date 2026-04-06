#ifndef CUSTOMTABWIDGET_H
#define CUSTOMTABWIDGET_H

#include <QPainter>
#include <QResizeEvent>
#include <QString>
#include <QTabBar>
#include <QTableWidget>
#include <QTimer>

class CustomTabBar : public QTabBar {
public:
    explicit CustomTabBar(QWidget* parent = nullptr) : QTabBar(parent) {}

protected:
    void paintEvent(QPaintEvent* event) override
    {
        // 標準の描画
        QTabBar::paintEvent(event);

        QPainter painter(this);

        for (int i = 0; i < count(); ++i) {
            QRect rect = tabRect(i);

            rect.adjust(1, 1, 0, 1);

            // 背景色
            if (currentIndex() == i) {
                painter.fillRect(rect, QColor("#d0eaff"));    // 選択タブ：薄い青
            }
            else {
                painter.fillRect(rect, QColor("#f0f0f0"));    // 非選択タブ
            }

            // 枠線
            painter.setPen(QPen(QColor("#A0A0A0"), 1));
            painter.drawLine(rect.topLeft(), rect.topRight());                  // 上
            if (i == 0) painter.drawLine(rect.topLeft(), rect.bottomLeft());    // 左
            painter.drawLine(rect.topRight(), rect.bottomRight());              // 右
            // 非選択タブだけ下線を描く
            if (currentIndex() != i) {
                painter.drawLine(rect.bottomLeft(), rect.bottomRight());    // 下
            }

            // 文字（中央揃え）
            painter.setPen(Qt::black);
            painter.drawText(rect, Qt::AlignCenter, tabText(i));
        }
    }
};

class CustomTabWidget : public QTabWidget {
    Q_OBJECT
public:
    explicit CustomTabWidget(QWidget* parent = nullptr) : QTabWidget(parent) {}

    void resizeEvent(QResizeEvent* event) override
    {
        /// TODO: 暫定（UIは置き換える予定なので、深追いせず動作だけ直す）
        /// 現状２番目のタブをアクティブにした状態で、タブ全体を非表示→再度表示で先頭タブが消える（表示扱いなのでQtのバグっぽい）
        /// カレント設定をしなおして無理やり表示させる
        /// 一応カレント設定は戻す（その場で戻すと直らないのでキューにためる）
        QSize size = event->size();
        if ((m_pre_size.width() == 0 && size.width() > 0) || (m_pre_size.height() == 0 && size.height() > 0)) {
            if (count() > 0) {
                int curret_index = currentIndex();
                if (curret_index > 0) {
                    setCurrentIndex(0);
                    QTimer::singleShot(0, this, [this, curret_index]() { setCurrentIndex(curret_index); });
                }
            }
        }
        QTabWidget::resizeEvent(event);
        m_pre_size = size;
    }

    void setCustomTabBar()
    {    // 既存のタブを一時的に保存
        QList<QWidget*> pages;
        QStringList     labels;
        for (int i = 0; i < count(); ++i) {
            pages << widget(i);
            labels << tabText(i);
        }

        // 全タブ削除
        while (count() > 0)
            removeTab(0);

        // カスタムTabBarをセット
        setTabBar(new CustomTabBar(this));
        m_custom_paint = true;

        // タブを再追加
        for (int i = 0; i < pages.size(); ++i) {
            addTab(pages[i], labels[i]);
        }
    }

    void paintEvent(QPaintEvent* event) override
    {
        if (m_custom_paint) {
            // 標準描画
            QTabWidget::paintEvent(event);

            QPainter painter(this);
            painter.setPen(QPen(QColor("#A0A0A0"), 1));

            QRect rect = this->rect();

            if (tabPosition() == QTabWidget::North) {
                rect.adjust(1, 1, 0, 0);

                int tabBarHeight = tabBar()->height();
                rect.setTop(tabBarHeight);

                // 左・右・下の枠線
                painter.drawLine(rect.topLeft(), rect.bottomLeft());        // 左
                painter.drawLine(rect.topRight(), rect.bottomRight());      // 右
                painter.drawLine(rect.bottomLeft(), rect.bottomRight());    // 下

                // 上枠線（タブバーの下端から選択タブの下端までを2回に分けて描画）
                QRect selRect  = tabBar()->tabRect(tabBar()->currentIndex());
                int   selLeft  = selRect.left();
                int   selRight = selRect.right();
                int   y        = tabBarHeight;

                // 左側
                painter.drawLine(rect.topLeft(), QPoint(selLeft, y));
                // 右側
                painter.drawLine(QPoint(selRight, y), rect.topRight());
            }
            else {
                // 他のタブ位置の場合は適宜調整
                painter.drawRect(rect.adjusted(0, 0, -1, -1));
            }
        }
        else {
            // 標準の描画
            QTabWidget::paintEvent(event);
        }
    }

protected:
    QSize m_pre_size;
    bool  m_custom_paint = false;
};
#endif    // CUSTOMTABWIDGET_H
