#ifndef CORE_MESH_H
#define CORE_MESH_H

#include "Scene/Voxel.h"

CORE_NAMESPACE_BEGIN

/// メッシュ形状クラス
/// 頂点・面インデックス・線インデックスを最適に構築したメッシュ形状を保持する
class CORE_EXPORT Mesh : public Voxel {
    DECLARE_META_OBJECT(Mesh)
protected:
    Mesh();
    Mesh(const Mesh& other);
    ~Mesh();

public:
    virtual ObjectType type() const override { return ObjectType::Mesh; }

    /// ボックス形状を生成する
    /// 8頂点を共有しつつ、面描画用インデックスと線描画用インデックスを最適に構築する
    void createBoxShape(const Point3f& min_pos, const Point3f& max_pos);
};

CORE_NAMESPACE_END

#endif    // CORE_MESH_H
