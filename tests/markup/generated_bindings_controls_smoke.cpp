#include <cstdlib>
#include <iostream>

#include "generated_controls.markup.h"
#include "tinalux/ui/Theme.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class ControlsPage {
public:
    int leadingIconClicks = 0;
    int trailingIconClicks = 0;
    int dismissCount = 0;
    bool subscribe = false;
    bool syncEnabled = false;
    float volume = 0.0f;
    generated_controls::Page page;

    explicit ControlsPage(const tinalux::ui::Theme& theme)
        : page(
              theme,
              [&](auto& ui) {
                  expect(
                      ui.searchInput.onLeadingIconClick(
                          this,
                          &ControlsPage::handleLeadingIconClick),
                      "generated controls page should bind leading icon member handlers");
                  expect(
                      ui.searchInput.onTrailingIconClick(
                          this,
                          &ControlsPage::handleTrailingIconClick),
                      "generated controls page should bind trailing icon member handlers");
                  expect(
                      ui.subscribeBox.onToggle(this, &ControlsPage::handleSubscribeChanged),
                      "generated controls page should bind checkbox member handlers");
                  expect(
                      ui.syncToggle.onToggle(this, &ControlsPage::handleSyncChanged),
                      "generated controls page should bind toggle member handlers");
                  expect(
                      ui.volumeSlider.onValueChanged(this, &ControlsPage::handleVolumeChanged),
                      "generated controls page should bind slider member handlers");
                  expect(
                      ui.confirmDialog.onDismiss(this, &ControlsPage::handleDismiss),
                      "generated controls page should bind dialog member handlers");
              })
    {
    }

    void handleLeadingIconClick()
    {
        ++leadingIconClicks;
    }

    void handleTrailingIconClick()
    {
        ++trailingIconClicks;
    }

    void handleSubscribeChanged(bool checked)
    {
        subscribe = checked;
    }

    void handleSyncChanged(bool on)
    {
        syncEnabled = on;
    }

    void handleVolumeChanged(float value)
    {
        volume = value;
    }

    void handleDismiss()
    {
        ++dismissCount;
    }
};

} // namespace

int main()
{
    using namespace tinalux;

    const auto theme = ui::Theme::dark();
    ControlsPage controlsPage(theme);
    expect(controlsPage.page.ok(), "generated controls page should load");
    expect(
        controlsPage.page.ui.searchInput != nullptr,
        "generated controls page should expose text input proxy");
    expect(
        controlsPage.page.ui.subscribeBox != nullptr,
        "generated controls page should expose checkbox proxy");
    expect(
        controlsPage.page.ui.syncToggle != nullptr,
        "generated controls page should expose toggle proxy");
    expect(
        controlsPage.page.ui.volumeSlider != nullptr,
        "generated controls page should expose slider proxy");
    expect(
        controlsPage.page.ui.confirmDialog != nullptr,
        "generated controls page should expose dialog proxy");

    const markup::ModelNode::Action* leadingAction =
        controlsPage.page.model()->findAction("onSearchIconClicked");
    expect(leadingAction != nullptr, "generated controls page should register leading icon action");
    (*leadingAction)(core::Value());
    expect(
        controlsPage.leadingIconClicks == 1,
        "generated controls page should dispatch leading icon handlers");

    const markup::ModelNode::Action* trailingAction =
        controlsPage.page.model()->findAction("onClearIconClicked");
    expect(trailingAction != nullptr, "generated controls page should register trailing icon action");
    (*trailingAction)(core::Value());
    expect(
        controlsPage.trailingIconClicks == 1,
        "generated controls page should dispatch trailing icon handlers");

    const markup::ModelNode::Action* subscribeAction =
        controlsPage.page.model()->findAction("onSubscribeChanged");
    expect(subscribeAction != nullptr, "generated controls page should register checkbox action");
    (*subscribeAction)(core::Value(true));
    expect(
        controlsPage.subscribe,
        "generated controls page should dispatch checkbox bool payloads");

    const markup::ModelNode::Action* syncAction =
        controlsPage.page.model()->findAction("onSyncChanged");
    expect(syncAction != nullptr, "generated controls page should register toggle action");
    (*syncAction)(core::Value(true));
    expect(
        controlsPage.syncEnabled,
        "generated controls page should dispatch toggle bool payloads");

    const markup::ModelNode::Action* volumeAction =
        controlsPage.page.model()->findAction("onVolumeChanged");
    expect(volumeAction != nullptr, "generated controls page should register slider action");
    (*volumeAction)(core::Value(7.5f));
    expect(
        controlsPage.volume == 7.5f,
        "generated controls page should dispatch slider float payloads");

    const markup::ModelNode::Action* dismissAction =
        controlsPage.page.model()->findAction("onConfirmDismissed");
    expect(dismissAction != nullptr, "generated controls page should register dialog dismiss action");
    (*dismissAction)(core::Value());
    expect(
        controlsPage.dismissCount == 1,
        "generated controls page should dispatch dialog dismiss handlers");

    return 0;
}
