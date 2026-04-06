#ifndef CORE_SEARCHPERFORM_H
#define CORE_SEARCHPERFORM_H

#include "CoreGlobal.h"

#include <deque>
#include <list>
#include <map>
#include <set>
#include <vector>

CORE_NAMESPACE_BEGIN

/// XY座標に対するデータの検索
template <typename _myData, typename _myDouble>
class SearchXYData {
public:
    SearchXYData() { m_eps = (_myDouble)0; }
    SearchXYData(_myDouble eps) { m_eps = eps; }
    ~SearchXYData() {}

    void      setEPS(_myDouble eps) { m_eps = eps; }
    _myDouble eps() const { return m_eps; }

    void appendData(_myDouble x, _myDouble y, const _myData& data) { m_map_xy_data[x][y].insert(data); }

    bool search(_myDouble x, _myDouble y, std::set<_myData>& search_all_data, _myDouble _EPS_2 = (_myDouble)-1) const
    {
        _myDouble _EPS = (_EPS_2 < (_myDouble)0) ? m_eps : _EPS_2;

        /// X座標範囲検索
        auto itr_x0 = m_map_xy_data.lower_bound((_myDouble)x - _EPS);
        auto itr_x1 = m_map_xy_data.upper_bound((_myDouble)x + _EPS);
        if (itr_x0 == m_map_xy_data.end() || itr_x1 == m_map_xy_data.begin()) {
            return false;
        }

        _myDouble search_distance_2 = _EPS * _EPS;

        _myDouble dist2_x  = (_myDouble)0;
        _myDouble dist2_xy = (_myDouble)0;

        bool ret = false;

        for (itr_x0; itr_x0 != itr_x1; ++itr_x0) {
            const _myDouble& x0         = itr_x0->first;
            auto&            map_y_data = itr_x0->second;

            dist2_x = x - x0;
            dist2_x *= dist2_x;

            /// Y座標範囲検索
            auto itr_y0 = map_y_data.lower_bound((_myDouble)y - _EPS);
            auto itr_y1 = map_y_data.upper_bound((_myDouble)y + _EPS);
            if (itr_y0 == map_y_data.end() || itr_y1 == map_y_data.begin()) {
                continue;
            }

            for (itr_y0; itr_y0 != itr_y1; ++itr_y0) {
                const _myDouble& y0 = itr_y0->first;

                dist2_xy = dist2_x + (y - y0) * (y - y0);
                if (dist2_xy > search_distance_2) {
                    continue;
                }

                search_all_data.insert(itr_y0->second.begin(), itr_y0->second.end());
                ret = true;
            }
        }

        return ret;
    }

    bool search(_myDouble x, _myDouble y, _myDouble _EPS_2 = (_myDouble)-1) const
    {
        _myDouble _EPS = (_EPS_2 < (_myDouble)0) ? m_eps : _EPS_2;

        /// X座標範囲検索
        auto itr_x0 = m_map_xy_data.lower_bound((_myDouble)x - _EPS);
        auto itr_x1 = m_map_xy_data.upper_bound((_myDouble)x + _EPS);
        if (itr_x0 == m_map_xy_data.end() || itr_x1 == m_map_xy_data.begin()) {
            return false;
        }

        _myDouble search_distance_2 = _EPS * _EPS;

        _myDouble dist2_x  = (_myDouble)0;
        _myDouble dist2_xy = (_myDouble)0;

        bool ret = false;

        for (itr_x0; itr_x0 != itr_x1; ++itr_x0) {
            const _myDouble& x0         = itr_x0->first;
            auto&            map_y_data = itr_x0->second;

            dist2_x = x - x0;
            dist2_x *= dist2_x;

            /// Y座標範囲検索
            auto itr_y0 = map_y_data.lower_bound((_myDouble)y - _EPS);
            auto itr_y1 = map_y_data.upper_bound((_myDouble)y + _EPS);
            if (itr_y0 == map_y_data.end() || itr_y1 == map_y_data.begin()) {
                continue;
            }

            for (itr_y0; itr_y0 != itr_y1; ++itr_y0) {
                const _myDouble& y0 = itr_y0->first;

                dist2_xy = dist2_x + (y - y0) * (y - y0);
                if (dist2_xy > search_distance_2) {
                    continue;
                }

                return true;
            }
        }

        return false;
    }

    int dataCount() const
    {
        int count = 0;
        for (const auto& [x, map_y_data] : m_map_xy_data) {
            for (const auto& [y, set_data] : map_y_data) {
                count += (int)set_data.size();
            }
        }
        return count;
    }

    const auto& alldata() const { return m_map_xy_data; }

protected:
    std::map<_myDouble, std::map<_myDouble, std::set<_myData>>> m_map_xy_data;
    _myDouble                                                   m_eps;
};

/// XY座標に対するデータの検索（カウントのみ）
template <typename _myDouble>
class SearchXYCount {
public:
    SearchXYCount() { m_eps = (_myDouble)0; }
    SearchXYCount(_myDouble eps) { m_eps = eps; }
    ~SearchXYCount() {}

    void      setEPS(_myDouble eps) { m_eps = eps; }
    _myDouble eps() const { return m_eps; }

    void appendData(_myDouble x, _myDouble y) { m_map_xy_count[x][y]++; }

    bool count(_myDouble x, _myDouble y, _myDouble _EPS_2 = (_myDouble)-1) const
    {
        _myDouble _EPS = (_EPS_2 < (_myDouble)0) ? m_eps : _EPS_2;

        /// X座標範囲検索
        auto itr_x0 = m_map_xy_count.lower_bound((_myDouble)x - _EPS);
        auto itr_x1 = m_map_xy_count.upper_bound((_myDouble)x + _EPS);
        if (itr_x0 == m_map_xy_count.end() || itr_x1 == m_map_xy_count.begin()) {
            return 0;
        }

        _myDouble search_distance_2 = _EPS * _EPS;

        _myDouble dist2_x  = (_myDouble)0;
        _myDouble dist2_xy = (_myDouble)0;

        int count = 0;

        for (itr_x0; itr_x0 != itr_x1; ++itr_x0) {
            const _myDouble& x0          = itr_x0->first;
            auto&            map_y_count = itr_x0->second;

            dist2_x = x - x0;
            dist2_x *= dist2_x;

            /// Y座標範囲検索
            auto itr_y0 = map_y_count.lower_bound((_myDouble)y - _EPS);
            auto itr_y1 = map_y_count.upper_bound((_myDouble)y + _EPS);
            if (itr_y0 == map_y_count.end() || itr_y1 == map_y_count.begin()) {
                continue;
            }

            for (itr_y0; itr_y0 != itr_y1; ++itr_y0) {
                const _myDouble& y0 = itr_y0->first;

                dist2_xy = dist2_x + (y - y0) * (y - y0);
                if (dist2_xy > search_distance_2) {
                    continue;
                }

                count += (int)itr_y0->second;
            }
        }

        return count;
    }

    bool search(_myDouble x, _myDouble y, _myDouble _EPS_2 = (_myDouble)-1) const
    {
        _myDouble _EPS = (_EPS_2 < (_myDouble)0) ? m_eps : _EPS_2;

        /// X座標範囲検索
        auto itr_x0 = m_map_xy_count.lower_bound((_myDouble)x - _EPS);
        auto itr_x1 = m_map_xy_count.upper_bound((_myDouble)x + _EPS);
        if (itr_x0 == m_map_xy_count.end() || itr_x1 == m_map_xy_count.begin()) {
            return false;
        }

        _myDouble search_distance_2 = _EPS * _EPS;

        _myDouble dist2_x  = (_myDouble)0;
        _myDouble dist2_xy = (_myDouble)0;

        bool ret = false;

        for (itr_x0; itr_x0 != itr_x1; ++itr_x0) {
            const _myDouble& x0          = itr_x0->first;
            auto&            map_y_count = itr_x0->second;

            dist2_x = x - x0;
            dist2_x *= dist2_x;

            /// Y座標範囲検索
            auto itr_y0 = map_y_count.lower_bound((_myDouble)y - _EPS);
            auto itr_y1 = map_y_count.upper_bound((_myDouble)y + _EPS);
            if (itr_y0 == map_y_count.end() || itr_y1 == map_y_count.begin()) {
                continue;
            }

            for (itr_y0; itr_y0 != itr_y1; ++itr_y0) {
                const _myDouble& y0 = itr_y0->first;

                dist2_xy = dist2_x + (y - y0) * (y - y0);
                if (dist2_xy > search_distance_2) {
                    continue;
                }

                return true;
            }
        }

        return false;
    }

    int dataCount() const
    {
        int all_count = 0;
        for (const auto& [x, map_y_data] : m_map_xy_count) {
            for (const auto& [y, count] : m_map_xy_count) {
                all_count += (int)count;
            }
        }
        return all_count;
    }

    const auto& alldata() const { return m_map_xy_count; }

protected:
    std::map<_myDouble, std::map<_myDouble, int>> m_map_xy_count;
    _myDouble                                     m_eps;
};

/// XY座標２つに対するデータの検索
template <typename _myData, typename _myDouble>
class SearchXYXYData {
public:
    SearchXYXYData() { m_eps = (_myDouble)0; }
    SearchXYXYData(_myDouble eps) { m_eps = eps; }
    ~SearchXYXYData() {}

    void      setEPS(_myDouble eps) { m_eps = eps; }
    _myDouble eps() const { return m_eps; }

    void appendData(_myDouble x, _myDouble y, _myDouble x2, _myDouble y2, const _myData& data, bool sort = true)
    {
        if (sort) {
            if (x < x2) {
                m_map_xyxy_data[x][y][x2][y2].insert(data);
            }
            else if (x == x2) {
                if (y > y2) {
                    m_map_xyxy_data[x2][y2][x][y].insert(data);
                }
                else {
                    m_map_xyxy_data[x][y][x2][y2].insert(data);
                }
            }
            else {
                m_map_xyxy_data[x2][y2][x][y].insert(data);
            }
        }
        else {
            m_map_xyxy_data[x][y][x2][y2].insert(data);
        }
    }

    bool search(_myDouble x, _myDouble y, _myDouble x2, _myDouble y2, std::set<_myData>& searchAllData,
                bool sort = true, _myDouble _EPS_2 = (_myDouble)-1) const
    {
        if (sort) {
            if (x < x2) {
            }
            else if (x == x2) {
                if (y > y2) {
                    _myDouble tempx = x2;
                    _myDouble tempy = y2;
                    x2              = x;
                    y2              = y;
                    x               = tempx;
                    y               = tempy;
                }
            }
            else {
                _myDouble tempx = x2;
                _myDouble tempy = y2;
                x2              = x;
                y2              = y;
                x               = tempx;
                y               = tempy;
            }
        }

        _myDouble _EPS = (_EPS_2 < (_myDouble)0) ? m_eps : _EPS_2;

        /// X座標範囲検索
        auto itr_x0 = m_map_xyxy_data.lower_bound((_myDouble)x - _EPS);
        auto itr_x1 = m_map_xyxy_data.upper_bound((_myDouble)x + _EPS);
        if (itr_x0 == m_map_xyxy_data.end() || itr_x1 == m_map_xyxy_data.begin()) {
            return false;
        }

        _myDouble search_distance_2 = _EPS * _EPS;

        _myDouble dist2_x  = (_myDouble)0;
        _myDouble dist2_xy = (_myDouble)0;

        bool ret = false;

        for (itr_x0; itr_x0 != itr_x1; ++itr_x0) {
            const double& x0           = itr_x0->first;
            auto&         map_yxy_data = itr_x0->second;

            dist2_x = x - x0;
            dist2_x *= dist2_x;

            /// Y座標範囲検索
            auto itr_y0 = map_yxy_data.lower_bound((_myDouble)y - _EPS);
            auto itr_y1 = map_yxy_data.upper_bound((_myDouble)y + _EPS);
            if (itr_y0 == map_yxy_data.end() || itr_y1 == map_yxy_data.begin()) {
                continue;
            }

            for (itr_y0; itr_y0 != itr_y1; ++itr_y0) {
                const _myDouble& y0          = itr_y0->first;
                auto&            map_xy_data = itr_y0->second;

                dist2_xy = dist2_x + (y - y0) * (y - y0);
                if (dist2_xy > search_distance_2) {
                    continue;
                }

                /// X座標範囲検索
                auto itr_x20 = map_xy_data.lower_bound((_myDouble)x2 - _EPS);
                auto itr_x21 = map_xy_data.upper_bound((_myDouble)x2 + _EPS);
                if (itr_x20 == map_xy_data.end() || itr_x21 == map_xy_data.begin()) {
                    continue;
                }

                _myDouble dist2_2x  = (_myDouble)0;
                _myDouble dist2_2xy = (_myDouble)0;

                for (itr_x20; itr_x20 != itr_x21; ++itr_x20) {
                    const double& x20        = itr_x20->first;
                    auto&         map_y_data = itr_x20->second;

                    dist2_2x = x2 - x20;
                    dist2_2x *= dist2_2x;

                    /// Y座標範囲検索
                    auto itr_y20 = map_y_data.lower_bound((_myDouble)y2 - _EPS);
                    auto itr_y21 = map_y_data.upper_bound((_myDouble)y2 + _EPS);
                    if (itr_y20 == map_y_data.end() || itr_y21 == map_y_data.begin()) {
                        continue;
                    }

                    for (itr_y20; itr_y20 != itr_y21; ++itr_y20) {
                        const _myDouble& y20 = itr_y20->first;

                        dist2_2xy = dist2_2x + (y2 - y20) * (y2 - y20);
                        if (dist2_2xy > search_distance_2) {
                            continue;
                        }

                        searchAllData.insert(itr_y20->second.begin(), itr_y20->second.end());
                        ret = true;
                    }
                }
            }
        }

        return ret;
    }

    bool search(_myDouble x, _myDouble y, _myDouble x2, _myDouble y2, bool sort = true,
                _myDouble _EPS_2 = (_myDouble)-1) const
    {
        if (sort) {
            if (x < x2) {
            }
            else if (x == x2) {
                if (y > y2) {
                    _myDouble tempx = x2;
                    _myDouble tempy = y2;
                    x2              = x;
                    y2              = y;
                    x               = tempx;
                    y               = tempy;
                }
            }
            else {
                _myDouble tempx = x2;
                _myDouble tempy = y2;
                x2              = x;
                y2              = y;
                x               = tempx;
                y               = tempy;
            }
        }

        _myDouble _EPS = (_EPS_2 < (_myDouble)0) ? m_eps : _EPS_2;

        /// X座標範囲検索
        auto itr_x0 = m_map_xyxy_data.lower_bound((_myDouble)x - _EPS);
        auto itr_x1 = m_map_xyxy_data.upper_bound((_myDouble)x + _EPS);
        if (itr_x0 == m_map_xyxy_data.end() || itr_x1 == m_map_xyxy_data.begin()) {
            return false;
        }

        _myDouble search_distance_2 = _EPS * _EPS;

        _myDouble dist2_x  = (_myDouble)0;
        _myDouble dist2_xy = (_myDouble)0;

        bool ret = false;

        for (itr_x0; itr_x0 != itr_x1; ++itr_x0) {
            const double& x0           = itr_x0->first;
            auto&         map_yxy_data = itr_x0->second;

            dist2_x = x - x0;
            dist2_x *= dist2_x;

            /// Y座標範囲検索
            auto itr_y0 = map_yxy_data.lower_bound((_myDouble)y - _EPS);
            auto itr_y1 = map_yxy_data.upper_bound((_myDouble)y + _EPS);
            if (itr_y0 == map_yxy_data.end() || itr_y1 == map_yxy_data.begin()) {
                continue;
            }

            for (itr_y0; itr_y0 != itr_y1; ++itr_y0) {
                const _myDouble& y0          = itr_y0->first;
                auto&            map_xy_data = itr_y0->second;

                dist2_xy = dist2_x + (y - y0) * (y - y0);
                if (dist2_xy > search_distance_2) {
                    continue;
                }

                /// X座標範囲検索
                auto itr_x20 = map_xy_data.lower_bound((_myDouble)x2 - _EPS);
                auto itr_x21 = map_xy_data.upper_bound((_myDouble)x2 + _EPS);
                if (itr_x20 == map_xy_data.end() || itr_x21 == map_xy_data.begin()) {
                    continue;
                }

                _myDouble dist2_2x  = (_myDouble)0;
                _myDouble dist2_2xy = (_myDouble)0;

                for (itr_x20; itr_x20 != itr_x21; ++itr_x20) {
                    const double& x20        = itr_x20->first;
                    auto&         map_y_data = itr_x20->second;

                    dist2_2x = x2 - x20;
                    dist2_2x *= dist2_2x;

                    /// Y座標範囲検索
                    auto itr_y20 = map_y_data.lower_bound((_myDouble)y2 - _EPS);
                    auto itr_y21 = map_y_data.upper_bound((_myDouble)y2 + _EPS);
                    if (itr_y20 == map_y_data.end() || itr_y21 == map_y_data.begin()) {
                        continue;
                    }

                    for (itr_y20; itr_y20 != itr_y21; ++itr_y20) {
                        const _myDouble& y20 = itr_y20->first;

                        dist2_2xy = dist2_2x + (y2 - y20) * (y2 - y20);
                        if (dist2_2xy > search_distance_2) {
                            continue;
                        }

                        return true;
                    }
                }
            }
        }

        return false;
    }

    const auto& alldata() const { return m_map_xyxy_data; }

protected:
    std::map<_myDouble, std::map<_myDouble, std::map<_myDouble, std::map<_myDouble, std::set<_myData>>>>>
              m_map_xyxy_data;
    _myDouble m_eps;
};

/// XY座標２つに対するデータの検索（カウントのみ）
template <typename _myDouble>
class SearchXYXYCount {
public:
    SearchXYXYCount() { m_eps = (_myDouble)0; }
    SearchXYXYCount(_myDouble eps) { m_eps = eps; }
    ~SearchXYXYCount() {}

    void      setEPS(_myDouble eps) { m_eps = eps; }
    _myDouble eps() const { return m_eps; }

    void appendData(_myDouble x, _myDouble y, _myDouble x2, _myDouble y2, bool sort = true)
    {
        if (sort) {
            if (x < x2) {
                m_map_xyxy_count[x][y][x2][y2]++;
            }
            else if (x == x2) {
                if (y > y2) {
                    m_map_xyxy_count[x2][y2][x][y]++;
                }
                else {
                    m_map_xyxy_count[x][y][x2][y2]++;
                }
            }
            else {
                m_map_xyxy_count[x2][y2][x][y]++;
            }
        }
        else {
            m_map_xyxy_count[x][y][x2][y2]++;
        }
    }

    int count(_myDouble x, _myDouble y, _myDouble x2, _myDouble y2, bool sort = true,
              _myDouble _EPS_2 = (_myDouble)-1) const
    {
        if (sort) {
            if (x < x2) {
            }
            else if (x == x2) {
                if (y > y2) {
                    _myDouble tempx = x2;
                    _myDouble tempy = y2;
                    x2              = x;
                    y2              = y;
                    x               = tempx;
                    y               = tempy;
                }
            }
            else {
                _myDouble tempx = x2;
                _myDouble tempy = y2;
                x2              = x;
                y2              = y;
                x               = tempx;
                y               = tempy;
            }
        }

        _myDouble _EPS = (_EPS_2 < (_myDouble)0) ? m_eps : _EPS_2;

        /// X座標範囲検索
        auto itr_x0 = m_map_xyxy_count.lower_bound((_myDouble)x - _EPS);
        auto itr_x1 = m_map_xyxy_count.upper_bound((_myDouble)x + _EPS);
        if (itr_x0 == m_map_xyxy_count.end() || itr_x1 == m_map_xyxy_count.begin()) {
            return 0;
        }

        _myDouble search_distance_2 = _EPS * _EPS;

        _myDouble dist2_x  = (_myDouble)0;
        _myDouble dist2_xy = (_myDouble)0;

        int count = 0;

        for (itr_x0; itr_x0 != itr_x1; ++itr_x0) {
            const double& x0            = itr_x0->first;
            auto&         map_yxy_count = itr_x0->second;

            dist2_x = x - x0;
            dist2_x *= dist2_x;

            /// Y座標範囲検索
            auto itr_y0 = map_yxy_count.lower_bound((_myDouble)y - _EPS);
            auto itr_y1 = map_yxy_count.upper_bound((_myDouble)y + _EPS);
            if (itr_y0 == map_yxy_count.end() || itr_y1 == map_yxy_count.begin()) {
                continue;
            }

            for (itr_y0; itr_y0 != itr_y1; ++itr_y0) {
                const _myDouble& y0           = itr_y0->first;
                auto&            map_xy_count = itr_y0->second;

                dist2_xy = dist2_x + (y - y0) * (y - y0);
                if (dist2_xy > search_distance_2) {
                    continue;
                }

                /// X座標範囲検索
                auto itr_x20 = map_xy_count.lower_bound((_myDouble)x2 - _EPS);
                auto itr_x21 = map_xy_count.upper_bound((_myDouble)x2 + _EPS);
                if (itr_x20 == map_xy_count.end() || itr_x21 == map_xy_count.begin()) {
                    continue;
                }

                _myDouble dist2_2x  = (_myDouble)0;
                _myDouble dist2_2xy = (_myDouble)0;

                for (itr_x20; itr_x20 != itr_x21; ++itr_x20) {
                    const double& x20         = itr_x20->first;
                    auto&         map_y_count = itr_x20->second;

                    dist2_2x = x2 - x20;
                    dist2_2x *= dist2_2x;

                    /// Y座標範囲検索
                    auto itr_y20 = map_y_count.lower_bound((_myDouble)y2 - _EPS);
                    auto itr_y21 = map_y_count.upper_bound((_myDouble)y2 + _EPS);
                    if (itr_y20 == map_y_count.end() || itr_y21 == map_y_count.begin()) {
                        continue;
                    }

                    for (itr_y20; itr_y20 != itr_y21; ++itr_y20) {
                        const _myDouble& y20 = itr_y20->first;

                        dist2_2xy = dist2_2x + (y2 - y20) * (y2 - y20);
                        if (dist2_2xy > search_distance_2) {
                            continue;
                        }

                        count += (int)itr_y20->second;
                    }
                }
            }
        }

        return count;
    }

    bool search(_myDouble x, _myDouble y, _myDouble x2, _myDouble y2, bool sort = true,
                _myDouble _EPS_2 = (_myDouble)-1) const
    {
        if (sort) {
            if (x < x2) {
            }
            else if (x == x2) {
                if (y > y2) {
                    _myDouble tempx = x2;
                    _myDouble tempy = y2;
                    x2              = x;
                    y2              = y;
                    x               = tempx;
                    y               = tempy;
                }
            }
            else {
                _myDouble tempx = x2;
                _myDouble tempy = y2;
                x2              = x;
                y2              = y;
                x               = tempx;
                y               = tempy;
            }
        }

        _myDouble _EPS = (_EPS_2 < (_myDouble)0) ? m_eps : _EPS_2;

        /// X座標範囲検索
        auto itr_x0 = m_map_xyxy_count.lower_bound((_myDouble)x - _EPS);
        auto itr_x1 = m_map_xyxy_count.upper_bound((_myDouble)x + _EPS);
        if (itr_x0 == m_map_xyxy_count.end() || itr_x1 == m_map_xyxy_count.begin()) {
            return false;
        }

        _myDouble search_distance_2 = _EPS * _EPS;

        _myDouble dist2_x  = (_myDouble)0;
        _myDouble dist2_xy = (_myDouble)0;

        for (itr_x0; itr_x0 != itr_x1; ++itr_x0) {
            const double& x0            = itr_x0->first;
            auto&         map_yxy_count = itr_x0->second;

            dist2_x = x - x0;
            dist2_x *= dist2_x;

            /// Y座標範囲検索
            auto itr_y0 = map_yxy_count.lower_bound((_myDouble)y - _EPS);
            auto itr_y1 = map_yxy_count.upper_bound((_myDouble)y + _EPS);
            if (itr_y0 == map_yxy_count.end() || itr_y1 == map_yxy_count.begin()) {
                continue;
            }

            for (itr_y0; itr_y0 != itr_y1; ++itr_y0) {
                const _myDouble& y0           = itr_y0->first;
                auto&            map_xy_count = itr_y0->second;

                dist2_xy = dist2_x + (y - y0) * (y - y0);
                if (dist2_xy > search_distance_2) {
                    continue;
                }

                /// X座標範囲検索
                auto itr_x20 = map_xy_count.lower_bound((_myDouble)x2 - _EPS);
                auto itr_x21 = map_xy_count.upper_bound((_myDouble)x2 + _EPS);
                if (itr_x20 == map_xy_count.end() || itr_x21 == map_xy_count.begin()) {
                    continue;
                }

                _myDouble dist2_2x  = (_myDouble)0;
                _myDouble dist2_2xy = (_myDouble)0;

                for (itr_x20; itr_x20 != itr_x21; ++itr_x20) {
                    const double& x20         = itr_x20->first;
                    auto&         map_y_count = itr_x20->second;

                    dist2_2x = x2 - x20;
                    dist2_2x *= dist2_2x;

                    /// Y座標範囲検索
                    auto itr_y20 = map_y_count.lower_bound((_myDouble)y2 - _EPS);
                    auto itr_y21 = map_y_count.upper_bound((_myDouble)y2 + _EPS);
                    if (itr_y20 == map_y_count.end() || itr_y21 == map_y_count.begin()) {
                        continue;
                    }

                    for (itr_y20; itr_y20 != itr_y21; ++itr_y20) {
                        const _myDouble& y20 = itr_y20->first;

                        dist2_2xy = dist2_2x + (y2 - y20) * (y2 - y20);
                        if (dist2_2xy > search_distance_2) {
                            continue;
                        }

                        return true;
                    }
                }
            }
        }

        return false;
    }

    const auto& alldata() const { return m_map_xyxy_count; }

protected:
    std::map<_myDouble, std::map<_myDouble, std::map<_myDouble, std::map<_myDouble, int>>>> m_map_xyxy_count;
    _myDouble                                                                               m_eps;
};

CORE_NAMESPACE_END

#endif    // CORE_SEARCHPERFORM_H
