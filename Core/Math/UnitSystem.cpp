#include "UnitSystem.h"

CORE_NAMESPACE_BEGIN

void LengthUnit::changeUnit(Unit unit)
{
    if (m_unit == unit) {
        return;
    }

    auto toMeter = [](Unit unit) -> float {
        switch (unit) {
            case Unit::Nanometer:
                return 1.0e9f;
            case Unit::Micrometer:
                return 1.0e6f;
            case Unit::Millimeter:
                return 1.0e3f;
            case Unit::Meter:
                return 1.0f;
            default:
                return 1.0f;
        }
    };

    float conversion_factor = toMeter(unit) / toMeter(m_unit);

    m_eps_zero *= conversion_factor;
    m_eps_valid_length *= conversion_factor;

    m_unit = unit;
}

float LengthUnit::changeUnit(Unit origin_unit, Unit new_unit, float value)
{
    auto toMeter = [](Unit unit) -> float {
        switch (unit) {
            case Unit::Nanometer:
                return 1.0e9f;
            case Unit::Micrometer:
                return 1.0e6f;
            case Unit::Millimeter:
                return 1.0e3f;
            case Unit::Meter:
                return 1.0f;
            default:
                return 1.0f;
        }
    };

    float conversion_factor = toMeter(new_unit) / toMeter(origin_unit);

    return value * conversion_factor;
}

std::wstring LengthUnit::unitString(Unit unit)
{
    switch (unit) {
        case Unit::Nanometer:
            return L"nm";
        case Unit::Micrometer:
            return L"um";
        case Unit::Millimeter:
            return L"mm";
        case Unit::Meter:
            return L"m";
        default:
            return L"";
    }
}

CORE_NAMESPACE_END
