#ifndef RANGESLIDER_H
#define RANGESLIDER_H

#include <QDoubleSpinBox>
#include <QMouseEvent>
#include <QSlider>

class RangeSlider : public QSlider {
    Q_OBJECT

public:
    RangeSlider(QWidget* parent = nullptr);
    int lowerValue() const;
    int upperValue() const;

    float minRealValue() const;
    float maxRealValue() const;
    float lowPosRate() const;
    float upperPosRate() const;
    void  setLowPosRate(float rate);
    void  setUpperPosRate(float rate);
    float lowerRealValue() const;
    float upperRealValue() const;
    void  setLowerRealValue(float value);
    void  setUpperRealValue(float value);

    void init(float min_value, float max_value, float step, int decimals, double low_upper_min_diff);

    void setEnableLower(bool enable);
    void setEnableUpper(bool enable);

    bool isEnableLower() const { return m_enable_lower; }
    bool isEnableUpper() const { return m_enable_upper; }

    void setSpinBox(QDoubleSpinBox* lower_spin, QDoubleSpinBox* upper_spin);
    void setRealRange(float lower, float upper, float step);
    void setIntRange(int lower, int upper, int step);
    // void setRange(int min_value, int max_value);

    void setLowerValue(int value);
    void setUpperValue(int value);

    QDoubleSpinBox* lowerSpinBox() const { return m_lower_spin_box; }
    QDoubleSpinBox* upperSpinBox() const { return m_upper_spin_box; }

    bool isActiveLower() const;
    bool isActiveUpper() const;

signals:
    void lowerValueChanged(int value);
    void upperValueChanged(int value);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    enum Handle { None, LowerHandle, UpperHandle };
    Handle m_active_handle  = None;
    int    m_lower_value    = 0;
    int    m_upper_value    = 100;
    int    m_handle_size    = 5;
    bool   m_dragging_lower = false;
    bool   m_dragging_upper = false;
    bool   m_enable_lower   = false;
    bool   m_enable_upper   = false;

    /// SpinBox連動
    float           m_range_min_value    = 0.0f;
    float           m_range_max_value    = 0.0f;
    float           m_range_step         = 0.0f;
    double          m_low_upper_min_diff = 0;
    QDoubleSpinBox* m_lower_spin_box     = nullptr;
    QDoubleSpinBox* m_upper_spin_box     = nullptr;

    double m_lower_back_value = -1;
    double m_upper_back_value = -1;

    /// Back Up
    int   m_backup_lower_value      = 0;
    int   m_backup_upper_value      = 0;
    float m_backup_lower_spin_value = 0;
    float m_backup_upper_spin_value = 0;
};

#endif    // RANGESLIDER_H
