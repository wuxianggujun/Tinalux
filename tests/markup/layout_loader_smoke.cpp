#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "tinalux/core/Geometry.h"
#include "tinalux/markup/LayoutLoader.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Dialog.h"
#include "tinalux/ui/Dropdown.h"
#include "tinalux/ui/Panel.h"
#include "tinalux/ui/ScrollView.h"
#include "tinalux/ui/TextInput.h"
#include "tinalux/ui/Theme.h"
#include "tinalux/ui/VBox.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool nearlyEqual(float lhs, float rhs)
{
    return std::abs(lhs - rhs) <= 0.001f;
}

void expectLoadOk(
    const tinalux::markup::LoadResult& result,
    const char* context)
{
    if (result.ok()) {
        return;
    }

    std::cerr << context << '\n';
    for (const auto& error : result.errors) {
        std::cerr << "error: " << error << '\n';
    }
    std::exit(1);
}

} // namespace

int main()
{
    using namespace tinalux;

    const ui::Theme theme = ui::Theme::light();

    {
        const std::string source = R"(
@style primaryAction: Button(
    backgroundColor: #FF336699,
    textColor: #FFF5F7FA,
    borderRadius: 24,
    paddingHorizontal: 20,
    paddingVertical: 12
)
@style searchField: TextInput(
    backgroundColor: #FFF0F4F8,
    borderColor: #FF336699,
    borderRadius: 18,
    minWidth: 320
)
VBox(id: "root") {
    Button(id: "cta", text: "Ship", style: primaryAction),
    TextInput(id: "query", placeholder: "Search", style: searchField),
    Dropdown(id: "picker", placeholder: "Pick one", maxVisibleItems: 7, selectedIndex: -1)
}
)";

        const markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "inline markup with named styles should load");
        expect(result.warnings.empty(), "inline markup smoke should not emit warnings");
        expect(dynamic_cast<ui::VBox*>(result.handle.root().get()) != nullptr, "root should be VBox");

        const ui::Button* button = result.handle.findById<ui::Button>("cta");
        expect(button != nullptr, "styled Button should be discoverable by id");
        expect(button->style() != nullptr, "named Button style should install a custom style");
        expect(
            button->style()->backgroundColor.normal == core::colorARGB(255, 51, 102, 153),
            "named Button style should override backgroundColor");
        expect(
            button->style()->textColor.normal == core::colorARGB(255, 245, 247, 250),
            "named Button style should override textColor");
        expect(
            nearlyEqual(button->style()->borderRadius, 24.0f),
            "named Button style should override borderRadius");
        expect(
            nearlyEqual(button->style()->paddingHorizontal, 20.0f),
            "named Button style should override paddingHorizontal");

        const ui::TextInput* input = result.handle.findById<ui::TextInput>("query");
        expect(input != nullptr, "styled TextInput should be discoverable by id");
        expect(input->style() != nullptr, "named TextInput style should install a custom style");
        expect(
            input->style()->backgroundColor.normal == core::colorARGB(255, 240, 244, 248),
            "named TextInput style should override backgroundColor");
        expect(
            input->style()->borderColor.normal == core::colorARGB(255, 51, 102, 153),
            "named TextInput style should override borderColor");
        expect(
            nearlyEqual(input->style()->borderRadius, 18.0f),
            "named TextInput style should override borderRadius");
        expect(
            nearlyEqual(input->style()->minWidth, 320.0f),
            "named TextInput style should override minWidth");

        const ui::Dropdown* dropdown = result.handle.findById<ui::Dropdown>("picker");
        expect(dropdown != nullptr, "Dropdown should be constructible from markup");
        expect(dropdown->placeholder() == "Pick one", "Dropdown placeholder should be parsed");
        expect(dropdown->maxVisibleItems() == 7, "Dropdown maxVisibleItems should be parsed");
        expect(dropdown->selectedIndex() == -1, "Dropdown selectedIndex should be parsed");
    }

    {
        namespace fs = std::filesystem;

        const fs::path tempRoot = fs::temp_directory_path() / "tinalux_markup_layout_loader_smoke";
        fs::remove_all(tempRoot);
        fs::create_directories(tempRoot);

        const fs::path sharedPath = tempRoot / "shared.tui";
        const fs::path layoutPath = tempRoot / "layout.tui";

        {
            std::ofstream shared(sharedPath);
            shared << R"(
@style importedPanel: Panel(
    backgroundColor: #FF102030,
    cornerRadius: 14
)
@component SearchCard: Panel(style: importedPanel) {
    Label(text: "Shared title")
}
@component SharedPicker: Dropdown(placeholder: "Shared choice", maxVisibleItems: 4, selectedIndex: -1)
)";
        }

        {
            std::ofstream layout(layoutPath);
            layout << R"(
@import "shared.tui"
VBox(id: "root") {
    Panel(id: "card", style: importedPanel),
    SearchCard(id: "searchCard"),
    SharedPicker(id: "sharedPicker"),
    SharedPicker(id: "sharedPickerOverride", placeholder: "Overridden choice", maxVisibleItems: 9),
    Dialog(id: "dialog", title: "Hello", padding: 18) {
        Panel(id: "dialogBody")
    },
    ScrollView(id: "scroll", preferredHeight: 180) {
        Panel(id: "scrollContent")
    }
}
)";
        }

        const markup::LoadResult result = markup::LayoutLoader::loadFile(layoutPath.string(), theme);
        expectLoadOk(result, "file markup with imports should load");
        expect(result.warnings.empty(), "file markup smoke should not emit warnings");

        const ui::Panel* panel = result.handle.findById<ui::Panel>("card");
        expect(panel != nullptr, "imported style target panel should exist");
        expect(panel->style() != nullptr, "imported named Panel style should install a custom style");
        expect(
            panel->style()->backgroundColor == core::colorARGB(255, 16, 32, 48),
            "imported Panel style should override backgroundColor");
        expect(
            nearlyEqual(panel->style()->cornerRadius, 14.0f),
            "imported Panel style should override cornerRadius");

        const ui::Panel* importedCard = result.handle.findById<ui::Panel>("searchCard");
        expect(importedCard != nullptr, "imported component instance should resolve to Panel root");
        expect(importedCard->style() != nullptr, "imported component should carry imported named style");
        expect(
            importedCard->children().size() == 1,
            "component template children should be attached to imported component instance");

        const ui::Dropdown* importedPicker = result.handle.findById<ui::Dropdown>("sharedPicker");
        expect(
            importedPicker != nullptr,
            "imported component instance should resolve to Dropdown root");
        expect(
            importedPicker->placeholder() == "Shared choice",
            "imported component should preserve template placeholder");
        expect(
            importedPicker->maxVisibleItems() == 4,
            "imported component should preserve template maxVisibleItems");

        const ui::Dropdown* importedPickerOverride = result.handle.findById<ui::Dropdown>("sharedPickerOverride");
        expect(
            importedPickerOverride != nullptr,
            "component instance with override should resolve to Dropdown root");
        expect(
            importedPickerOverride->placeholder() == "Overridden choice",
            "component instance property should override imported template placeholder");
        expect(
            importedPickerOverride->maxVisibleItems() == 9,
            "component instance property should override imported template maxVisibleItems");

        const ui::Dialog* dialog = result.handle.findById<ui::Dialog>("dialog");
        expect(dialog != nullptr, "Dialog should be constructible from markup");
        expect(dialog->content() != nullptr, "Dialog child should be attached via setContent");
        expect(dialog->content()->id() == "dialogBody", "Dialog content id should be preserved");

        const ui::ScrollView* scrollView = result.handle.findById<ui::ScrollView>("scroll");
        expect(scrollView != nullptr, "ScrollView should be constructible from markup");
        expect(scrollView->content() != nullptr, "ScrollView child should be attached via setContent");
        expect(scrollView->content()->id() == "scrollContent", "ScrollView content id should be preserved");
        expect(
            nearlyEqual(scrollView->preferredHeight(), 180.0f),
            "ScrollView preferredHeight should be parsed");

        fs::remove_all(tempRoot);
    }

    {
        const std::string source = R"(
@component FormField: Dropdown(placeholder: "Base placeholder", maxVisibleItems: 5, selectedIndex: -1)
VBox(id: "root") {
    FormField(id: "username"),
    FormField(id: "email", placeholder: "Email address", maxVisibleItems: 8)
}
)";

        const markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "inline component definitions should load");
        expect(result.warnings.empty(), "component inline smoke should not emit warnings");

        const ui::Dropdown* username = result.handle.findById<ui::Dropdown>("username");
        expect(username != nullptr, "component instance should materialize as Dropdown");
        expect(
            username->placeholder() == "Base placeholder",
            "component materialization should preserve template placeholder");
        expect(
            username->maxVisibleItems() == 5,
            "component materialization should preserve template maxVisibleItems");

        const ui::Dropdown* email = result.handle.findById<ui::Dropdown>("email");
        expect(email != nullptr, "component override instance should materialize as Dropdown");
        expect(
            email->placeholder() == "Email address",
            "component instance should override template placeholder");
        expect(
            email->maxVisibleItems() == 8,
            "component instance should override template maxVisibleItems");
    }

    return 0;
}
