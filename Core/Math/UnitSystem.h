#ifndef CORE_UNITSYSTEM_H
#define CORE_UNITSYSTEM_H

#include <cmath>
#include "CoreGlobal.h"

CORE_NAMESPACE_BEGIN

/// 今後の単位系変更考慮して許容値等はクラスを通す
class CORE_EXPORT LengthUnit {
public:
    enum class Unit { Nanometer, Micrometer, Millimeter, Meter };

    void set(Unit unit, float eps_zero, float eps_valid_length)
    {
        m_unit             = unit;
        m_eps_zero         = eps_zero;
        m_eps_valid_length = eps_valid_length;
    }

    Unit currentUnit() const { return m_unit; }

    void changeUnit(Unit unit);

    static float changeUnit(Unit origin_unit, Unit new_unit, float value);

    static std::wstring unitString(Unit unit);

    inline float epsilonZero() const { return m_eps_zero; }
    inline float epsilonZeroSquared() const { return std::pow(epsilonZero(), 2); }

    inline float epsilonValidLength() const { return m_eps_valid_length; }
    inline float epsilonValidLengthSquared() const { return std::pow(epsilonValidLength(), 2); }

protected:
    Unit m_unit = Unit::Millimeter;

    /// ゼロ判定の許容値
    float m_eps_zero = 1.0e-6f;

    /// 妥当な長さ（距離判定など）の許容値
    float m_eps_valid_length = 1.0e-4f;
};
CORE_NAMESPACE_END

#endif    // CORE_UNITSYSTEM_H
