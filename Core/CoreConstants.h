#ifndef CORE_CORECONSTANTS_H
#define CORE_CORECONSTANTS_H

#include <memory>
#include "CoreGlobal.h"
CORE_NAMESPACE_BEGIN

/// クラス名と同じにする（変えるのであればObjectヘッダのマクロ直す/マクロ引数に渡すようにする）
enum class ObjectType {
    TypeNone       = 0,      /// 未設定
    Shape          = 100,    /// Shape（使うことはない/基底クラス）
    Voxel          = 150,    /// ボクセル
    VoxelScalar    = 160,    /// 解析結果のボクセル
    Annotation     = 200,    /// Annotation（使うことはない/基底クラス）
    Dimension      = 250,    /// 寸法
    MultiDimension = 251,    /// 自動寸法（マルチ寸法）
};

enum class AttributeType {
    TypeNone = 0,
};

/// Snap用の定義
/// 　※ 暫定でここ
/// 　組み合わせなのでビット演算可能にする
enum RenderSnap {
    SnapNone   = 0x01,
    SnapAny    = 0x02,
    SnapVertex = 0x04,
    SnapEdge   = 0x08,
    /// 組み合わせ
    SnapVertexEdge = SnapVertex | SnapEdge,
    SnapVertexAny  = SnapAny | SnapVertex | SnapEdge,
};

/// Text描画情報
/// 　※ 暫定でここ
enum TextAlignment {
    BottomLeft = 0,
    TopLeft,
    CenterCenter,
    BottomRight,
    TopRight,
    BottomCenter,
    TopCenter,
    CenterLeft,
    CenterRight
};

CORE_NAMESPACE_END
#endif    // CORE_CORECONSTANTS_H
