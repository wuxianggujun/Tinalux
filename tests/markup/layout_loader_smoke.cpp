#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "tinalux/core/Geometry.h"
#include "tinalux/markup/LayoutLoader.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Checkbox.h"
#include "tinalux/ui/Dialog.h"
#include "tinalux/ui/Dropdown.h"
#include "tinalux/ui/ImageWidget.h"
#include "tinalux/ui/Label.h"
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
component RememberOption(label: "", checked: false): Checkbox(label, checked)
VBox(id: root, 12, 8) {
    Button(id: cta, "Deploy", res("assets/icons/ship.png")),
    Checkbox(id: remember, "Remember me", true),
    Dialog(id: confirmDialog, "Confirm", 18) {
        Panel(id: confirmBody)
    },
    ImageWidget(id: hero, res("assets/images/hero.png"), Cover, 0.75),
    RememberOption(id: componentRemember, "Use biometrics", true)
}
)";

        const markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "anonymous property syntax should load");
        expect(result.warnings.empty(), "anonymous property syntax should not emit warnings");

        const ui::VBox* root = dynamic_cast<ui::VBox*>(result.handle.root().get());
        expect(root != nullptr, "VBox shorthand should materialize VBox root");
        expect(nearlyEqual(root->spacing(), 12.0f), "VBox first positional arg should map to spacing");
        expect(nearlyEqual(root->padding(), 8.0f), "VBox second positional arg should map to padding");

        const ui::Button* cta = result.handle.findById<ui::Button>("cta");
        expect(cta != nullptr, "Button shorthand should materialize Button widget");
        expect(
            cta->iconPath() == "assets/icons/ship.png",
            "Button second positional arg should map to icon");

        const ui::Checkbox* remember = result.handle.findById<ui::Checkbox>("remember");
        expect(remember != nullptr, "Checkbox shorthand should materialize Checkbox widget");
        expect(remember->label() == "Remember me", "Checkbox shorthand should map to label");
        expect(remember->checked(), "Checkbox shorthand should map trailing bool positional argument");

        const ui::Dialog* confirmDialog = result.handle.findById<ui::Dialog>("confirmDialog");
        expect(confirmDialog != nullptr, "Dialog shorthand should materialize Dialog widget");
        expect(confirmDialog->title() == "Confirm", "Dialog first positional arg should map to title");
        expect(confirmDialog->content() != nullptr, "Dialog shorthand should keep child content");
        expect(confirmDialog->content()->id() == "confirmBody", "Dialog shorthand should preserve content child");

        const ui::ImageWidget* hero = result.handle.findById<ui::ImageWidget>("hero");
        expect(hero != nullptr, "ImageWidget shorthand should materialize ImageWidget");
        expect(hero->imagePath() == "assets/images/hero.png", "ImageWidget first positional arg should map to source");
        expect(hero->fit() == ui::ImageFit::Cover, "ImageWidget second positional arg should map to fit");
        expect(nearlyEqual(hero->opacity(), 0.75f), "ImageWidget third positional arg should map to opacity");

        const ui::Checkbox* componentRemember = result.handle.findById<ui::Checkbox>("componentRemember");
        expect(componentRemember != nullptr, "multi-parameter component shorthand should resolve");
        expect(componentRemember->label() == "Use biometrics", "component first positional arg should map to first parameter");
        expect(componentRemember->checked(), "component second positional arg should map to second parameter");
    }

    {
        const std::string source = R"(
VBox(id: root) {
    Button(id: cta, text: "Ship", icon: res("assets/icons/ship.png")),
    TextInput(id: query, placeholder: "Search", leadingIcon: res("assets/icons/search.png")),
    ImageWidget(id: hero, source: res("assets/images/hero.png"), fit: Cover, opacity: 0.75)
}
)";

        const markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "res should work in inline markup");
        expect(result.warnings.empty(), "inline res smoke should not emit warnings");

        const ui::Button* button = result.handle.findById<ui::Button>("cta");
        expect(button != nullptr, "Button with res icon should be discoverable by id");
        expect(
            button->iconPath() == "assets/icons/ship.png",
            "inline res should preserve relative path text");

        const ui::TextInput* input = result.handle.findById<ui::TextInput>("query");
        expect(input != nullptr, "TextInput with res icon should be discoverable by id");
        expect(
            input->leadingIconPath() == "assets/icons/search.png",
            "inline res should preserve TextInput icon path");

        const ui::ImageWidget* hero = result.handle.findById<ui::ImageWidget>("hero");
        expect(hero != nullptr, "ImageWidget should be constructible from markup");
        expect(
            hero->imagePath() == "assets/images/hero.png",
            "inline res should feed ImageWidget source");
        expect(hero->fit() == ui::ImageFit::Cover, "ImageWidget fit should be parsed");
        expect(nearlyEqual(hero->opacity(), 0.75f), "ImageWidget opacity should be parsed");
    }

    {
        const std::string source = R"(
style primaryAction: Button(
    backgroundColor: #FF336699,
    textColor: #FFF5F7FA,
    paddingHorizontal: 20,
    paddingVertical: 12
)
style searchFieldBase: TextInput(
    backgroundColor: #FFF0F4F8,
    borderColor: #FF336699,
    borderRadius: 12
)
style searchField: TextInput(
    style: searchFieldBase,
    borderRadius: 18,
    minWidth: 320
)
VBox(id: root) {
    Button(id: cta, text: "Ship", style: {
        style: primaryAction,
        borderRadius: 24
    }),
    TextInput(id: query, placeholder: "Search", style: searchField),
    Dropdown(id: picker, placeholder: "Pick one", maxVisibleItems: 7, selectedIndex: -1)
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
            "derived TextInput style should override borderRadius");
        expect(
            nearlyEqual(input->style()->minWidth, 320.0f),
            "derived TextInput style should override minWidth");

        const ui::Dropdown* dropdown = result.handle.findById<ui::Dropdown>("picker");
        expect(dropdown != nullptr, "Dropdown should be constructible from markup");
        expect(dropdown->placeholder() == "Pick one", "Dropdown placeholder should be parsed");
        expect(dropdown->maxVisibleItems() == 7, "Dropdown maxVisibleItems should be parsed");
        expect(dropdown->selectedIndex() == -1, "Dropdown selectedIndex should be parsed");
    }

    {
        const std::filesystem::path tempRoot =
            std::filesystem::temp_directory_path() / "tinalux_markup_layout_loader_smoke";
        std::filesystem::remove_all(tempRoot);
        std::filesystem::create_directories(tempRoot);

        const std::filesystem::path sharedPath = tempRoot / "components" / "shared.tui";
        const std::filesystem::path layoutPath = tempRoot / "layout.tui";

        {
            std::filesystem::create_directories(sharedPath.parent_path());
            std::filesystem::create_directories(sharedPath.parent_path() / "icons");
            std::ofstream shared(sharedPath);
            shared << R"(
style importedPanel: Panel(
    backgroundColor: #FF102030,
    cornerRadius: 14
)
component SharedSearchButton: Button(text: "Search", icon: res("icons/search.png"))
component SearchCard: Panel(style: importedPanel) {
    Label(text: "Shared title")
}
component SharedPicker: Dropdown(placeholder: "Shared choice", maxVisibleItems: 4, selectedIndex: -1)
)";
        }

        {
            std::ofstream layout(layoutPath);
            layout << R"(
import "components/shared.tui"
VBox(id: root) {
    Panel(id: card, style: importedPanel),
    SharedSearchButton(id: searchButton),
    SearchCard(id: searchCard),
    SharedPicker(id: sharedPicker),
    SharedPicker(id: sharedPickerOverride, placeholder: "Overridden choice", maxVisibleItems: 9),
    Dialog(id: dialog, title: "Hello", padding: 18) {
        Panel(id: dialogBody)
    },
    ScrollView(id: scroll, preferredHeight: 180) {
        Panel(id: scrollContent)
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

        const ui::Button* sharedSearchButton = result.handle.findById<ui::Button>("searchButton");
        expect(sharedSearchButton != nullptr, "imported component with res icon should resolve to Button root");
        const std::string expectedImportedIconPath = std::filesystem::weakly_canonical(
            sharedPath.parent_path() / "icons" / "search.png").generic_string();
        const std::string actualImportedIconPath = std::filesystem::weakly_canonical(
            std::filesystem::path(sharedSearchButton->iconPath())).generic_string();
        if (actualImportedIconPath != expectedImportedIconPath) {
            std::cerr
                << "imported component res path should resolve relative to defining file\n"
                << "expected: " << expectedImportedIconPath << '\n'
                << "actual:   " << actualImportedIconPath << '\n';
            std::exit(1);
        }

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

        std::filesystem::remove_all(tempRoot);
    }

    {
        const std::string source = R"(
component FormField(placeholder: "Base placeholder", maxVisibleItems: 5): Dropdown(
    placeholder: placeholder,
    maxVisibleItems: maxVisibleItems,
    selectedIndex: -1
)
VBox(id: root) {
    FormField(id: username),
    FormField(id: email, placeholder: "Email address", maxVisibleItems: 8)
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

    {
        const std::string source = R"(
component AccentButton(accent: #FF336699): Button(
    text: "Styled",
    style: {
        backgroundColor: accent,
        textColor: #FFF5F7FA,
        borderRadius: 14
    }
)
VBox(id: root) {
    AccentButton(id: primary),
    AccentButton(id: danger, accent: #FFC0392B)
}
)";

        const markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "component parameters inside inline style should load");
        expect(result.warnings.empty(), "component inline style smoke should not emit warnings");

        const ui::Button* primary = result.handle.findById<ui::Button>("primary");
        expect(primary != nullptr, "inline style component should materialize as Button");
        expect(primary->style() != nullptr, "inline style component should install a custom style");
        expect(
            primary->style()->backgroundColor.normal == core::colorARGB(255, 51, 102, 153),
            "component default inline style color should resolve");

        const ui::Button* danger = result.handle.findById<ui::Button>("danger");
        expect(danger != nullptr, "inline style component override should materialize as Button");
        expect(danger->style() != nullptr, "inline style component override should install a custom style");
        expect(
            danger->style()->backgroundColor.normal == core::colorARGB(255, 192, 57, 43),
            "component parameter override should flow into inline style block");
        expect(
            nearlyEqual(danger->style()->borderRadius, 14.0f),
            "inline style block should preserve non-parameter properties");
    }

    {
        const std::string source = R"(
component CardShell: Panel(id: shell) {
    Slot()
}
CardShell(id: card) {
    Button(id: inside, text: "OK")
}
)";

        const markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "component default slot should load");
        expect(result.warnings.empty(), "component default slot smoke should not emit warnings");

        const ui::Panel* card = result.handle.findById<ui::Panel>("card");
        expect(card != nullptr, "default slot component should materialize as Panel");
        expect(card->children().size() == 1, "default slot content should attach to Panel");
        expect(card->children().front()->id() == "inside", "default slot child id should be preserved");
    }

    {
        const std::string source = R"(
component StaticShell: Panel(id: shell)
StaticShell(id: card) {
    Button(id: inside, text: "OK")
}
)";

        const markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "component without slot should still load");
        expect(result.warnings.size() == 1, "component without slot children should emit one warning");
        expect(
            result.warnings.front().find("declares no Slot()") != std::string::npos,
            "component without slot warning should explain missing Slot");

        const ui::Panel* card = result.handle.findById<ui::Panel>("card");
        expect(card != nullptr, "component without slot should materialize as Panel");
        expect(card->children().empty(), "component without slot should ignore instance children");
        expect(
            result.handle.findById<ui::Button>("inside") == nullptr,
            "ignored component child should not materialize");
    }

    {
        const std::string source = R"(
component DialogFrame: Panel(id: frame) {
    Panel(id: bodyHost) {
        Slot(name: body)
    },
    Panel(id: footerHost) {
        Slot(name: "footer")
    }
}
DialogFrame(id: dialog) {
    Panel(slot: body, id: content),
    Button(slot: footer, id: ok, text: "OK")
}
)";

        const markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "component named slots should load");
        expect(result.warnings.empty(), "component named slot smoke should not emit warnings");

        const ui::Panel* dialog = result.handle.findById<ui::Panel>("dialog");
        expect(dialog != nullptr, "named slot component should materialize as Panel");
        expect(dialog->children().size() == 2, "named slot hosts should remain in template tree");

        const ui::Panel* bodyHost = result.handle.findById<ui::Panel>("bodyHost");
        expect(bodyHost != nullptr, "body slot host should exist");
        expect(bodyHost->children().size() == 1, "body slot should project one child");
        expect(bodyHost->children().front()->id() == "content", "body slot child id should be preserved");

        const ui::Panel* footerHost = result.handle.findById<ui::Panel>("footerHost");
        expect(footerHost != nullptr, "footer slot host should exist");
        expect(footerHost->children().size() == 1, "footer slot should project one child");
        expect(footerHost->children().front()->id() == "ok", "footer slot child id should be preserved");
    }

    {
        const std::string source = R"(
component FooterShell: Panel(id: shell) {
    Slot(name: footer) {
        Button(id: fallbackBtn, text: "Cancel")
    }
}
FooterShell(id: withoutOverride)
)";

        const markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "component slot fallback should load");
        expect(result.warnings.empty(), "component slot fallback smoke should not emit warnings");

        const ui::Panel* shell = result.handle.findById<ui::Panel>("withoutOverride");
        expect(shell != nullptr, "slot fallback component should materialize as Panel");
        expect(shell->children().size() == 1, "slot fallback child should be attached");
        expect(shell->children().front()->id() == "fallbackBtn", "slot fallback child id should be preserved");
    }

    {
        const std::string source = R"(
VBox(id: root) {
    Dialog(id: dialog, title: "Warn") {
        Panel(id: dialogFirst),
        Panel(id: dialogSecond)
    },
    ScrollView(id: scroll) {
        Panel(id: scrollFirst),
        Panel(id: scrollSecond)
    },
    Button(id: invalid, text: "Oops") {
        Label(id: nested, text: "Should not exist")
    }
}
)";

        const markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expectLoadOk(result, "attachment warnings markup should still load");
        expect(result.warnings.size() == 3, "attachment warnings smoke should emit three warnings");
        expect(
            std::any_of(
                result.warnings.begin(),
                result.warnings.end(),
                [](const std::string& warning) {
                    return warning.find("'Dialog' accepts only one child content node") != std::string::npos;
                }),
            "Dialog with multiple children should emit a single-content warning");
        expect(
            std::any_of(
                result.warnings.begin(),
                result.warnings.end(),
                [](const std::string& warning) {
                    return warning.find("'ScrollView' accepts only one child content node") != std::string::npos;
                }),
            "ScrollView with multiple children should emit a single-content warning");
        expect(
            std::any_of(
                result.warnings.begin(),
                result.warnings.end(),
                [](const std::string& warning) {
                    return warning.find("'Button' is not a container but has children") != std::string::npos;
                }),
            "non-container child warning should remain intact");

        const ui::Dialog* dialog = result.handle.findById<ui::Dialog>("dialog");
        expect(dialog != nullptr, "Dialog warning case should still build");
        expect(dialog->content() != nullptr, "Dialog should keep the first content child");
        expect(dialog->content()->id() == "dialogFirst", "Dialog should attach only the first child");

        const ui::ScrollView* scrollView = result.handle.findById<ui::ScrollView>("scroll");
        expect(scrollView != nullptr, "ScrollView warning case should still build");
        expect(scrollView->content() != nullptr, "ScrollView should keep the first content child");
        expect(
            scrollView->content()->id() == "scrollFirst",
            "ScrollView should attach only the first child");

        expect(
            result.handle.findById<ui::Label>("nested") == nullptr,
            "non-container children should not materialize");
    }

    {
        const std::string source = R"(
VBox(id: root) {
    Button(id: "legacyButton", text: "Legacy")
}
)";

        const markup::LoadResult result = markup::LayoutLoader::load(source, theme);
        expect(!result.ok(), "string id markup should be rejected");
        expect(
            std::any_of(
                result.errors.begin(),
                result.errors.end(),
                [](const std::string& error) {
                    return error.find("property 'id' expects a bare identifier or binding expression")
                        != std::string::npos;
                }),
            "string id parse error should explain identifier-only id syntax");
    }

    return 0;
}
