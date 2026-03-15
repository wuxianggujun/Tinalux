#include "tinalux/ui/DemoScene.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "tinalux/rendering/rendering.h"
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

struct ActivityEntry {
    std::string title;
    std::string meta;
    std::string hint;
};

core::Color lerpColor(core::Color from, core::Color to, float progress)
{
    const float t = std::clamp(progress, 0.0f, 1.0f);
    const auto lerpChannel = [t](std::uint8_t a, std::uint8_t b) -> std::uint8_t {
        return static_cast<std::uint8_t>(
            static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * t);
    };

    return core::colorARGB(
        lerpChannel(core::colorAlpha(from), core::colorAlpha(to)),
        lerpChannel(core::colorRed(from), core::colorRed(to)),
        lerpChannel(core::colorGreen(from), core::colorGreen(to)),
        lerpChannel(core::colorBlue(from), core::colorBlue(to)));
}

std::string lowerAscii(std::string_view text)
{
    std::string lowered;
    lowered.reserve(text.size());
    for (const char ch : text) {
        lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return lowered;
}

bool matchesActivityEntry(const ActivityEntry& entry, std::string_view query)
{
    if (query.empty()) {
        return true;
    }

    const std::string loweredQuery = lowerAscii(query);
    const std::string haystack =
        lowerAscii(entry.title) + " " + lowerAscii(entry.meta) + " " + lowerAscii(entry.hint);
    return haystack.find(loweredQuery) != std::string::npos;
}

rendering::Image makeIcon(
    float sizeHint,
    const std::function<void(rendering::Canvas&, float)>& draw)
{
    const int side = std::max(16, static_cast<int>(std::lround(sizeHint)));
    const rendering::RenderSurface surface = rendering::createRasterSurface(side, side);
    if (!surface) {
        return {};
    }

    rendering::Canvas canvas = surface.canvas();
    if (!canvas) {
        return {};
    }

    canvas.clear(core::colorARGB(0, 0, 0, 0));
    draw(canvas, static_cast<float>(side));
    return surface.snapshotImage();
}

rendering::Image makeUserIcon(core::Color color, float sizeHint)
{
    return makeIcon(sizeHint, [color](rendering::Canvas& canvas, float side) {
        canvas.drawCircle(side * 0.5f, side * 0.34f, side * 0.15f, color);
        canvas.drawRoundRect(
            core::Rect::MakeXYWH(side * 0.2f, side * 0.56f, side * 0.6f, side * 0.18f),
            side * 0.1f,
            side * 0.1f,
            color);
    });
}

rendering::Image makeLockIcon(core::Color color, float sizeHint)
{
    return makeIcon(sizeHint, [color](rendering::Canvas& canvas, float side) {
        canvas.drawRoundRect(
            core::Rect::MakeXYWH(side * 0.24f, side * 0.46f, side * 0.52f, side * 0.30f),
            side * 0.08f,
            side * 0.08f,
            color);
        canvas.drawRoundRect(
            core::Rect::MakeXYWH(side * 0.34f, side * 0.18f, side * 0.32f, side * 0.34f),
            side * 0.16f,
            side * 0.16f,
            color,
            rendering::PaintStyle::Stroke,
            std::max(1.5f, side * 0.08f));
    });
}

rendering::Image makeClearIcon(core::Color color, float sizeHint)
{
    return makeIcon(sizeHint, [color](rendering::Canvas& canvas, float side) {
        const float inset = side * 0.24f;
        const float stroke = std::max(1.5f, side * 0.1f);
        canvas.drawLine(inset, inset, side - inset, side - inset, color, stroke, true);
        canvas.drawLine(side - inset, inset, inset, side - inset, color, stroke, true);
    });
}

rendering::Image makeSearchIcon(core::Color color, float sizeHint)
{
    return makeIcon(sizeHint, [color](rendering::Canvas& canvas, float side) {
        const float stroke = std::max(1.5f, side * 0.08f);
        canvas.drawCircle(
            side * 0.42f,
            side * 0.42f,
            side * 0.18f,
            color,
            rendering::PaintStyle::Stroke,
            stroke);
        canvas.drawLine(
            side * 0.56f,
            side * 0.56f,
            side * 0.78f,
            side * 0.78f,
            color,
            stroke,
            true);
    });
}

std::shared_ptr<Panel> makeActivityCard(const ActivityEntry& entry, Theme theme, std::size_t index)
{
    auto itemCard = std::make_shared<Panel>();
    itemCard->setBackgroundColor(index % 2 == 0 ? theme.surface : theme.background);
    itemCard->setCornerRadius(theme.cornerRadius);

    auto itemLayout = std::make_unique<VBoxLayout>();
    itemLayout->padding = 14.0f;
    itemLayout->spacing = 6.0f;
    itemCard->setLayout(std::move(itemLayout));

    auto itemTitle = std::make_shared<Label>(entry.title);
    auto itemMeta = std::make_shared<Label>(entry.meta);
    itemMeta->setColor(theme.textSecondary);
    auto itemHint = std::make_shared<ParagraphLabel>(entry.hint);
    itemHint->setColor(theme.textSecondary);
    itemHint->setMaxLines(2);

    itemCard->addChild(itemTitle);
    itemCard->addChild(itemMeta);
    itemCard->addChild(itemHint);
    return itemCard;
}

void rebuildActivityList(
    const std::shared_ptr<ListView>& list,
    const std::shared_ptr<ParagraphLabel>& summary,
    const std::vector<ActivityEntry>& entries,
    Theme theme,
    std::string_view query)
{
    if (list == nullptr || summary == nullptr) {
        return;
    }

    list->clearItems();
    std::size_t visibleCount = 0;
    for (std::size_t index = 0; index < entries.size(); ++index) {
        if (!matchesActivityEntry(entries[index], query)) {
            continue;
        }
        list->addItem(makeActivityCard(entries[index], theme, visibleCount));
        ++visibleCount;
    }

    if (visibleCount == 0) {
        auto emptyState = std::make_shared<Panel>();
        emptyState->setBackgroundColor(theme.surface);
        emptyState->setCornerRadius(theme.cornerRadius);
        auto emptyLayout = std::make_unique<VBoxLayout>();
        emptyLayout->padding = 18.0f;
        emptyLayout->spacing = 8.0f;
        emptyState->setLayout(std::move(emptyLayout));

        auto emptyTitle = std::make_shared<Label>("No sessions match your filter");
        auto emptyHint = std::make_shared<ParagraphLabel>(
            "Try searching by device, city, or review state, or clear the search field.");
        emptyHint->setColor(theme.textSecondary);
        emptyHint->setMaxLines(2);
        emptyState->addChild(emptyTitle);
        emptyState->addChild(emptyHint);
        list->addItem(emptyState);
    }

    if (query.empty()) {
        summary->setText(
            "Browse recent sessions by device, city, and review state. "
            "Use the search box to narrow the feed instantly.");
    } else {
        summary->setText(
            "Showing " + std::to_string(visibleCount) + " of "
            + std::to_string(entries.size()) + " sessions for \"" + std::string(query) + "\".");
    }
}

}  // namespace

std::shared_ptr<Widget> createDemoScene(Theme theme, AnimationSink& animations)
{
    const rendering::Image userFieldIcon = makeUserIcon(theme.primary, 18.0f);
    const rendering::Image lockFieldIcon = makeLockIcon(theme.primary, 18.0f);
    const rendering::Image searchFieldIcon = makeSearchIcon(theme.textSecondary, 18.0f);
    const rendering::Image clearFieldIcon = makeClearIcon(theme.textSecondary, 16.0f);
    const rendering::Image loginButtonIcon = makeLockIcon(theme.onPrimary, 16.0f);
    const rendering::Image clearButtonIcon = makeClearIcon(theme.onPrimary, 16.0f);

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

    auto subtitle = std::make_shared<Label>("Phase 6: icon-ready forms and searchable activity workspace");
    subtitle->setFontSize(theme.fontSize);
    subtitle->setColor(theme.textSecondary);
    root->addChild(subtitle);

    auto workspace = std::make_shared<Container>();
    auto workspaceResponsive = std::make_unique<ResponsiveLayout>();
    auto stackedWorkspace = std::make_unique<VBoxLayout>();
    stackedWorkspace->spacing = 18.0f;
    auto wideWorkspace = std::make_unique<FlexLayout>();
    auto* wideWorkspaceLayout = wideWorkspace.get();
    wideWorkspace->direction = FlexDirection::Row;
    wideWorkspace->spacing = 22.0f;
    wideWorkspace->alignItems = AlignItems::Stretch;
    workspaceResponsive->addBreakpoint(0.0f, std::move(stackedWorkspace));
    workspaceResponsive->addBreakpoint(980.0f, std::move(wideWorkspace));
    workspace->setLayout(std::move(workspaceResponsive));

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
        "The form now supports field icons, quick clear actions, and clipboard shortcuts.");
    cardBody->setColor(theme.textSecondary);
    cardBody->setMaxLines(3);

    auto emailCaption = std::make_shared<Label>("Email");
    emailCaption->setColor(theme.textSecondary);
    auto emailInput = std::make_shared<TextInput>("name@company.com");
    emailInput->setLeadingIcon(userFieldIcon);
    emailInput->setTrailingIcon(clearFieldIcon);

    auto passwordCaption = std::make_shared<Label>("Password");
    passwordCaption->setColor(theme.textSecondary);
    auto passwordInput = std::make_shared<TextInput>("Enter your password");
    passwordInput->setObscured(true);
    passwordInput->setLeadingIcon(lockFieldIcon);
    passwordInput->setTrailingIcon(clearFieldIcon);

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
    loginButton->setIcon(loginButtonIcon);
    loginButton->setIconSize(core::Size::Make(16.0f, 16.0f));
    loginButton->onClick([emailInput, passwordInput, status, theme] {
        if (emailInput->text().empty() || passwordInput->text().empty()) {
            status->setText("Please enter both email and password.");
            status->setColor(core::colorRGB(250, 179, 135));
            return;
        }

        status->setText("Signing in as " + emailInput->text());
        status->setColor(theme.primary);
    });

    auto resetButton = std::make_shared<Button>("Clear");
    resetButton->setIcon(clearButtonIcon);
    resetButton->setIconSize(core::Size::Make(16.0f, 16.0f));
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

    auto accountColumn = std::make_shared<Container>();
    auto accountLayout = std::make_unique<VBoxLayout>();
    accountLayout->spacing = 18.0f;
    accountColumn->setLayout(std::move(accountLayout));
    accountColumn->addChild(loginCard);
    accountColumn->addChild(controlsCard);

    auto activityCard = std::make_shared<Panel>();
    activityCard->setBackgroundColor(theme.surface);
    activityCard->setCornerRadius(theme.cornerRadius + 4.0f);
    auto activityLayout = std::make_unique<VBoxLayout>();
    activityLayout->padding = 20.0f;
    activityLayout->spacing = 12.0f;
    activityCard->setLayout(std::move(activityLayout));

    auto feedTitle = std::make_shared<Label>("Recent Sign-In Activity");
    feedTitle->setFontSize(theme.fontSizeLarge - 2.0f);

    auto feedBody = std::make_shared<ParagraphLabel>(
        "Browse recent sessions by device, city, and review state.");
    feedBody->setColor(theme.textSecondary);
    feedBody->setMaxLines(2);

    auto searchInput = std::make_shared<TextInput>("Filter by device, city, or review state");
    searchInput->setLeadingIcon(searchFieldIcon);
    searchInput->setTrailingIcon(clearFieldIcon);

    auto activityList = std::make_shared<ListView>();
    activityList->setPreferredHeight(240.0f);
    activityList->setSpacing(10.0f);
    activityList->setPadding(10.0f);

    const auto activityEntries = std::make_shared<std::vector<ActivityEntry>>(std::initializer_list<ActivityEntry>{
        { "Session #1  |  Windows 11", "Shanghai Office  |  Review score 92", "Multi-factor challenge completed successfully." },
        { "Session #2  |  macOS Sonoma", "Tokyo Remote  |  Review score 90", "Trusted device check completed in under 2 seconds." },
        { "Session #3  |  Android 15", "Shenzhen Mobile  |  Review score 88", "Passwordless sign-in used a recent passkey." },
        { "Session #4  |  Ubuntu 24.04", "Singapore VPN  |  Review score 85", "New browser fingerprint reviewed by strict policy." },
        { "Session #5  |  iPadOS 18", "Shanghai Lab  |  Review score 91", "No unusual risk detected in the last 24 hours." },
        { "Session #6  |  Android 14", "Beijing Transit  |  Review score 80", "Carrier network changed twice during the session." },
        { "Session #7  |  Windows 11", "Seoul Office  |  Review score 89", "Background device posture scan completed successfully." },
        { "Session #8  |  macOS Sonoma", "Hangzhou Remote  |  Review score 87", "Geo-location drift stayed within workspace baseline." },
        { "Session #9  |  ChromeOS", "Guangzhou Kiosk  |  Review score 78", "Review score lowered because the device is shared." },
        { "Session #10  |  iPhone 17", "Shanghai Mobile  |  Review score 93", "Face ID verified, session resumed without challenge." },
        { "Session #11  |  Windows 11", "Singapore Office  |  Review score 90", "No unusual risk detected in the last 24 hours." },
        { "Session #12  |  Ubuntu 24.04", "Tokyo Satellite  |  Review score 84", "Session remained stable after elevated review." },
    });

    rebuildActivityList(activityList, feedBody, *activityEntries, theme, {});
    activityCard->addChild(feedTitle);
    activityCard->addChild(feedBody);
    activityCard->addChild(searchInput);
    activityCard->addChild(activityList);

    workspace->addChild(accountColumn);
    workspace->addChild(activityCard);
    wideWorkspaceLayout->setFlex(accountColumn.get(), 0.95f, 1.0f);
    wideWorkspaceLayout->setFlex(activityCard.get(), 1.25f, 1.0f);
    root->addChild(workspace);

    const std::weak_ptr<TextInput> weakEmailInput = emailInput;
    const std::weak_ptr<TextInput> weakPasswordInput = passwordInput;
    const std::weak_ptr<TextInput> weakSearchInput = searchInput;
    const std::weak_ptr<Label> weakStatus = status;
    emailInput->onTrailingIconClick([weakEmailInput, weakStatus, theme] {
        if (const auto input = weakEmailInput.lock(); input != nullptr) {
            if (!input->text().empty()) {
                input->setText("");
            }
        }
        if (const auto label = weakStatus.lock(); label != nullptr) {
            label->setText("Email field cleared");
            label->setColor(theme.textSecondary);
        }
    });
    passwordInput->onTrailingIconClick([weakPasswordInput, weakStatus, theme] {
        if (const auto input = weakPasswordInput.lock(); input != nullptr) {
            if (!input->text().empty()) {
                input->setText("");
            }
        }
        if (const auto label = weakStatus.lock(); label != nullptr) {
            label->setText("Password field cleared");
            label->setColor(theme.textSecondary);
        }
    });
    searchInput->onTrailingIconClick([weakSearchInput] {
        if (const auto input = weakSearchInput.lock(); input != nullptr) {
            if (!input->text().empty()) {
                input->setText("");
            }
        }
    });

    const std::weak_ptr<ListView> weakSearchActivityList = activityList;
    const std::weak_ptr<ParagraphLabel> weakFeedBody = feedBody;
    searchInput->onTextChanged([weakSearchActivityList, weakFeedBody, activityEntries, theme](const std::string& text) {
        const auto list = weakSearchActivityList.lock();
        const auto summary = weakFeedBody.lock();
        rebuildActivityList(list, summary, *activityEntries, theme, text);
    });

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

    animations.animate(
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

    return root;
}

}  // namespace tinalux::ui
