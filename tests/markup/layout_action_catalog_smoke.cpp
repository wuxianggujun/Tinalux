#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "tinalux/markup/LayoutActionCatalog.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

const tinalux::markup::ActionSlotInfo* findSlot(
    const tinalux::markup::ActionCatalogResult& catalog,
    std::string_view path)
{
    for (const auto& slot : catalog.slots) {
        if (slot.path == path) {
            return &slot;
        }
    }
    return nullptr;
}

const tinalux::markup::WidgetAccessInfo* findWidget(
    const tinalux::markup::ActionCatalogResult& catalog,
    std::string_view id)
{
    for (const auto& widget : catalog.widgets) {
        if (widget.id == id) {
            return &widget;
        }
    }
    return nullptr;
}

} // namespace

int main()
{
    using namespace tinalux;

    {
        const std::string source = R"(
component ActionRow(action: ${model.onApply}): HBox {
    Button(id: rowApplyButton, text: "Apply", onClick: action)
}

VBox {
    Button(id: applyButton, text: "Apply", onClick: ${model.onApply})
    Dialog(id: profileDialog, title: "Profile", onDismiss: ${model.profile.onClose}) {
        Label(text: "Profile")
    }
    Slider(id: sharedSlider, value: 0.0, onValueChanged: ${model.shared})
    Button(text: "Reset", onClick: ${model.shared})
    ActionRow(action: ${model.onApply})
}
)";

        const markup::ActionCatalogResult catalog =
            markup::LayoutActionCatalog::collect(source);
        expect(catalog.ok(), "collect(source) should succeed");
        expect(catalog.slots.size() == 3, "catalog should deduplicate action paths");

        const markup::ActionSlotInfo* applySlot = findSlot(catalog, "onApply");
        expect(applySlot != nullptr, "catalog should contain onApply");
        expect(applySlot->symbolName == "onApply", "top-level action should keep slot name");
        expect(
            applySlot->payloadType == core::ValueType::None && !applySlot->genericPayload,
            "button click should export void() slot");

        const markup::ActionSlotInfo* closeSlot = findSlot(catalog, "profile.onClose");
        expect(closeSlot != nullptr, "catalog should contain nested action path");
        expect(
            closeSlot->symbolName == "profile_onClose",
            "nested path should sanitize into an underscore slot name");
        expect(
            closeSlot->payloadType == core::ValueType::None && !closeSlot->genericPayload,
            "dismiss action should export void() slot");

        const markup::ActionSlotInfo* sharedSlot = findSlot(catalog, "shared");
        expect(sharedSlot != nullptr, "catalog should contain shared action path");
        expect(sharedSlot->genericPayload, "conflicting payload types should fall back to core::Value");
        expect(
            catalog.warnings.size() == 1,
            "payload conflict should surface a single warning");
        expect(catalog.widgets.size() == 4, "catalog should collect all id-bound widgets");

        const markup::WidgetAccessInfo* applyButton = findWidget(catalog, "applyButton");
        expect(applyButton != nullptr, "catalog should contain applyButton");
        expect(
            applyButton->cppTypeName == "::tinalux::ui::Button",
            "button ids should resolve to typed ui::Button access");

        const markup::WidgetAccessInfo* profileDialog = findWidget(catalog, "profileDialog");
        expect(profileDialog != nullptr, "catalog should contain profileDialog");
        expect(
            profileDialog->cppHeader == "tinalux/ui/Dialog.h",
            "dialog ids should export the matching widget header");

        const std::string header = markup::LayoutActionCatalog::emitHeader(
            catalog,
            {
                .includeGuard = "TINALUX_GENERATED_LOGIN_SLOTS_H",
                .slotNamespace = "demo::slots",
                .layoutFilePath = "ui/login.tui",
            });
        expect(
            header.find("#ifndef TINALUX_GENERATED_LOGIN_SLOTS_H") != std::string::npos,
            "generated header should include the requested include guard");
        expect(
            header.find("namespace demo::slots {") != std::string::npos,
            "generated header should wrap slots inside the requested namespace");
        expect(
            header.find("#include \"tinalux/markup/LayoutLoader.h\"") != std::string::npos,
            "generated header should include layout handle support");
        expect(
            header.find("#include \"tinalux/ui/Button.h\"") != std::string::npos,
            "generated header should include typed widget headers");
        expect(
            header.find("class Ui {") != std::string::npos,
            "generated header should provide a typed ui accessor wrapper");
        expect(
            header.find(
                "class ButtonProxy : public ::tinalux::markup::WidgetRef<::tinalux::ui::Button> {")
                != std::string::npos,
            "generated header should emit reusable typed widget proxy helpers on top of WidgetRef");
        expect(
            header.find("ButtonProxy applyButton {}") != std::string::npos,
            "generated header should emit typed widget proxy fields");
        expect(
            header.find("using Base = ::tinalux::markup::WidgetRef<::tinalux::ui::Button>;")
                != std::string::npos,
            "generated widget proxies should inherit common pointer-like access from WidgetRef");
        expect(
            header.find("attachUi(::tinalux::markup::LayoutHandle& handle)")
                != std::string::npos,
            "generated header should provide a ui attachment helper");
        expect(
            header.find("struct LoadedPage {") != std::string::npos,
            "generated header should provide a low-level loaded page aggregate");
        expect(
            header.find("const std::shared_ptr<::tinalux::markup::ViewModel>& model() const")
                != std::string::npos,
            "generated page should expose model() instead of a raw public viewModel field");
        expect(
            header.find("::tinalux::markup::SignalRef<void()> clicked {}")
                != std::string::npos,
            "generated ui proxies should keep signal fields for Qt-style signal access");
        expect(
            header.find("bool onClick(Handler&& handler) const") != std::string::npos
                && header.find("return clicked.connect(std::forward<Handler>(handler));")
                    != std::string::npos,
            "generated widget proxies should also expose direct event helper methods");
        expect(
            header.find("bool onClick(Object* object, Method method) const") != std::string::npos
                && header.find("return clicked.connect(object, method);")
                    != std::string::npos,
            "generated widget proxies should expose direct object member event helpers");
        expect(
            header.find(
                "LoadedPage load(const ::tinalux::ui::Theme& theme, const binding::Handlers& handlers)")
                != std::string::npos,
            "generated header should provide one-shot page loading for handlers");
        expect(
            header.find("page.layout = ::tinalux::markup::LayoutLoader::loadFile(\"ui/login.tui\", theme);")
                != std::string::npos,
            "generated header should bake the source layout path into the page loader");
        expect(
            header.find("namespace demo {") != std::string::npos,
            "generated header should provide a parent facade namespace when slot namespace ends with ::slots");
        expect(
            header.find("class Page {") != std::string::npos,
            "generated header should provide a facade page class");
        expect(
            header.find("explicit Page(slots::LoadedPage page)") != std::string::npos,
            "generated facade page should wrap the renamed low-level loaded page type");
        expect(
            header.find("Page(const ::tinalux::ui::Theme& theme)")
                != std::string::npos,
            "generated header should provide a facade page constructor for direct page loading");
        expect(
            header.find("const slots::binding::Handlers& handlers)")
                != std::string::npos,
            "generated header should provide a facade page constructor for handlers-based loading");
        expect(
            header.find("class SetupContext : public slots::Ui {") != std::string::npos,
            "generated header should expose a setup context that inherits the generated ui proxy");
        expect(
            header.find("SetupContext& ui;") != std::string::npos
                && header.find("::tinalux::markup::LoadResult& layout;") != std::string::npos
                && header.find("Page& page() const") != std::string::npos,
            "generated setup context should expose ui-style access plus page-level state");
        expect(
            header.find("requires (Setup&& candidate, SetupContext& context)")
                    != std::string::npos
                && header.find("SetupContext setupContext(*this);") != std::string::npos
                && header.find("std::forward<Setup>(setup)(setupContext);")
                    != std::string::npos,
            "generated header should route inline setup lambdas through the setup context");
        expect(
            header.find("using Handlers = slots::Handlers;") == std::string::npos,
            "facade namespace should not re-export low-level handlers");
        expect(
            header.find("using Ui = slots::Ui;") == std::string::npos,
            "facade namespace should not re-export low-level ui type");
        expect(
            header.find("namespace binding {") != std::string::npos,
            "generated header should group low-level binding helpers under a binding namespace");
        expect(
            header.find("struct Handlers {") != std::string::npos,
            "generated header should provide a generated handlers aggregate");
        expect(
            header.find("std::function<void()> onApply {}") != std::string::npos,
            "generated header should emit no-payload handler fields");
        expect(
            header.find("std::function<void(const ::tinalux::core::Value&)> shared {}")
                != std::string::npos,
            "generated header should emit payload handler fields");
        expect(
            header.find("class Actions {") != std::string::npos,
            "generated header should group raw action slots inside an actions object");
        expect(
            header.find("inline const Actions actions {}") != std::string::npos,
            "generated header should expose a shared generated actions object");
        expect(
            header.find("binding::makeViewModel") != std::string::npos,
            "generated page loading should use the namespaced binding helpers");
        expect(
            header.find("bind(::tinalux::markup::ViewModel& viewModel, const Handlers& handlers)")
                != std::string::npos,
            "generated binding namespace should provide a handlers binding helper");
        expect(
            header.find("makeViewModel(const Handlers& handlers)") != std::string::npos,
            "generated binding namespace should provide a handlers-based ViewModel factory");
        expect(
            header.find("Controller& controller)") == std::string::npos,
            "generated header should not keep old controller auto-bind compatibility");
        expect(
            header.find("ControllerBase") == std::string::npos,
            "generated header should no longer emit inheritance-based controller helpers");
        expect(
            header.find("ActionSlot<void()> onApply { \"onApply\" };") != std::string::npos,
            "generated header should emit void() slot declarations");
        expect(
            header.find("class profile_Group {") != std::string::npos,
            "generated header should emit nested action groups for dotted action paths");
        expect(
            header.find("ActionSlot<void()> onClose { \"profile.onClose\" };")
                != std::string::npos,
            "generated header should emit nested slot declarations inside action groups");
        expect(
            header.find("profile_Group profile {}") != std::string::npos,
            "generated header should expose nested action groups as fields");
        expect(
            header.find(
                "ActionSlot<void(const ::tinalux::core::Value&)> shared { \"shared\" };")
                != std::string::npos,
            "generated header should fall back to core::Value for conflicting payloads");
        expect(
            header.find("actions.") != std::string::npos,
            "generated helpers should reference action slots through the actions object");
        expect(
            header.find("class profile_Group {") != std::string::npos,
            "generated handlers should emit nested groups for dotted action paths");
        expect(
            header.find("std::function<void()> onClose {}") != std::string::npos,
            "generated handlers should emit nested action fields");
        expect(
            header.find("std::shared_ptr<::tinalux::markup::ViewModel> viewModel_ {}")
                != std::string::npos,
            "generated ui proxies should retain a view model handle for event binding");
        expect(
            header.find("std::shared_ptr<::tinalux::markup::ViewModel> viewModel {}")
                == std::string::npos,
            "generated page should not keep a redundant public viewModel field");
        expect(
            header.find("slots::Ui ui {}") != std::string::npos,
            "facade page should store the low-level ui type directly");
        expect(
            header.find("demo::load(const ::tinalux::ui::Theme& theme)") == std::string::npos,
            "facade namespace should not keep a redundant free load helper");

        const std::string scaffold = markup::LayoutActionCatalog::emitPageScaffold(
            catalog,
            {
                .includeGuard = "TINALUX_GENERATED_LOGIN_PAGE_H",
                .slotNamespace = "demo::slots",
                .markupHeaderInclude = "login.markup.h",
                .className = "LoginPage",
            });
        expect(
            scaffold.find("#include \"login.markup.h\"") != std::string::npos,
            "page scaffold should include the generated markup header");
        expect(
            scaffold.find("class LoginPage {") != std::string::npos,
            "page scaffold should emit the requested class name");
        expect(
            scaffold.find("demo::Page page {}") != std::string::npos,
            "page scaffold should hold the generated facade page directly");
        expect(
            scaffold.find("// Generated page scaffold for the markup Page + ui.xxx main path.")
                    != std::string::npos
                && scaffold.find("// 2. samples/markup/README.md") != std::string::npos
                && scaffold.find(
                       "// Low-level helpers like Handlers / slots::load / slots::actions are not the default starting point.")
                    != std::string::npos,
            "page scaffold should include top-level guidance that points readers back to the copy-first main path");
        expect(
            scaffold.find("setupUi(ui);") != std::string::npos,
            "page scaffold should centralize generated bindings inside a single setupUi helper");
        expect(
            scaffold.find("initUi(ui);") != std::string::npos
                && scaffold.find("bindUi(ui);") != std::string::npos,
            "page scaffold should split generated setup into initUi and bindUi sections");
        expect(
            scaffold.find("void initUi(Ui& ui)") != std::string::npos
                && scaffold.find("// Qt-style local aliases for direct widget access.")
                    != std::string::npos
                && scaffold.find("[[maybe_unused]] auto& applyButton = ui.applyButton;")
                    != std::string::npos
                && scaffold.find("[[maybe_unused]] auto& profileDialog = ui.profileDialog;")
                    != std::string::npos
                && scaffold.find("// Button") != std::string::npos
                && scaffold.find("// Dialog") != std::string::npos
                && scaffold.find("// Examples:") != std::string::npos
                && scaffold.find("// applyButton->setEnabled(false);") != std::string::npos
                && scaffold.find("// sharedSlider->setValue(42.0f);") != std::string::npos
                && scaffold.find("// profileDialog->setDismissOnEscape(false);")
                    != std::string::npos,
            "page scaffold should emit grouped Qt-style aliases with useful initialization examples");
        expect(
            scaffold.find("// Qt-style local aliases for direct event binding.")
                != std::string::npos
                && scaffold.find("[[maybe_unused]] auto& applyButton = ui.applyButton;")
                    != std::string::npos
                && scaffold.find("[[maybe_unused]] auto& sharedSlider = ui.sharedSlider;")
                    != std::string::npos,
            "page scaffold should emit direct widget aliases inside the binding section");
        expect(
            scaffold.find("applyButton.onClick([this] {")
                != std::string::npos,
            "page scaffold should bind button events inline through widget aliases");
        expect(
            scaffold.find("profileDialog.onDismiss([this] {")
                != std::string::npos,
            "page scaffold should bind non-click widget events inline through widget aliases");
        expect(
            scaffold.find(
                "sharedSlider.onValueChanged([this](const ::tinalux::core::Value& value) {")
                != std::string::npos,
            "page scaffold should bind payload events inline through widget aliases");
        expect(
            scaffold.find("template <typename Ui>") != std::string::npos
                && scaffold.find("void bindUi(Ui& ui)") != std::string::npos
                && scaffold.find("// Slider") != std::string::npos
                && scaffold.find("// TODO: handle event here.") != std::string::npos
                && scaffold.find("(void)value;") != std::string::npos,
            "page scaffold should emit an inline bindUi section with TODO lambdas");
        expect(
            scaffold.find("onSharedSliderValueChanged") == std::string::npos,
            "page scaffold should no longer explode into one empty handler method per event");
    }

    {
        const std::string groupedSource = R"(
VBox {
    TextInput(id: form__queryInput, text: ${model.query}, onTextChanged: ${model.onQueryChanged})
    Button(id: form__loginButton, text: "Login", onClick: ${model.onLoginClicked})
    Button(id: toolbar__actions__clearButton, text: "Clear", onClick: ${model.onClearClicked})
}
)";

        const markup::ActionCatalogResult groupedCatalog =
            markup::LayoutActionCatalog::collect(groupedSource);
        expect(groupedCatalog.ok(), "collect(source) should support grouped ids");

        const std::string groupedHeader = markup::LayoutActionCatalog::emitHeader(
            groupedCatalog,
            {
                .includeGuard = "TINALUX_GROUPED_UI_H",
                .slotNamespace = "grouped::slots",
                .layoutFilePath = "ui/grouped.tui",
            });
        expect(
            groupedHeader.find("class form_Group {") != std::string::npos,
            "grouped ids should emit a nested ui group type");
        expect(
            groupedHeader.find("form_Group form {}") != std::string::npos,
            "grouped ids should emit a top-level ui group field");
        expect(
            groupedHeader.find("ButtonProxy loginButton {}") != std::string::npos,
            "grouped ids should keep typed widget proxies inside the generated group");
        expect(
            groupedHeader.find("TextInputProxy queryInput {}") != std::string::npos,
            "grouped ids should keep sibling widgets under the same group");
        expect(
            groupedHeader.find("class toolbar_Group {") != std::string::npos,
            "grouped ids should support top-level nested groups");
        expect(
            groupedHeader.find("class actions_Group {") != std::string::npos,
            "grouped ids should support multi-level ui nesting");
        expect(
            groupedHeader.find("actions_Group actions {}") != std::string::npos,
            "grouped ids should expose child groups as fields");
        expect(
            groupedHeader.find("clearButton {}") != std::string::npos,
            "grouped ids should expose deep leaf widgets inside child groups");

        const std::string groupedScaffold = markup::LayoutActionCatalog::emitPageScaffold(
            groupedCatalog,
            {
                .slotNamespace = "grouped::slots",
                .markupHeaderInclude = "grouped.markup.h",
                .className = "GroupedPage",
            });
        expect(
            groupedScaffold.find(
                "[[maybe_unused]] auto& formQueryInput = ui.form.queryInput;")
                    != std::string::npos
                && groupedScaffold.find(
                    "formQueryInput.onTextChanged([this](std::string_view text) {")
                    != std::string::npos,
            "grouped ids should flatten into direct widget aliases inside the binding section");
        expect(
            groupedScaffold.find(
                "toolbarActionsClearButton.onClick([this] {")
                != std::string::npos,
            "page scaffold should preserve nested ui groups when binding through aliases");
        expect(
            groupedScaffold.find("void initUi(Ui& ui)") != std::string::npos
                && groupedScaffold.find("void bindUi(Ui& ui)") != std::string::npos
                && groupedScaffold.find(
                    "[[maybe_unused]] auto& formQueryInput = ui.form.queryInput;")
                    != std::string::npos
                && groupedScaffold.find(
                    "[[maybe_unused]] auto& toolbarActionsClearButton = ui.toolbar.actions.clearButton;")
                    != std::string::npos
                && groupedScaffold.find("// formQueryInput->setText(\"seed\");")
                    != std::string::npos,
            "grouped page scaffold should keep separate initUi and bindUi sections with direct aliases");
    }

    {
        const markup::ActionCatalogResult inlineImportCatalog =
            markup::LayoutActionCatalog::collect(R"(
import "shared.tui"

VBox {}
)");
        expect(!inlineImportCatalog.ok(), "collect(source) should reject imports");
        expect(
            !inlineImportCatalog.errors.empty()
                && inlineImportCatalog.errors.front()
                    == "inline markup source cannot use import; use collectFile instead",
            "collect(source) should point callers at collectFile for import support");
    }

    {
        const std::filesystem::path tempDir =
            std::filesystem::temp_directory_path() / "tinalux_markup_action_catalog_smoke";
        std::filesystem::remove_all(tempDir);
        std::filesystem::create_directories(tempDir);

        const std::filesystem::path sharedPath = tempDir / "shared.tui";
        {
            std::ofstream sharedFile(sharedPath);
            sharedFile << R"(component Toolbar(primaryAction: ${model.tools.onRun}): HBox {
    Button(text: "Run", onClick: primaryAction)
})";
        }

        const std::filesystem::path layoutPath = tempDir / "screen.tui";
        {
            std::ofstream layoutFile(layoutPath);
            layoutFile << R"(import "shared.tui"

VBox {
    Toolbar(primaryAction: ${model.tools.onRun})
    Dropdown(placeholder: "Pick", onSelectionChanged: ${model.tools.onSelect})
})";
        }

        const markup::ActionCatalogResult fileCatalog =
            markup::LayoutActionCatalog::collectFile(layoutPath.string());
        expect(fileCatalog.ok(), "collectFile should resolve imports");
        expect(fileCatalog.warnings.empty(), "collectFile should not emit warnings");
        expect(fileCatalog.slots.size() == 2, "collectFile should merge imported component slots");

        const markup::ActionSlotInfo* runSlot = findSlot(fileCatalog, "tools.onRun");
        expect(runSlot != nullptr, "collectFile should resolve component-translated button actions");
        expect(
            runSlot->symbolName == "tools_onRun"
                && runSlot->payloadType == core::ValueType::None
                && !runSlot->genericPayload,
            "button action imported from component should export void() slot");

        const markup::ActionSlotInfo* selectSlot = findSlot(fileCatalog, "tools.onSelect");
        expect(selectSlot != nullptr, "collectFile should resolve imported dropdown action");
        expect(
            selectSlot->payloadType == core::ValueType::Int && !selectSlot->genericPayload,
            "dropdown selection should export int payload slot");
    }

    return 0;
}
