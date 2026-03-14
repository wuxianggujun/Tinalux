#include "tinalux/ui/DemoScene.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

#include "include/core/SkColor.h"

#include "tinalux/ui/Animation.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Checkbox.h"
#include "tinalux/ui/Label.h"
#include "tinalux/ui/Layout.h"
#include "tinalux/ui/ListView.h"
#include "tinalux/ui/ParagraphLabel.h"
#include "tinalux/ui/Panel.h"
#include "tinalux/ui/Radio.h"
#include "tinalux/ui/Slider.h"
#include "tinalux/ui/TextInput.h"
#include "tinalux/ui/Theme.h"
#include "tinalux/ui/Toggle.h"

namespace tinalux::ui {

namespace {

SkColor lerpColor(SkColor from, SkColor to, float progress)
{
    const float t = std::clamp(progress, 0.0f, 1.0f);
    const auto lerpChannel = [t](U8CPU a, U8CPU b) -> U8CPU {
        return static_cast<U8CPU>(static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * t);
    };

    return SkColorSetARGB(
        lerpChannel(SkColorGetA(from), SkColorGetA(to)),
        lerpChannel(SkColorGetR(from), SkColorGetR(to)),
        lerpChannel(SkColorGetG(from), SkColorGetG(to)),
        lerpChannel(SkColorGetB(from), SkColorGetB(to)));
}

}  // namespace

std::shared_ptr<Widget> createDemoScene()
{
    setTheme(Theme::dark());
    const Theme& theme = currentTheme();

    auto root = std::make_shared<Panel>();
    root->setBackgroundColor(theme.background);
    root->setCornerRadius(0.0f);

    auto rootLayout = std::make_unique<VBoxLayout>();
    rootLayout->padding = 36.0f;
    rootLayout->spacing = theme.spacing;
    root->setLayout(std::move(rootLayout));

    auto eyebrow = std::make_shared<Label>("TIN ALUX");
    eyebrow->setFontSize(13.0f);
    eyebrow->setColor(theme.primary);
    root->addChild(eyebrow);

    auto title = std::make_shared<Label>("Secure Login Demo");
    title->setFontSize(theme.fontSizeLarge + 6.0f);
    title->setColor(theme.text);
    root->addChild(title);

    auto subtitle = std::make_shared<Label>("Phase 5: ScrollView, ListView, richer workspace UI");
    subtitle->setFontSize(theme.fontSize);
    subtitle->setColor(theme.textSecondary);
    root->addChild(subtitle);

    auto loginCard = std::make_shared<Panel>();
    loginCard->setBackgroundColor(theme.surface);
    loginCard->setCornerRadius(theme.cornerRadius + 8.0f);
    auto cardLayout = std::make_unique<VBoxLayout>();
    cardLayout->padding = 24.0f;
    cardLayout->spacing = 14.0f;
    loginCard->setLayout(std::move(cardLayout));

    auto cardTitle = std::make_shared<Label>("Welcome back");
    cardTitle->setFontSize(theme.fontSizeLarge);
    auto cardBody = std::make_shared<ParagraphLabel>(
        "Sign in with your workspace email and password. "
        "The form now supports multi-line rich text layout and clipboard shortcuts.");
    cardBody->setColor(theme.textSecondary);
    cardBody->setMaxLines(3);

    auto emailCaption = std::make_shared<Label>("Email");
    emailCaption->setColor(theme.textSecondary);
    auto emailInput = std::make_shared<TextInput>("name@company.com");

    auto passwordCaption = std::make_shared<Label>("Password");
    passwordCaption->setColor(theme.textSecondary);
    auto passwordInput = std::make_shared<TextInput>("Enter your password");
    passwordInput->setObscured(true);

    auto hint = std::make_shared<ParagraphLabel>(
        "Tip: Tab moves focus. Use Ctrl+C / Ctrl+X / Ctrl+V inside the inputs, "
        "then move to Sign In and press Enter.");
    hint->setColor(theme.textSecondary);
    hint->setMaxLines(3);

    auto status = std::make_shared<Label>("Live");
    status->setColor(theme.textSecondary);

    auto actionRow = std::make_shared<Panel>();
    actionRow->setBackgroundColor(theme.background);
    actionRow->setCornerRadius(theme.cornerRadius);
    auto actionLayout = std::make_unique<HBoxLayout>();
    actionLayout->padding = 12.0f;
    actionLayout->spacing = 12.0f;
    actionRow->setLayout(std::move(actionLayout));

    auto loginButton = std::make_shared<Button>("Sign In");
    loginButton->onClick([emailInput, passwordInput, status, theme] {
        if (emailInput->text().empty() || passwordInput->text().empty()) {
            status->setText("Please enter both email and password.");
            status->setColor(SkColorSetRGB(250, 179, 135));
            return;
        }

        status->setText("Signing in as " + emailInput->text());
        status->setColor(theme.primary);
    });

    auto resetButton = std::make_shared<Button>("Clear");
    resetButton->onClick([emailInput, passwordInput, status, theme] {
        emailInput->setText("");
        passwordInput->setText("");
        status->setText("Form cleared");
        status->setColor(theme.textSecondary);
    });

    actionRow->addChild(loginButton);
    actionRow->addChild(resetButton);

    loginCard->addChild(cardTitle);
    loginCard->addChild(cardBody);
    loginCard->addChild(emailCaption);
    loginCard->addChild(emailInput);
    loginCard->addChild(passwordCaption);
    loginCard->addChild(passwordInput);
    loginCard->addChild(actionRow);
    loginCard->addChild(hint);
    loginCard->addChild(status);

    root->addChild(loginCard);

    auto controlsCard = std::make_shared<Panel>();
    controlsCard->setBackgroundColor(theme.surface);
    controlsCard->setCornerRadius(theme.cornerRadius + 4.0f);
    auto controlsLayout = std::make_unique<VBoxLayout>();
    controlsLayout->padding = 20.0f;
    controlsLayout->spacing = 12.0f;
    controlsCard->setLayout(std::move(controlsLayout));

    auto controlsTitle = std::make_shared<Label>("Workspace Controls");
    controlsTitle->setFontSize(theme.fontSizeLarge - 4.0f);
    auto controlsBody = std::make_shared<ParagraphLabel>(
        "Phase 5 controls now include checkbox and slider primitives for form toggles and range input.");
    controlsBody->setColor(theme.textSecondary);
    controlsBody->setMaxLines(2);

    auto revealPassword = std::make_shared<Checkbox>("Reveal password while typing");
    auto autoRefresh = std::make_shared<Toggle>("Auto refresh activity feed", true);
    auto feedHeightLabel = std::make_shared<Label>("Activity list height: 240 px");
    feedHeightLabel->setColor(theme.textSecondary);
    auto feedHeightSlider = std::make_shared<Slider>();
    feedHeightSlider->setRange(180.0f, 320.0f);
    feedHeightSlider->setStep(20.0f);
    feedHeightSlider->setValue(240.0f);
    auto standardReview = std::make_shared<Radio>("Standard review", "review-level", true);
    auto strictReview = std::make_shared<Radio>("Strict review", "review-level");

    auto controlsStatus = std::make_shared<Label>("Password reveal is off.");
    controlsStatus->setColor(theme.textSecondary);

    controlsCard->addChild(controlsTitle);
    controlsCard->addChild(controlsBody);
    controlsCard->addChild(revealPassword);
    controlsCard->addChild(autoRefresh);
    controlsCard->addChild(feedHeightLabel);
    controlsCard->addChild(feedHeightSlider);
    controlsCard->addChild(standardReview);
    controlsCard->addChild(strictReview);
    controlsCard->addChild(controlsStatus);
    root->addChild(controlsCard);

    auto feedTitle = std::make_shared<Label>("Recent Sign-In Activity");
    feedTitle->setFontSize(theme.fontSizeLarge - 2.0f);
    root->addChild(feedTitle);

    auto feedBody = std::make_shared<ParagraphLabel>(
        "Scroll to inspect recent sessions, devices, and review checks.");
    feedBody->setColor(theme.textSecondary);
    feedBody->setMaxLines(2);
    root->addChild(feedBody);

    auto activityList = std::make_shared<ListView>();
    activityList->setPreferredHeight(240.0f);
    activityList->setSpacing(10.0f);
    activityList->setPadding(10.0f);

    for (int index = 0; index < 12; ++index) {
        auto itemCard = std::make_shared<Panel>();
        itemCard->setBackgroundColor(index % 2 == 0 ? theme.surface : theme.background);
        itemCard->setCornerRadius(theme.cornerRadius);

        auto itemLayout = std::make_unique<VBoxLayout>();
        itemLayout->padding = 14.0f;
        itemLayout->spacing = 6.0f;
        itemCard->setLayout(std::move(itemLayout));

        auto itemTitle = std::make_shared<Label>(
            "Session #" + std::to_string(index + 1) + "  |  Windows 11");
        auto itemMeta = std::make_shared<Label>(
            "Shanghai Office  |  Review score " + std::to_string(92 - index));
        itemMeta->setColor(theme.textSecondary);
        auto itemHint = std::make_shared<ParagraphLabel>(
            index % 3 == 0
                ? "Multi-factor challenge completed successfully."
                : "No unusual risk detected in the last 24 hours.");
        itemHint->setColor(theme.textSecondary);
        itemHint->setMaxLines(2);

        itemCard->addChild(itemTitle);
        itemCard->addChild(itemMeta);
        itemCard->addChild(itemHint);
        activityList->addItem(itemCard);
    }

    root->addChild(activityList);

    const std::weak_ptr<TextInput> weakPasswordInput = passwordInput;
    const std::weak_ptr<Label> weakControlsStatus = controlsStatus;
    revealPassword->onToggle([weakPasswordInput, weakControlsStatus, theme](bool checked) {
        if (const auto input = weakPasswordInput.lock(); input != nullptr) {
            input->setObscured(!checked);
        }
        if (const auto label = weakControlsStatus.lock(); label != nullptr) {
            label->setText(checked ? "Password reveal is on." : "Password reveal is off.");
            label->setColor(checked ? theme.primary : theme.textSecondary);
        }
    });

    const std::weak_ptr<Label> weakControlsStatusForRefresh = controlsStatus;
    autoRefresh->onToggle([weakControlsStatusForRefresh, theme](bool on) {
        if (const auto label = weakControlsStatusForRefresh.lock(); label != nullptr) {
            label->setText(on
                ? "Auto refresh is on. Password reveal follows your checkbox choice."
                : "Auto refresh is off. The activity feed is now static.");
            label->setColor(on ? theme.primary : theme.textSecondary);
        }
    });

    const std::weak_ptr<Label> weakFeedHeightLabel = feedHeightLabel;
    const std::weak_ptr<ListView> weakActivityList = activityList;
    feedHeightSlider->onValueChanged([weakFeedHeightLabel, weakActivityList](float value) {
        const int height = static_cast<int>(std::lround(value));
        if (const auto label = weakFeedHeightLabel.lock(); label != nullptr) {
            label->setText("Activity list height: " + std::to_string(height) + " px");
        }
        if (const auto list = weakActivityList.lock(); list != nullptr) {
            list->setPreferredHeight(static_cast<float>(height));
        }
    });

    const std::weak_ptr<Label> weakControlsStatusForReview = controlsStatus;
    standardReview->onChanged([weakControlsStatusForReview, theme](bool selected) {
        if (!selected) {
            return;
        }
        if (const auto label = weakControlsStatusForReview.lock(); label != nullptr) {
            label->setText("Standard review keeps the login flow balanced.");
            label->setColor(theme.textSecondary);
        }
    });
    strictReview->onChanged([weakControlsStatusForReview, theme](bool selected) {
        if (!selected) {
            return;
        }
        if (const auto label = weakControlsStatusForReview.lock(); label != nullptr) {
            label->setText("Strict review raises the risk threshold for new sessions.");
            label->setColor(theme.primary);
        }
    });

    animate(
        {
            .from = 0.0f,
            .to = 1.0f,
            .durationSeconds = 1.2,
            .loop = true,
            .alternate = true,
            .easing = Easing::EaseInOut,
        },
        [status, theme](float value) {
            status->setColor(lerpColor(theme.textSecondary, theme.primary, value));
        });

    auto frameStart = std::make_shared<double>(animationNowSeconds());
    auto frameLoop = std::make_shared<std::function<void(double)>>();
    const std::weak_ptr<std::function<void(double)>> weakFrameLoop = frameLoop;
    *frameLoop = [feedBody, frameStart, weakFrameLoop](double nowSeconds) {
        const int dots = static_cast<int>(std::fmod((nowSeconds - *frameStart) * 2.0, 4.0));
        feedBody->setText("Scroll to inspect recent sessions, devices, and review checks"
            + std::string(static_cast<std::size_t>(dots), '.'));
        if (const auto loop = weakFrameLoop.lock(); loop != nullptr) {
            requestAnimationFrame(*loop);
        }
    };
    requestAnimationFrame(*frameLoop);

    return root;
}

}  // namespace tinalux::ui
