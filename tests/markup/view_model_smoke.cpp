#include <cstdlib>
#include <iostream>
#include <string>

#include "tinalux/markup/ViewModel.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

tinalux::markup::ModelNode stringNode(const char* value)
{
    return tinalux::markup::ModelNode(tinalux::core::Value(std::string(value)));
}

tinalux::markup::ModelNode boolNode(bool value)
{
    return tinalux::markup::ModelNode(tinalux::core::Value(value));
}

} // namespace

int main()
{
    using namespace tinalux;

    auto viewModel = markup::ViewModel::create();

    int profileNameChangeCount = 0;
    std::string lastProfileName;
    int firstItemLabelChangeCount = 0;
    std::string lastFirstItemLabel;
    int itemsInvalidationCount = 0;
    int firstItemInvalidationCount = 0;

    const markup::ViewModel::ListenerId profileNameListener =
        viewModel->addListener(
            "model.profile.name",
            [&](const core::Value& value) {
                ++profileNameChangeCount;
                lastProfileName = value.asString();
            });

    viewModel->addListener(
        "items.0.label",
        [&](const core::Value& value) {
            ++firstItemLabelChangeCount;
            lastFirstItemLabel = value.asString();
        });

    viewModel->addInvalidationListener(
        "items",
        [&]() {
            ++itemsInvalidationCount;
        });

    viewModel->addInvalidationListener(
        "items.0",
        [&]() {
            ++firstItemInvalidationCount;
        });

    const bool profileChanged = viewModel->setObject(
        "profile",
        {
            {"name", stringNode("alice")},
            {"active", boolNode(true)},
        });

    expect(profileChanged, "setting profile object should report a change");
    const core::Value* profileName = viewModel->findValue("profile.name");
    expect(profileName != nullptr, "profile.name should be discoverable");
    expect(
        profileName->type() == core::ValueType::String
            && profileName->asString() == "alice",
        "profile.name should preserve scalar payload");
    expect(profileNameChangeCount == 1, "profile.name listener should fire once");
    expect(lastProfileName == "alice", "profile.name listener should receive latest text");

    const bool itemsChanged = viewModel->setArray(
        "model.items",
        {
            markup::ModelNode::object({
                {"label", stringNode("first")},
                {"visible", boolNode(true)},
            }),
            markup::ModelNode::object({
                {"label", stringNode("second")},
                {"visible", boolNode(false)},
            }),
        });

    expect(itemsChanged, "setting items array should report a change");
    const core::Value* firstLabel = viewModel->findValue("items.0.label");
    expect(firstLabel != nullptr, "array child scalar should be discoverable");
    expect(
        firstLabel->type() == core::ValueType::String
            && firstLabel->asString() == "first",
        "array child scalar should preserve initial text");
    expect(
        firstItemLabelChangeCount == 1,
        "items.0.label listener should fire after array materialization");
    expect(
        lastFirstItemLabel == "first",
        "items.0.label listener should receive first array element text");
    expect(itemsInvalidationCount == 1, "parent array invalidation should fire once");
    expect(
        firstItemInvalidationCount == 1,
        "descendant invalidation should fire when parent array is replaced");

    const bool firstLabelChanged = viewModel->setString("items.0.label", "first updated");
    expect(firstLabelChanged, "updating array child scalar should report a change");
    firstLabel = viewModel->findValue("items.0.label");
    expect(firstLabel != nullptr, "updated array child scalar should stay discoverable");
    expect(
        firstLabel->asString() == "first updated",
        "updated array child scalar should preserve latest text");
    expect(
        firstItemLabelChangeCount == 2,
        "items.0.label listener should fire for descendant updates");
    expect(
        lastFirstItemLabel == "first updated",
        "items.0.label listener should receive updated descendant text");
    expect(itemsInvalidationCount == 2, "parent invalidation should fire for descendant update");
    expect(
        firstItemInvalidationCount == 2,
        "matching descendant invalidation should fire for descendant update");

    const bool secondVisibleChanged = viewModel->setBool("items.1.visible", true);
    expect(secondVisibleChanged, "updating sibling array child should report a change");
    const core::Value* secondVisible = viewModel->findValue("items.1.visible");
    expect(secondVisible != nullptr, "sibling array child should stay discoverable");
    expect(
        secondVisible->type() == core::ValueType::Bool && secondVisible->asBool(),
        "sibling array child should preserve bool payload");
    expect(
        itemsInvalidationCount == 3,
        "parent invalidation should fire for sibling descendant update");
    expect(
        firstItemInvalidationCount == 2,
        "unrelated descendant invalidation should not fire for sibling update");

    viewModel->removeListener(profileNameListener);
    const bool profileUpdated = viewModel->setString("profile.name", "bob");
    expect(profileUpdated, "updating profile.name should still report a change");
    expect(
        profileNameChangeCount == 1,
        "removed profile.name listener should not receive later updates");
    profileName = viewModel->findValue("profile.name");
    expect(profileName != nullptr, "profile.name should remain readable after listener removal");
    expect(
        profileName->asString() == "bob",
        "profile.name should preserve latest text after listener removal");

    const markup::ModelNode* itemsNode = viewModel->findNode("items");
    expect(itemsNode != nullptr && itemsNode->isArray(), "items should resolve to an array node");
    const markup::ModelNode* firstItemNode = viewModel->findNode("items.0");
    expect(
        firstItemNode != nullptr && firstItemNode->isObject(),
        "items.0 should resolve to an object node");

    int actionCalls = 0;
    bool actionPayloadWasNone = false;
    const bool actionChanged = viewModel->setAction(
        "profile.onSave",
        [&](const core::Value& value) {
            ++actionCalls;
            actionPayloadWasNone = value.isNone();
        });
    expect(actionChanged, "setting action node should report a change");
    const markup::ModelNode* actionNode = viewModel->findNode("profile.onSave");
    expect(actionNode != nullptr && actionNode->isAction(), "action node should be discoverable");
    const markup::ModelNode::Action* action = viewModel->findAction("profile.onSave");
    expect(action != nullptr, "action path should resolve to callable handler");
    (*action)(core::Value());
    expect(actionCalls == 1, "resolved action should be invokable");
    expect(actionPayloadWasNone, "resolved action should preserve payload values");

    return 0;
}
