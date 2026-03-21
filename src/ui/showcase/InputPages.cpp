#include "ShowcasePageFactories.h"

#include <cmath>
#include <memory>

#include "ShowcaseSupport.h"
#include "../RuntimeState.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Checkbox.h"
#include "tinalux/ui/Label.h"
#include "tinalux/ui/Layout.h"
#include "tinalux/ui/Panel.h"
#include "tinalux/ui/Radio.h"
#include "tinalux/ui/Slider.h"
#include "tinalux/ui/TextInput.h"
#include "tinalux/ui/Toggle.h"

namespace tinalux::ui::showcase {

using namespace support;

std::shared_ptr<Widget> createAuthFormPage(Theme theme)
{
    const rendering::Image userIcon = makeUserIcon(theme.colors.primary, 18.0f);
    const rendering::Image lockIcon = makeLockIcon(theme.colors.primary, 18.0f);
    const rendering::Image clearIcon = makeClearIcon(theme.secondaryTextColor(), 16.0f);
    const rendering::Image loginIcon = makeLockIcon(theme.colors.onPrimary, 16.0f);
    const rendering::Image clearButtonIcon = makeClearIcon(theme.colors.onPrimary, 16.0f);

    auto column = makePageColumn();
    auto loginCard = makeInfoCard(
        "Welcome back",
        "This page isolates text input, button, and validation interactions from the other showcase content.",
        theme);
    auto emailCaption = std::make_shared<Label>("Email");
    emailCaption->setColor(theme.secondaryTextColor());
    auto emailInput = std::make_shared<TextInput>("name@company.com");
    emailInput->setLeadingIcon(userIcon);
    emailInput->setTrailingIcon(clearIcon);
    auto passwordCaption = std::make_shared<Label>("Password");
    passwordCaption->setColor(theme.secondaryTextColor());
    auto passwordInput = std::make_shared<TextInput>("Enter your password");
    passwordInput->setObscured(true);
    passwordInput->setLeadingIcon(lockIcon);
    passwordInput->setTrailingIcon(clearIcon);
    auto status = std::make_shared<Label>("Live");
    status->setColor(theme.secondaryTextColor());

    auto actionRow = std::make_shared<Panel>();
    actionRow->setBackgroundColor(theme.colors.background);
    actionRow->setCornerRadius(theme.cornerRadius());
    auto actionLayout = std::make_unique<HBoxLayout>();
    actionLayout->padding = compactPadding(theme);
    actionLayout->spacing = compactSpacing(theme);
    actionRow->setLayout(std::move(actionLayout));

    auto loginButton = std::make_shared<Button>("Sign In");
    loginButton->setIcon(loginIcon);
    loginButton->setIconSize(core::Size::Make(16.0f, 16.0f));
    loginButton->onClick([emailInput, passwordInput, status, theme] {
        if (emailInput->text().empty() || passwordInput->text().empty()) {
            status->setText("Please enter both email and password.");
            status->setColor(core::colorRGB(250, 179, 135));
            return;
        }
        status->setText("Signing in as " + emailInput->text());
        status->setColor(theme.colors.primary);
    });

    auto clearButton = std::make_shared<Button>("Clear");
    clearButton->setIcon(clearButtonIcon);
    clearButton->setIconSize(core::Size::Make(16.0f, 16.0f));
    clearButton->onClick([emailInput, passwordInput, status, theme] {
        emailInput->setText("");
        passwordInput->setText("");
        status->setText("Form cleared");
        status->setColor(theme.secondaryTextColor());
    });
    actionRow->addChild(loginButton);
    actionRow->addChild(clearButton);

    loginCard->addChild(emailCaption);
    loginCard->addChild(emailInput);
    loginCard->addChild(passwordCaption);
    loginCard->addChild(passwordInput);
    loginCard->addChild(actionRow);
    loginCard->addChild(status);

    runtimeAnimationSink().animate(
        {
            .from = 0.0f,
            .to = 1.0f,
            .durationSeconds = 1.2,
            .loop = true,
            .alternate = true,
            .easing = Easing::EaseInOut,
        },
        [status, theme](float value) {
            status->setColor(lerpColor(theme.secondaryTextColor(), theme.colors.primary, value));
        });

    column->addChild(loginCard);
    column->addChild(makeInfoCard(
        "Editing Focus",
        "This page keeps text input, validation, and action buttons together so input-specific regressions stay easy to isolate.",
        theme));
    return column;
}

std::shared_ptr<Widget> createControlsPage(Theme theme)
{
    auto column = makePageColumn();
    auto controlsCard = makeInfoCard(
        "Workspace Controls",
        "Checkbox, toggle, radio, and slider widgets now live on their own page instead of sharing state with the login form.",
        theme);
    auto revealPassword = std::make_shared<Checkbox>("Reveal password while typing");
    auto autoRefresh = std::make_shared<Toggle>("Auto refresh activity feed", true);
    auto reviewA = std::make_shared<Radio>("Standard review", "review-level", true);
    auto reviewB = std::make_shared<Radio>("Strict review", "review-level");
    auto slider = std::make_shared<Slider>();
    slider->setRange(180.0f, 320.0f);
    slider->setStep(20.0f);
    slider->setValue(240.0f);
    auto controlsStatus = std::make_shared<Label>("Password reveal is off.");
    controlsStatus->setColor(theme.secondaryTextColor());

    controlsCard->addChild(revealPassword);
    controlsCard->addChild(autoRefresh);
    controlsCard->addChild(slider);
    controlsCard->addChild(reviewA);
    controlsCard->addChild(reviewB);
    controlsCard->addChild(controlsStatus);

    revealPassword->onToggle([controlsStatus, theme](bool checked) {
        controlsStatus->setText(checked ? "Reveal password is on." : "Reveal password is off.");
        controlsStatus->setColor(checked ? theme.colors.primary : theme.secondaryTextColor());
    });
    autoRefresh->onToggle([controlsStatus, theme](bool on) {
        controlsStatus->setText(on ? "Auto refresh is on." : "Auto refresh is off.");
        controlsStatus->setColor(on ? theme.colors.primary : theme.secondaryTextColor());
    });
    reviewA->onChanged([controlsStatus, theme](bool selected) {
        if (selected) {
            controlsStatus->setText("Standard review keeps the interaction profile balanced.");
            controlsStatus->setColor(theme.secondaryTextColor());
        }
    });
    reviewB->onChanged([controlsStatus, theme](bool selected) {
        if (selected) {
            controlsStatus->setText("Strict review raises the interaction threshold.");
            controlsStatus->setColor(theme.colors.primary);
        }
    });
    slider->onValueChanged([controlsStatus](float value) {
        controlsStatus->setText("Slider updated to " + std::to_string(static_cast<int>(std::lround(value))) + " px.");
    });

    column->addChild(controlsCard);
    column->addChild(makeInfoCard(
        "State Isolation",
        "Selection controls now publish local state without piggybacking on the authentication page status label.",
        theme));
    return column;
}

}  // namespace tinalux::ui::showcase
