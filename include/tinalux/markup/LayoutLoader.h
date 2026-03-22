#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "tinalux/markup/Ast.h"
#include "tinalux/markup/ViewModel.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::ui {
struct Theme;
} // namespace tinalux::ui

namespace tinalux::markup {

class LayoutHandle;

namespace detail {

struct LayoutHandleState {
    LayoutHandle* owner = nullptr;
};

} // namespace detail

class LayoutHandle {
public:
    LayoutHandle();
    ~LayoutHandle();
    LayoutHandle(const LayoutHandle&) = delete;
    LayoutHandle& operator=(const LayoutHandle&) = delete;
    LayoutHandle(LayoutHandle&& other) noexcept;
    LayoutHandle& operator=(LayoutHandle&& other) noexcept;
    LayoutHandle(
        std::shared_ptr<ui::Widget> root,
        std::unordered_map<std::string, std::shared_ptr<ui::Widget>> idMap,
        std::vector<detail::BindingDescriptor> bindings,
        std::shared_ptr<AstDocument> documentTemplate,
        const ui::Theme& theme,
        std::vector<std::string> structuralPaths);

    std::shared_ptr<ui::Widget> root() const;
    void bindViewModel(const std::shared_ptr<ViewModel>& viewModel);
    std::shared_ptr<ViewModel> viewModel() const;

    template <typename W>
    W* findById(const std::string& id) const
    {
        auto it = idMap_.find(id);
        if (it == idMap_.end())
            return nullptr;
        return dynamic_cast<W*>(it->second.get());
    }

    void bindClick(const std::string& id, std::function<void()> handler);
    void bindDismiss(const std::string& id, std::function<void()> handler);
    void bindToggle(const std::string& id, std::function<void(bool)> handler);
    void bindTextChanged(const std::string& id, std::function<void(const std::string&)> handler);
    void bindLeadingIconClick(const std::string& id, std::function<void()> handler);
    void bindTrailingIconClick(const std::string& id, std::function<void()> handler);
    void bindValueChanged(const std::string& id, std::function<void(float)> handler);
    void bindScrollChanged(const std::string& id, std::function<void(float)> handler);
    void bindSelectionChanged(const std::string& id, std::function<void(int)> handler);

private:
    bool applyBindingNow(const detail::BindingDescriptor& binding);
    void clearValueListeners();
    void clearStructureListeners();
    void registerValueListeners();
    void registerStructureListeners();
    bool isWidgetBindingPath(std::string_view path) const;
    std::optional<ModelNode> readWidgetBindingNode(std::string_view path) const;
    bool bindingDependsOnWidgetPath(
        const detail::BindingDescriptor& binding,
        std::string_view path) const;
    void handleWidgetDependencyChange(std::string_view path);
    void bindInteraction(
        const std::string& id,
        std::string_view interactionName,
        core::InteractionHandler handler);
    void rebuildFromTemplate();
    void refreshInteractionBindings();
    void refreshWidgetInteractionBindings(ui::Widget& widget);
    void refreshInteractionBinding(
        ui::Widget& widget,
        const core::InteractionInfo& interaction);
    const detail::BindingDescriptor* findBinding(
        const ui::Widget* widget,
        std::string_view propertyName) const;

    std::shared_ptr<ui::Widget> root_;
    std::unordered_map<std::string, std::shared_ptr<ui::Widget>> idMap_;
    std::vector<detail::BindingDescriptor> bindings_;
    std::shared_ptr<AstDocument> documentTemplate_;
    std::shared_ptr<ui::Theme> theme_;
    std::vector<std::string> structuralPaths_;
    std::shared_ptr<ViewModel> viewModel_;
    std::vector<ViewModel::ListenerId> valueListenerIds_;
    std::vector<ViewModel::ListenerId> structureListenerIds_;
    bool rebuildInProgress_ = false;
    std::shared_ptr<std::uint64_t> bindingGeneration_ = std::make_shared<std::uint64_t>(0);
    std::shared_ptr<detail::LayoutHandleState> runtimeState_;
    std::unordered_map<std::string, std::unordered_map<std::string, core::InteractionHandler>>
        interactionHandlers_;
};

struct LoadResult {
    LayoutHandle handle;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    bool ok() const { return errors.empty(); }
};

class LayoutLoader {
public:
    static LoadResult load(std::string_view source, const ui::Theme& theme);
    static LoadResult loadFile(const std::string& path, const ui::Theme& theme);
};

} // namespace tinalux::markup
