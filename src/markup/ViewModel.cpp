#include "tinalux/markup/ViewModel.h"

#include <optional>
#include <sstream>

namespace tinalux::markup {

namespace {

bool isArrayIndexSegment(std::string_view segment)
{
    if (segment.empty()) {
        return false;
    }

    for (char ch : segment) {
        if (ch < '0' || ch > '9') {
            return false;
        }
    }

    return true;
}

std::optional<std::size_t> parseArrayIndex(std::string_view segment)
{
    if (!isArrayIndexSegment(segment)) {
        return std::nullopt;
    }

    std::size_t value = 0;
    for (char ch : segment) {
        value = value * 10 + static_cast<std::size_t>(ch - '0');
    }
    return value;
}

std::string normalizeBindingPath(std::string_view path)
{
    const auto trim = [](std::string_view value) {
        std::size_t start = 0;
        while (start < value.size()
            && (value[start] == ' ' || value[start] == '\t'
                || value[start] == '\r' || value[start] == '\n')) {
            ++start;
        }

        std::size_t end = value.size();
        while (end > start
            && (value[end - 1] == ' ' || value[end - 1] == '\t'
                || value[end - 1] == '\r' || value[end - 1] == '\n')) {
            --end;
        }

        return value.substr(start, end - start);
    };

    std::string_view normalized = trim(path);
    constexpr std::string_view kModelPrefix = "model.";
    if (normalized.substr(0, kModelPrefix.size()) == kModelPrefix) {
        normalized.remove_prefix(kModelPrefix.size());
    }

    return std::string(normalized);
}

bool valueEquals(const core::Value& lhs, const core::Value& rhs)
{
    if (lhs.type() != rhs.type()) {
        return false;
    }

    switch (lhs.type()) {
    case core::ValueType::None:
        return true;
    case core::ValueType::Bool:
        return lhs.asBool() == rhs.asBool();
    case core::ValueType::Int:
        return lhs.asInt() == rhs.asInt();
    case core::ValueType::Float:
        return lhs.asFloat() == rhs.asFloat();
    case core::ValueType::String:
    case core::ValueType::Enum:
        return lhs.asString() == rhs.asString();
    case core::ValueType::Color:
        return lhs.asColor() == rhs.asColor();
    }

    return false;
}

bool nodeEquals(const ModelNode& lhs, const ModelNode& rhs)
{
    if (lhs.kind() != rhs.kind()) {
        return false;
    }

    if (lhs.isNull()) {
        return true;
    }

    if (lhs.isScalar()) {
        const core::Value* lhsValue = lhs.scalar();
        const core::Value* rhsValue = rhs.scalar();
        return lhsValue != nullptr
            && rhsValue != nullptr
            && valueEquals(*lhsValue, *rhsValue);
    }

    if (lhs.isObject()) {
        const auto* lhsObject = lhs.objectValue();
        const auto* rhsObject = rhs.objectValue();
        if (lhsObject == nullptr || rhsObject == nullptr || lhsObject->size() != rhsObject->size()) {
            return false;
        }

        for (const auto& [key, lhsChild] : *lhsObject) {
            const auto rhsIt = rhsObject->find(key);
            if (rhsIt == rhsObject->end() || !nodeEquals(lhsChild, rhsIt->second)) {
                return false;
            }
        }

        return true;
    }

    const auto* lhsArray = lhs.arrayValue();
    const auto* rhsArray = rhs.arrayValue();
    if (lhsArray == nullptr || rhsArray == nullptr || lhsArray->size() != rhsArray->size()) {
        return false;
    }

    for (std::size_t index = 0; index < lhsArray->size(); ++index) {
        if (!nodeEquals((*lhsArray)[index], (*rhsArray)[index])) {
            return false;
        }
    }

    return true;
}

bool pathMatchesOrContains(std::string_view listenerPath, std::string_view changedPath)
{
    if (listenerPath == changedPath) {
        return true;
    }

    if (listenerPath.size() > changedPath.size()
        && listenerPath.compare(0, changedPath.size(), changedPath) == 0
        && listenerPath[changedPath.size()] == '.') {
        return true;
    }

    if (changedPath.size() > listenerPath.size()
        && changedPath.compare(0, listenerPath.size(), listenerPath) == 0
        && changedPath[listenerPath.size()] == '.') {
        return true;
    }

    return false;
}

std::vector<std::string_view> splitPath(std::string_view path)
{
    std::vector<std::string_view> parts;
    std::size_t start = 0;
    while (start < path.size()) {
        std::size_t end = path.find('.', start);
        if (end == std::string_view::npos) {
            parts.push_back(path.substr(start));
            break;
        }

        parts.push_back(path.substr(start, end - start));
        start = end + 1;
    }
    return parts;
}

ModelNode::Object* ensureObject(ModelNode& node)
{
    if (!node.isObject()) {
        node = ModelNode::object({});
    }
    return const_cast<ModelNode::Object*>(node.objectValue());
}

ModelNode::Array* ensureArray(ModelNode& node)
{
    if (!node.isArray()) {
        node = ModelNode::array({});
    }
    return const_cast<ModelNode::Array*>(node.arrayValue());
}

ModelNode* accessChildNode(
    ModelNode& node,
    std::string_view segment,
    std::optional<std::string_view> nextSegment)
{
    if (node.isObject()) {
        auto* object = ensureObject(node);
        ModelNode& child = (*object)[std::string(segment)];
        if (nextSegment.has_value() && child.isNull()) {
            child = isArrayIndexSegment(*nextSegment)
                ? ModelNode::array({})
                : ModelNode::object({});
        }
        return &child;
    }

    if (node.isArray()) {
        const std::optional<std::size_t> index = parseArrayIndex(segment);
        if (!index.has_value()) {
            return nullptr;
        }

        auto* array = ensureArray(node);
        if (array->size() <= *index) {
            array->resize(*index + 1);
        }

        ModelNode& child = (*array)[*index];
        if (nextSegment.has_value() && child.isNull()) {
            child = isArrayIndexSegment(*nextSegment)
                ? ModelNode::array({})
                : ModelNode::object({});
        }
        return &child;
    }

    if (isArrayIndexSegment(segment)) {
        node = ModelNode::array({});
    } else {
        node = ModelNode::object({});
    }

    return accessChildNode(node, segment, nextSegment);
}

const ModelNode* findChildNode(const ModelNode& node, std::string_view segment)
{
    if (node.isObject()) {
        return node.child(segment);
    }

    if (!node.isArray()) {
        return nullptr;
    }

    const std::optional<std::size_t> index = parseArrayIndex(segment);
    if (!index.has_value()) {
        return nullptr;
    }

    const ModelNode::Array* array = node.arrayValue();
    if (array == nullptr || *index >= array->size()) {
        return nullptr;
    }

    return &(*array)[*index];
}

} // namespace

ModelNode::ModelNode(core::Value scalar)
    : kind_(Kind::Scalar)
    , data_(std::move(scalar))
{
}

ModelNode ModelNode::object(Object object)
{
    ModelNode node;
    node.kind_ = Kind::Object;
    node.data_ = std::move(object);
    return node;
}

ModelNode ModelNode::array(Array array)
{
    ModelNode node;
    node.kind_ = Kind::Array;
    node.data_ = std::move(array);
    return node;
}

const core::Value* ModelNode::scalar() const
{
    return isScalar() ? &std::get<core::Value>(data_) : nullptr;
}

const ModelNode::Object* ModelNode::objectValue() const
{
    return isObject() ? &std::get<Object>(data_) : nullptr;
}

const ModelNode::Array* ModelNode::arrayValue() const
{
    return isArray() ? &std::get<Array>(data_) : nullptr;
}

const ModelNode* ModelNode::child(std::string_view name) const
{
    const Object* object = objectValue();
    if (object == nullptr) {
        return nullptr;
    }

    const auto it = object->find(std::string(name));
    return it != object->end() ? &it->second : nullptr;
}

std::shared_ptr<ViewModel> ViewModel::create()
{
    return std::make_shared<ViewModel>();
}

bool ViewModel::setNode(std::string_view path, ModelNode node)
{
    const std::string normalizedPath = normalizeBindingPath(path);
    if (normalizedPath.empty()) {
        return false;
    }

    const std::vector<std::string_view> segments = splitPath(normalizedPath);
    if (segments.empty()) {
        return false;
    }

    ModelNode* targetNode = &root_;
    for (std::size_t index = 0; index + 1 < segments.size(); ++index) {
        targetNode = accessChildNode(
            *targetNode,
            segments[index],
            segments[index + 1]);
        if (targetNode == nullptr) {
            return false;
        }
    }

    ModelNode* storedNode = accessChildNode(*targetNode, segments.back(), std::nullopt);
    if (storedNode == nullptr) {
        return false;
    }

    if (nodeEquals(*storedNode, node)) {
        return false;
    }

    *storedNode = std::move(node);

    std::vector<ChangeCallback> valueCallbacks;
    for (const auto& [listenerId, listener] : valueListeners_) {
        (void)listenerId;
        if (!listener.callback || !pathMatchesOrContains(listener.path, normalizedPath)) {
            continue;
        }

        const core::Value* resolvedValue = findValue(listener.path);
        if (resolvedValue != nullptr) {
            valueCallbacks.push_back([callback = listener.callback, value = *resolvedValue](const core::Value&) {
                callback(value);
            });
        }
    }

    std::vector<InvalidationCallback> invalidationCallbacks;
    for (const auto& [listenerId, listener] : invalidationListeners_) {
        (void)listenerId;
        if (listener.callback && pathMatchesOrContains(listener.path, normalizedPath)) {
            invalidationCallbacks.push_back(listener.callback);
        }
    }

    for (const auto& callback : valueCallbacks) {
        callback(core::Value());
    }
    for (const auto& callback : invalidationCallbacks) {
        callback();
    }

    return true;
}

const ModelNode* ViewModel::findNode(std::string_view path) const
{
    const std::string normalizedPath = normalizeBindingPath(path);
    if (normalizedPath.empty()) {
        return &root_;
    }

    const ModelNode* currentNode = &root_;
    for (std::string_view segment : splitPath(normalizedPath)) {
        if (segment.empty()) {
            return nullptr;
        }

        currentNode = findChildNode(*currentNode, segment);
        if (currentNode == nullptr) {
            return nullptr;
        }
    }

    return currentNode;
}

bool ViewModel::setArray(std::string_view path, Array array)
{
    return setNode(path, ModelNode::array(std::move(array)));
}

bool ViewModel::setObject(std::string_view path, Object object)
{
    return setNode(path, ModelNode::object(std::move(object)));
}

bool ViewModel::setValue(std::string_view path, const core::Value& value)
{
    return setNode(path, ModelNode(value));
}

const core::Value* ViewModel::findValue(std::string_view path) const
{
    const ModelNode* node = findNode(path);
    return node != nullptr ? node->scalar() : nullptr;
}

bool ViewModel::setString(std::string_view path, std::string value)
{
    return setValue(path, core::Value(std::move(value)));
}

bool ViewModel::setBool(std::string_view path, bool value)
{
    return setValue(path, core::Value(value));
}

bool ViewModel::setInt(std::string_view path, int value)
{
    return setValue(path, core::Value(value));
}

bool ViewModel::setFloat(std::string_view path, float value)
{
    return setValue(path, core::Value(value));
}

bool ViewModel::setColor(std::string_view path, core::Color value)
{
    return setValue(path, core::Value(value));
}

bool ViewModel::setEnum(std::string_view path, std::string name)
{
    return setValue(path, core::Value::enumValue(std::move(name)));
}

ViewModel::ListenerId ViewModel::addListener(
    std::string_view path,
    ChangeCallback callback)
{
    const ListenerId listenerId = nextListenerId_++;
    valueListeners_[listenerId] = ValueListenerEntry {
        .path = normalizeBindingPath(path),
        .callback = std::move(callback),
    };
    return listenerId;
}

ViewModel::ListenerId ViewModel::addInvalidationListener(
    std::string_view path,
    InvalidationCallback callback)
{
    const ListenerId listenerId = nextListenerId_++;
    invalidationListeners_[listenerId] = InvalidationListenerEntry {
        .path = normalizeBindingPath(path),
        .callback = std::move(callback),
    };
    return listenerId;
}

void ViewModel::removeListener(ListenerId id)
{
    valueListeners_.erase(id);
    invalidationListeners_.erase(id);
}

} // namespace tinalux::markup
