#include "Node.h"
#include "Shape.h"

CORE_NAMESPACE_BEGIN

Node::Node()
    : m_name(L"")
    , m_parent(nullptr)
    , m_object(nullptr)
    , m_renderable(nullptr)
    , m_box_dirty(true)
    , m_show(true)
    , m_suppress(false)
{
}

Node::~Node()
{
    if (m_object.ptr()) {
        m_object->deleteNode(this);
    }
    if (m_renderable.ptr()) {
        m_renderable->deleteNode(this);
    }

    removeAllChild();
}

RefPtr<Node> Node::createNode()
{
    return RefPtr<Node>(new Node());
}

const std::wstring& Node::name() const
{
    return m_name;
}

void Node::setName(const std::wstring& name)
{
    m_name = name;
}

#ifdef USE_QT
void Node::setName(const QString& name)
{
    m_name = name.toStdWString();
}
#endif

void Node::setParent(Node* parent)
{
    m_parent = parent;
}

Node* Node::parent() const
{
    return m_parent;
}

void Node::appendChild(Node* child)
{
    m_children.emplace_back(child);
    child->setParent(this);
    markBoxDirty();
}

Node* Node::addChild()
{
    auto node = createNode();
    appendChild(node.ptr());
    return node.ptr();
}

void Node::removeChild(Node* node)
{
    auto it = std::find(m_children.begin(), m_children.end(), node);
    if (it != m_children.end()) {
        (*it)->setParent(nullptr);
        // delete *it; /// RefPtr
        m_children.erase(it);
        markBoxDirty();
    }
}

void Node::removeChild(const std::set<Node*>& nodes)
{
    auto it = std::remove_if(m_children.begin(), m_children.end(), [&nodes](RefPtr<Node>& child) {
        if (nodes.count(child.ptr())) {
            child->setParent(nullptr);
            /// RefPtr化
            // delete child;
            return true;
        }
        return false;
    });

    m_children.erase(it, m_children.end());
    markBoxDirty();
}

void Node::removeAllChild(bool recur)
{
    for (auto& child : m_children) {
        if (recur) {
            child->removeAllChild(recur);
        }
        child->setParent(nullptr);
        /// RefPtr化
        // delete child;
    }
    m_children.clear();
    markBoxDirty();
}

const VecNode& Node::children() const
{
    return m_children;
}

VecNode& Node::children()
{
    return m_children;
}

int Node::numChildren()
{
    return (int)m_children.size();
}

Node* Node::child(int index)
{
    return m_children[index].ptr();
}

void Node::setObject(Object* object)
{
    m_object = object;
    if (m_object != nullptr) {
        if (m_object->isShape()) {
            m_renderable = RenderableNode::createRenderableNode(((Shape*)m_object.ptr())->renderableObject());
        }
    }

    markBoxDirty();
}

Object* Node::object()
{
    return m_object.ptr();
}

const Object* Node::object() const
{
    return m_object.ptr();
}

void Node::setRenderable(RenderableNode* renderable)
{
    m_renderable = renderable;
}

RenderableNode* Node::renderable()
{
    return m_renderable.ptr();
}

const RenderableNode* Node::renderable() const
{
    return m_renderable.ptr();
}

void* Node::renderData()
{
    if (m_renderable != nullptr) {
        return m_renderable->renderData();
    }
    return nullptr;
}

void Node::clearDisplayData()
{
    if (m_renderable != nullptr) {
        m_renderable->clearDisplayData();
    }
    /*
    m_vertices.clear();
    m_indices.clear();
    clearDisplayEditData();
    markSegmentsGroupDirty();
    markRenderDirty();
    */
    markBoxDirty();
}

Shape* Node::shape() const
{
    auto obj = object();
    if (obj && obj->isShape()) {
        return (Shape*)obj;
    }

    return nullptr;
}

void Node::setBoundingBox(const BoundingBox3f& box)
{
    m_bbox = box;
    markBoxDirty();
}

void Node::markBoxDirty(bool notify_parent)
{
    m_box_dirty = true;
    if (notify_parent) {
        notifyParentBoxDirty();
    }
}

void Node::resetBoxDirty()
{
    m_box_dirty = false;
}

bool Node::isBoxDirty() const
{
    return m_box_dirty;
}

bool Node::isVisible() const
{
    if (isSuppress()) {
        return false;
    }
    if (!isShow()) {
        return false;
    }
    return true;
}

bool Node::isParentVisible() const
{
    Node* current = m_parent;
    while (current) {
        if (!current->isVisible()) {
            return false;
        }
        current = current->parent();
    }
    return true;
}

void Node::show()
{
    m_show = true;
}

void Node::hide()
{
    m_show = false;
}

bool Node::isShow() const
{
    return m_show;
}

void Node::setShowHierarchy(bool _show)
{
    /// 非表示
    if (!_show) {
        hide();
        return;
    }

    /// 自身と子を表示
    show();
    for (auto& child : m_children) {
        child->setShowChildren(_show);
    }

    /// 親が非表示なら表示させつつ兄弟は非表示（※元の階層表示を維持）
    Node* current = m_parent;
    while (current) {
        if (!current->isShow()) {
            current->show();
            for (auto child : current->children()) {
                if (child != this) {
                    child->hide();
                }
            }
            return;
        }
        current = current->parent();
    }
}

void Node::setShowChildren(bool _show)
{
    for (auto child : m_children) {
        if (_show) {
            child->show();
        }
        else {
            child->hide();
        }
        child->setShowChildren(_show);
    }
}

void Node::suppress()
{
    m_suppress = true;
}

void Node::unsuppress()
{
    m_suppress = false;
}

bool Node::isSuppress() const
{
    return m_suppress;
}

void Node::setSuppressHierarchy(bool _suppress)
{
    /// 抑制
    if (_suppress) {
        suppress();
        return;
    }

    /// 自身と子を抑制解除
    unsuppress();
    for (auto& child : m_children) {
        child->setSuppressHierarchy(_suppress);
    }

    /// 親が抑制なら抑制解除させつつ兄弟は抑制（※元の階層表示を維持）
    Node* current = m_parent;
    while (current) {
        if (current->isSuppress()) {
            current->unsuppress();
            for (auto child : current->children()) {
                if (child != this) {
                    child->suppress();
                }
            }
            return;
        }
        current = current->parent();
    }
}

void Node::setSuppressChildren(bool _suppress)
{
    for (auto child : m_children) {
        if (_suppress) {
            child->suppress();
        }
        else {
            child->unsuppress();
        }
        child->setSuppressChildren(_suppress);
    }
}

void Node::notifyParentBoxDirty()
{
    Node* current = m_parent;
    while (current) {
        if (current->isBoxDirty()) {
            return;
        }
        current->markBoxDirty(false);
        current = current->parent();
    }
}

const BoundingBox3f& Node::boundingBox()
{
    if (isBoxDirty()) {
        updateBoundingBox();
    }
    return m_bbox;
}

BoundingBox3f Node::displayBoundingBox()
{
    if (!isVisible()) {
        return BoundingBox3f();
    }

    BoundingBox3f bbox;
    /// 自身のみのBoundingBox
    bbox.expandBy(renderableBoundingBox());

    for (auto& child : m_children) {
        bbox.expandBy(child->matrix() * child->displayBoundingBox());
    }

    return bbox;
}

BoundingBox3f Node::shapeBoundingBox()
{
    BoundingBox3f bbox;
    if (m_object != nullptr && m_object->isShape()) {
        /// 自身のみのBoundingBox
        bbox.expandBy(renderableBoundingBox());
    }

    for (auto& child : m_children) {
        bbox.expandBy(child->matrix() * child->shapeBoundingBox());
    }

    return bbox;
}

BoundingBox3f Node::shapeDisplayBoundingBox()
{
    if (!isVisible()) {
        return BoundingBox3f();
    }

    BoundingBox3f bbox;
    if (m_object != nullptr && m_object->isShape()) {
        /// 自身のみのBoundingBox
        bbox.expandBy(renderableBoundingBox());
    }

    for (auto& child : m_children) {
        bbox.expandBy(child->matrix() * child->shapeDisplayBoundingBox());
    }

    return bbox;
}

void Node::updateBoundingBox()
{
    BoundingBox3f new_bbox = renderableBoundingBox();
    for (auto& child : m_children) {
        new_bbox.expandBy(child->matrix() * child->boundingBox());
    }
    m_bbox = new_bbox;
    resetBoxDirty();
}

BoundingBox3f Node::renderableBoundingBox()
{
    if (m_renderable != nullptr) {
        return m_renderable->boundingBox();
    }
    else if (m_object != nullptr) {
        return m_object->boundingBox();
    }
    return BoundingBox3f();
}

BoundingBox3f Node::calculateBoundingBox(const Matrix4x4f& parent_matrix, bool only_visible, bool including_text) const
{
    if (only_visible && !isVisible()) {
        return BoundingBox3f();
    }

    Matrix4x4f cur_matrix = parent_matrix * m_matrix;

    BoundingBox3f new_bbox;
    if (m_renderable != nullptr) {
        new_bbox.expandBy(m_renderable->calculateBoundingBox(cur_matrix, only_visible, including_text));
    }
    else if (m_object != nullptr) {
        new_bbox.expandBy(m_object->calculateBoundingBox(cur_matrix, only_visible, including_text));
    }

    for (auto& child : m_children) {
        new_bbox.expandBy(child->calculateBoundingBox(cur_matrix, only_visible, including_text));
    }

    return new_bbox;
}

void Node::setMatrix(const Matrix4x4f& matrix)
{
    m_matrix = matrix;
}

const Matrix4x4f& Node::matrix() const
{
    return m_matrix;
}

Matrix4x4f Node::pathMatrix() const
{
    Matrix4x4f  result;
    const Node* current = this;
    while (current) {
        result  = current->matrix() * result;
        current = current->parent();
    }
    return result;
}

Matrix4x4f Node::parentPathMatrix() const
{
    Matrix4x4f  result;
    const Node* current = m_parent;
    while (current) {
        result  = current->matrix() * result;
        current = current->parent();
    }
    return result;
}

void Node::setUserAttributeInt(const std::wstring& key, int value)
{
    m_attribute_user.setAttribute(key, value);
}

int Node::userAttributeInt(const std::wstring& key)
{
    auto& attr = m_attribute_user.attribute(key);
    if (attr.type() == typeid(int)) {
        return std::any_cast<int>(attr);
    }

    return 0;
}

void Node::setUserAttributeFloat(const std::wstring& key, float value)
{
    m_attribute_user.setAttribute(key, value);
}

float Node::userAttributeFloat(const std::wstring& key)
{
    auto& attr = m_attribute_user.attribute(key);
    if (attr.type() == typeid(float)) {
        return std::any_cast<float>(attr);
    }

    return 0;
}

void Node::setUserAttributeString(const std::wstring& key, const std::wstring& value)
{
    m_attribute_user.setAttribute(key, value);
}

const std::wstring Node::userAttributeString(const std::wstring& key)
{
    auto& attr = m_attribute_user.attribute(key);
    if (attr.type() == typeid(std::wstring)) {
        return std::any_cast<std::wstring>(attr);
    }

    return L"";
}

RefPtr<Node> Node::copyInstance()
{
    auto copy_node = createNode();

    /// 暫定 - copyなので変えるべき
    copy_node->setName(m_name);

    copy_node->setObject(m_object.ptr());
    copy_node->setMatrix(m_matrix);

    /// RenderDataはNode単位
    if (m_renderable != nullptr) {
        copy_node->setRenderable((RenderableNode*)m_renderable->cloneRenderable().ptr());
    }

    /// 暫定 ー ものによってはこれだとダメだと思う
    copy_node->m_attribute.setAttributes(m_attribute.attributes());
    copy_node->m_attribute_user.setAttributes(m_attribute_user.attributes());

    copy_node->m_show     = m_show;
    copy_node->m_suppress = m_suppress;

    for (auto& child : m_children) {
        auto copy_child = child->copyInstance();
        copy_node->appendChild(copy_child.ptr());
    }

    return copy_node;
}

CORE_NAMESPACE_END
