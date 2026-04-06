#include "RangeSlider.h"

#include <QApplication>
#include <QBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QSlider>
#include <QStylePainter>
#include <QWidget>

#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QSlider>
#include <QStyleOptionSlider>
#include <QStylePainter>
#include <QVBoxLayout>
#include <QWidget>

RangeSlider::RangeSlider(QWidget* parent) : QSlider(Qt::Horizontal, parent)
{
    setRange(m_lower_value, m_upper_value);
    setFocusPolicy(Qt::StrongFocus);
}

int RangeSlider::lowerValue() const
{
    return m_lower_value;
}

int RangeSlider::upperValue() const
{
    return m_upper_value;
}

float RangeSlider::minRealValue() const
{
    return m_range_min_value;
}

float RangeSlider::maxRealValue() const
{
    return m_range_max_value;
}

float RangeSlider::lowPosRate() const
{
    if (m_lower_spin_box) {
        return (m_lower_spin_box->value() - m_range_min_value) / (m_range_max_value - m_range_min_value);
    }
    else {
        return (lowerRealValue() - m_range_min_value) / (m_range_max_value - m_range_min_value);
    }
}

float RangeSlider::upperPosRate() const
{
    if (m_upper_spin_box) {
        return (m_upper_spin_box->value() - m_range_min_value) / (m_range_max_value - m_range_min_value);
    }
    else {
        return (upperRealValue() - m_range_min_value) / (m_range_max_value - m_range_min_value);
    }
}

void RangeSlider::setLowPosRate(float rate)
{
    if (m_lower_spin_box) {
        m_lower_spin_box->setValue((m_range_max_value - m_range_min_value) * rate + m_range_min_value);
    }
    else {
        int slider_value = static_cast<int>(
            floor(((m_range_max_value - m_range_min_value) * rate + m_range_min_value) / m_range_step + 0.001));
        setLowerValue(slider_value);
    }
}

void RangeSlider::setUpperPosRate(float rate)
{
    if (m_upper_spin_box) {
        m_upper_spin_box->setValue((m_range_max_value - m_range_min_value) * rate + m_range_min_value);
    }
    else {
        int slider_value = static_cast<int>(
            floor(((m_range_max_value - m_range_min_value) * rate + m_range_min_value) / m_range_step + 0.001));
        setUpperValue(slider_value);
    }
}

float RangeSlider::lowerRealValue() const
{
    if (m_lower_spin_box) {
        return m_lower_spin_box->value();
    }
    else {
        return (float)lowerValue() * m_range_step + m_range_min_value;
    }
}

float RangeSlider::upperRealValue() const
{
    if (m_upper_spin_box) {
        return m_upper_spin_box->value();
    }
    else {
        return (float)upperValue() * m_range_step + m_range_min_value;
    }
}

void RangeSlider::setLowerRealValue(float value)
{
    if (m_lower_spin_box) {
        m_lower_spin_box->setValue(value);
    }
    else {
        int slider_value = static_cast<int>(floor((value - m_range_min_value) / m_range_step + 0.001));
        setLowerValue(slider_value);
    }
}

void RangeSlider::setUpperRealValue(float value)
{
    if (m_upper_spin_box) {
        m_upper_spin_box->setValue(value);
    }
    else {
        int slider_value = static_cast<int>(floor((value - m_range_min_value) / m_range_step + 0.001));
        setUpperValue(slider_value);
    }
}

void RangeSlider::init(float min_value, float max_value, float step, int decimals, double low_upper_min_diff)
{
    m_enable_lower = false;
    m_enable_upper = false;

    m_range_min_value    = min_value;
    m_range_max_value    = max_value;
    m_range_step         = step;
    m_low_upper_min_diff = low_upper_min_diff;

    /// stepでmax調整
    int step_num = static_cast<int>(ceil((max_value - min_value) / step - 0.001));
    if (step_num == 0) {
        step_num = 1;
    }
    m_range_max_value = ((float)step_num * step) + m_range_min_value;

    m_range_min_value -= (step / 2.0f);
    m_range_max_value += (step / 2.0f);

    setRange(0, step_num + 1);
    setSingleStep(1);
    m_lower_value             = 0;
    m_upper_value             = step_num + 1;
    m_backup_lower_value      = m_lower_value;
    m_backup_upper_value      = m_upper_value;
    m_backup_lower_spin_value = m_range_min_value;
    m_backup_upper_spin_value = m_range_max_value;

    if (decimals < 0) {
        /// stepの小数点以下の桁数を計算
        decimals = 0;
        if (step != static_cast<int>(step)) {
            QString stepStr  = QString::number(step, 'f', 6);
            int     dotIndex = stepStr.indexOf('.');
            if (dotIndex != -1) {
                decimals = stepStr.length() - dotIndex - 1;
                while (decimals > 0 && stepStr.endsWith('0')) {
                    stepStr.chop(1);
                    decimals--;
                }
            }
        }
        /// 暫定
        if (decimals < 2) {
            decimals = 2;
        }
        else if (decimals < 4) {
            decimals = 4;
        }
    }

    if (m_lower_spin_box) {
        m_lower_spin_box->setDecimals(decimals);
        m_lower_spin_box->setRange(m_range_min_value, m_range_max_value);
        m_lower_spin_box->blockSignals(true);
        m_lower_spin_box->setValue(m_range_min_value);
        m_lower_spin_box->blockSignals(false);
        m_lower_spin_box->setSingleStep(m_range_step / 2.0f);    /// 両端は半分
    }
    if (m_upper_spin_box) {
        m_upper_spin_box->setDecimals(decimals);
        m_upper_spin_box->setRange(m_range_min_value, m_range_max_value);
        m_upper_spin_box->blockSignals(true);
        m_upper_spin_box->setValue(m_range_max_value);
        m_upper_spin_box->blockSignals(false);
        m_upper_spin_box->setSingleStep(m_range_step / 2.0f);    /// 両端は半分
    }
    if (m_lower_spin_box) {
        emit m_lower_spin_box->valueChanged(m_range_min_value);
    }
    if (m_upper_spin_box) {
        emit m_upper_spin_box->valueChanged(m_range_max_value);
    }
}

void RangeSlider::setEnableLower(bool enable)
{
    m_enable_lower = enable;
    if (m_lower_spin_box) {
        m_lower_spin_box->setEnabled(m_enable_lower);
    }
    if (m_enable_lower) {
        if (m_lower_spin_box) {
            m_lower_spin_box->setValue(m_backup_lower_spin_value);
        }
        else {
            setLowerValue(m_backup_lower_value);
        }
    }
    else {
        m_backup_lower_value      = m_lower_value;
        m_backup_lower_spin_value = m_lower_spin_box ? m_lower_spin_box->value() : 0;
        setLowerValue(minimum());
        if (m_active_handle == LowerHandle) {
            m_active_handle = None;
        }
    }
    update();
}

void RangeSlider::setEnableUpper(bool enable)
{
    m_enable_upper = enable;
    if (m_upper_spin_box) {
        m_upper_spin_box->setEnabled(m_enable_upper);
    }
    if (m_enable_upper) {
        if (m_upper_spin_box) {
            m_upper_spin_box->setValue(m_backup_upper_spin_value);
        }
        else {
            setUpperValue(m_backup_upper_value);
        }
    }
    else {
        m_backup_upper_value      = m_upper_value;
        m_backup_upper_spin_value = m_upper_spin_box ? m_upper_spin_box->value() : 0;
        setUpperValue(maximum());
        if (m_active_handle == UpperHandle) {
            m_active_handle = None;
        }
    }
    update();
}

void RangeSlider::setSpinBox(QDoubleSpinBox* lower_spin, QDoubleSpinBox* upper_spin)
{
    m_lower_spin_box = lower_spin;
    m_upper_spin_box = upper_spin;

    if (m_lower_spin_box) {
        auto updateLowerValue = [this]() {
            auto value        = m_lower_spin_box->value();
            int  slider_value = static_cast<int>(floor((value - m_range_min_value) / m_range_step + 0.001));

            if (value < m_range_min_value + m_range_step / 2.0f) {
                m_lower_spin_box->blockSignals(true);
                m_lower_spin_box->setValue(m_range_min_value + m_range_step / 2.0f);
                m_lower_spin_box->blockSignals(false);

                m_lower_spin_box->setSingleStep(m_range_step / 2.0f);    /// 両端は半分

                value = m_lower_spin_box->value();
            }
            else if (value > m_range_max_value - m_range_step / 2.0f) {
                m_lower_spin_box->blockSignals(true);
                m_lower_spin_box->setValue(m_range_max_value - m_range_step / 2.0f);
                m_lower_spin_box->blockSignals(false);

                m_lower_spin_box->setSingleStep(m_range_step / 2.0f);    /// 両端は半分

                value = m_lower_spin_box->value();
            }
            else {
                m_lower_spin_box->setSingleStep(m_range_step);
            }

            if (slider_value <= m_upper_value) {
                if (m_upper_spin_box) {
                    double check_upper = m_upper_spin_box->value() - m_low_upper_min_diff;
                    check_upper        = std::clamp(check_upper, (double)(m_range_min_value + m_range_step / 2.0f),
                                                    (double)(m_range_max_value - m_range_step / 2.0f));

                    if (value >= check_upper) {
                        value = check_upper;

                        m_lower_spin_box->blockSignals(true);
                        m_lower_spin_box->setValue(value);
                        m_lower_spin_box->blockSignals(false);

                        // slider_value = static_cast<int>(floor((value - m_range_min_value) / m_range_step + 0.001));
                    }
                }

                if (slider_value != m_lower_value) {
                    m_lower_value = slider_value;    /// 変数のみ
                    emit lowerValueChanged(m_lower_value);
                }
                else {
                    /// スライダー位置変わらないが値変更時
                    if (m_lower_back_value >= 0 && m_lower_back_value != value) {
                        emit lowerValueChanged(m_lower_value);
                    }
                }
            }
            else {
                setLowerValue(m_upper_value);
            }
            if (m_enable_lower) {
                m_active_handle = LowerHandle;
            }
            update();

            m_lower_back_value = m_lower_spin_box->value();
        };

        m_lower_spin_box->setKeyboardTracking(false);
        connect(m_lower_spin_box, &QDoubleSpinBox::editingFinished, this, updateLowerValue);
        connect(m_lower_spin_box, &QDoubleSpinBox::valueChanged, this, updateLowerValue);
    }

    if (m_upper_spin_box) {
        auto updateUpperValue = [this]() {
            auto value        = m_upper_spin_box->value();
            int  slider_value = static_cast<int>(floor((value - m_range_min_value) / m_range_step + 0.001));

            if (value < m_range_min_value + m_range_step / 2.0f) {
                m_upper_spin_box->blockSignals(true);
                m_upper_spin_box->setValue(m_range_min_value + m_range_step / 2.0f);
                m_upper_spin_box->blockSignals(false);

                m_upper_spin_box->setSingleStep(m_range_step / 2.0f);    /// 両端は半分

                value = m_upper_spin_box->value();
            }
            else if (value > m_range_max_value - m_range_step / 2.0f) {
                m_upper_spin_box->blockSignals(true);
                m_upper_spin_box->setValue(m_range_max_value - m_range_step / 2.0f);
                m_upper_spin_box->blockSignals(false);

                m_upper_spin_box->setSingleStep(m_range_step / 2.0f);    /// 両端は半分

                value = m_upper_spin_box->value();
            }
            else {
                m_upper_spin_box->setSingleStep(m_range_step);
            }

            if (slider_value >= m_lower_value) {
                if (m_lower_spin_box) {
                    double check_lower = m_lower_spin_box->value() + m_low_upper_min_diff;
                    check_lower        = std::clamp(check_lower, (double)(m_range_min_value + m_range_step / 2.0f),
                                                    (double)(m_range_max_value - m_range_step / 2.0f));

                    if (value <= check_lower) {
                        value = check_lower;

                        m_upper_spin_box->blockSignals(true);
                        m_upper_spin_box->setValue(value);
                        m_upper_spin_box->blockSignals(false);

                        // slider_value = static_cast<int>(floor((value - m_range_min_value) / m_range_step + 0.001));
                    }
                }

                if (slider_value != m_upper_value) {
                    m_upper_value = slider_value;    /// 変数のみ
                    emit upperValueChanged(m_upper_value);
                }
                else {
                    /// スライダー位置変わらないが値変更時
                    if (m_upper_back_value >= 0 && m_upper_back_value != value) {
                        emit upperValueChanged(m_upper_value);
                    }
                }
            }
            else {
                setUpperValue(m_lower_value);
            }
            if (m_enable_upper) {
                m_active_handle = UpperHandle;
            }
            update();

            m_upper_back_value = m_upper_spin_box->value();
        };

        m_upper_spin_box->setKeyboardTracking(false);
        connect(m_upper_spin_box, &QDoubleSpinBox::editingFinished, this, updateUpperValue);
        connect(m_upper_spin_box, &QDoubleSpinBox::valueChanged, this, updateUpperValue);
    }
}

void RangeSlider::setRealRange(float lower, float upper, float step) {}

void RangeSlider::setIntRange(int lower, int upper, int step) {}

void RangeSlider::setLowerValue(int value)
{
    if (value > m_upper_value) {
        return;
    }
    if (value < 0) {
        value = 0;
    }

    bool change = false;
    if (value != m_lower_value) {
        m_lower_value = value;
        change        = true;
    }

    if (m_lower_spin_box) {
        auto new_value = (double)value * m_range_step + m_range_min_value;
        if (new_value != m_lower_spin_box->value()) {
            m_lower_spin_box->setValue(new_value);
        }
    }
    if (change) {
        update();
        emit lowerValueChanged(m_lower_value);
    }
}

void RangeSlider::setUpperValue(int value)
{
    if (value < m_lower_value) {
        return;
    }
    if (value > maximum()) {
        value = maximum();
    }

    bool change = false;
    if (value != m_upper_value) {
        m_upper_value = value;
        change        = true;
    }

    if (m_upper_spin_box) {
        auto new_value = (double)value * m_range_step + m_range_min_value;
        if (new_value != m_upper_spin_box->value()) {
            m_upper_spin_box->setValue(new_value);
        }
    }

    if (change) {
        update();
        emit upperValueChanged(m_upper_value);
    }
}

bool RangeSlider::isActiveLower() const
{
    return m_active_handle == LowerHandle;
}

bool RangeSlider::isActiveUpper() const
{
    return m_active_handle == UpperHandle;
}

void RangeSlider::paintEvent(QPaintEvent* event)
{
    QStylePainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    /// 背景バーを描画（適当）
    QColor color_background_bar(160, 160, 160);

    int   lower_min = QStyle::sliderPositionFromValue(minimum(), maximum(), minimum(), width() - 2 * m_handle_size);
    int   upper_max = QStyle::sliderPositionFromValue(minimum(), maximum(), maximum(), width() - 2 * m_handle_size);
    QRect background_bar_rect(lower_min + m_handle_size, height() / 2 - 1, upper_max - lower_min, 2);
    painter.setBrush(color_background_bar);
    painter.setPen(Qt::NoPen);
    painter.drawRect(background_bar_rect);

    if (m_enable_lower || m_enable_upper) {
        /// スライダーのトラックの色を取得(適当）
        QColor color_slider_bar(51, 153, 204);

        /// 範囲を描画（2つのハンドル間を塗りつぶす）
        int lower_pos =
            QStyle::sliderPositionFromValue(minimum(), maximum(), m_lower_value, width() - 2 * m_handle_size);
        int upper_pos =
            QStyle::sliderPositionFromValue(minimum(), maximum(), m_upper_value, width() - 2 * m_handle_size);
        QRect rangeRect(lower_pos + m_handle_size, height() / 2 - 2, upper_pos - lower_pos, 4);
        painter.setBrush(color_slider_bar.lighter(120));    /// 色を少し薄くする
        painter.setPen(Qt::NoPen);
        painter.drawRect(rangeRect);

        /// ハンドルを描画
        /// アクティブが裏に隠れるので前面にする（アクティブを最後に描画する）
        bool lower_bar_front = m_active_handle == LowerHandle;

        for (int ic = 0; ic < 2; ++ic) {
            int index = lower_bar_front ? 1 - ic : ic;
            switch (index) {
                case 0:
                    if (m_enable_lower) {
                        /// 下側のハンドルを描画
                        painter.setBrush(m_active_handle == LowerHandle ? Qt::blue : color_slider_bar.lighter(150));
                        painter.drawRect(QRect(lower_pos, 0, 2 * m_handle_size, height()));
                    }
                    break;
                case 1:
                    if (m_enable_upper) {
                        /// 上側のハンドルを描画
                        painter.setBrush(m_active_handle == UpperHandle ? Qt::blue : color_slider_bar.lighter(150));
                        painter.drawRect(QRect(upper_pos, 0, 2 * m_handle_size, height()));
                    }
                    break;
            }
        }
    }
}

void RangeSlider::mousePressEvent(QMouseEvent* event)
{
    int pos       = event->pos().x();
    int lower_pos = QStyle::sliderPositionFromValue(minimum(), maximum(), m_lower_value, width() - 2 * m_handle_size)
                  + m_handle_size;
    int upper_pos = QStyle::sliderPositionFromValue(minimum(), maximum(), m_upper_value, width() - 2 * m_handle_size)
                  + m_handle_size;

    /// 重なっているとき
    if (m_enable_lower && m_enable_upper && qAbs(pos - lower_pos) < m_handle_size
        && qAbs(pos - upper_pos) < m_handle_size) {
        /// 端で動かなせなくなるので対策
        if (m_lower_value <= minimum() + (maximum() - minimum()) / 30) {
            m_dragging_upper = true;
            m_active_handle  = UpperHandle;
        }
        else if (m_upper_value >= maximum() - (maximum() - minimum()) / 30) {
            m_dragging_lower = true;
            m_active_handle  = LowerHandle;
        }
        /// 端でなければ元々アクティブの方
        else if (m_active_handle == LowerHandle) {
            m_dragging_lower = true;
        }
        else if (m_active_handle == UpperHandle) {
            m_dragging_upper = true;
        }
        /// アクティブもなければLower
        else {
            m_dragging_lower = true;
            m_active_handle  = LowerHandle;
        }
    }
    else if (m_enable_lower && qAbs(pos - lower_pos) < m_handle_size) {
        m_dragging_lower = true;
        m_active_handle  = LowerHandle;
    }
    else if (m_enable_upper && qAbs(pos - upper_pos) < m_handle_size) {
        m_dragging_upper = true;
        m_active_handle  = UpperHandle;
    }
    else {
        m_active_handle = None;
    }
    update();
}

void RangeSlider::mouseMoveEvent(QMouseEvent* event)
{
    int pos   = event->pos().x();
    int value = QStyle::sliderValueFromPosition(minimum(), maximum(), pos - m_handle_size, width() - 2 * m_handle_size);

    if (m_dragging_lower) {
        setLowerValue(value);
    }
    else if (m_dragging_upper) {
        setUpperValue(value);
    }
}

void RangeSlider::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    m_dragging_lower = false;
    m_dragging_upper = false;
}

void RangeSlider::keyPressEvent(QKeyEvent* event)
{
    if (m_active_handle == None) {
        QSlider::keyPressEvent(event);
        return;
    }

    int step = singleStep();
    if (event->key() == Qt::Key_Left) {
        if (m_active_handle == LowerHandle) {
            setLowerValue(m_lower_value - step);
        }
        else if (m_active_handle == UpperHandle) {
            setUpperValue(m_upper_value - step);
        }
    }
    else if (event->key() == Qt::Key_Right) {
        if (m_active_handle == LowerHandle) {
            setLowerValue(m_lower_value + step);
        }
        else if (m_active_handle == UpperHandle) {
            setUpperValue(m_upper_value + step);
        }
    }
}
