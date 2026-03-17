#include "TextInputLayoutCache.h"

#include <algorithm>
#include <cmath>

#include "../../TextPrimitives.h"
#include "TextInputModel.h"
#include "TextInputUtf8.h"

namespace tinalux::ui::detail {

namespace {

bool sameFontSize(float lhs, float rhs)
{
    return std::abs(lhs - rhs) <= 0.001f;
}

}  // namespace

void TextInputLayoutCache::invalidate()
{
    dirty_ = true;
}

void TextInputLayoutCache::ensure(
    const TextInputModel& model,
    const std::string& placeholder,
    bool obscured,
    float fontSize)
{
    const float clampedFontSize = std::max(fontSize, 1.0f);
    const bool needsRebuild = dirty_
        || !sameFontSize(cachedFontSize_, clampedFontSize)
        || cachedCommittedText_ != model.text()
        || cachedPlaceholder_ != placeholder
        || cachedCompositionText_ != model.compositionText()
        || cachedCompositionReplacePending_ != model.compositionReplacePending()
        || cachedCompositionReplaceStart_ != model.compositionReplaceStart()
        || cachedCompositionReplaceEnd_ != model.compositionReplaceEnd()
        || cachedObscured_ != obscured;
    if (!needsRebuild) {
        return;
    }

    cachedFontSize_ = clampedFontSize;
    cachedCommittedText_ = model.text();
    cachedPlaceholder_ = placeholder;
    cachedCompositionText_ = model.compositionText();
    cachedCompositionReplacePending_ = model.compositionReplacePending();
    cachedCompositionReplaceStart_ = model.compositionReplaceStart();
    cachedCompositionReplaceEnd_ = model.compositionReplaceEnd();
    cachedObscured_ = obscured;
    cachedDisplayText_ = model.displayText(placeholder, obscured);

    const TextMetrics metrics = measureTextMetrics(cachedDisplayText_, cachedFontSize_);
    cachedTextWidth_ = metrics.width;
    cachedTextHeight_ = metrics.height;
    cachedBaseline_ = metrics.baseline;
    cachedDrawX_ = metrics.drawX;

    rebuildPrefixCache(cachedCommittedText_, obscured, cachedFontSize_, cachedCaretOffsets_, cachedCaretXs_);
    rebuildPrefixCache(cachedCompositionText_, obscured, cachedFontSize_, cachedCompositionOffsets_, cachedCompositionXs_);
    dirty_ = false;
}

const std::string& TextInputLayoutCache::displayText() const
{
    return cachedDisplayText_;
}

float TextInputLayoutCache::textWidth() const
{
    return cachedTextWidth_;
}

float TextInputLayoutCache::textHeight() const
{
    return cachedTextHeight_;
}

float TextInputLayoutCache::baseline() const
{
    return cachedBaseline_;
}

float TextInputLayoutCache::drawX() const
{
    return cachedDrawX_;
}

float TextInputLayoutCache::prefixWidth(
    const TextInputModel& model,
    std::size_t cursorPos,
    float fontSize)
{
    ensure(model, {}, cachedObscured_, fontSize);

    const auto it = std::lower_bound(
        cachedCaretOffsets_.begin(),
        cachedCaretOffsets_.end(),
        std::min(cursorPos, model.text().size()));
    if (it == cachedCaretOffsets_.end()) {
        return cachedCaretXs_.empty() ? 0.0f : cachedCaretXs_.back();
    }

    return cachedCaretXs_[static_cast<std::size_t>(std::distance(cachedCaretOffsets_.begin(), it))];
}

std::size_t TextInputLayoutCache::cursorFromLocalX(
    const TextInputModel& model,
    float localX,
    float fontSize)
{
    ensure(model, {}, cachedObscured_, fontSize);
    if (cachedCaretXs_.empty()) {
        return 0;
    }

    const auto it = std::lower_bound(cachedCaretXs_.begin(), cachedCaretXs_.end(), localX);
    if (it == cachedCaretXs_.begin()) {
        return cachedCaretOffsets_.front();
    }
    if (it == cachedCaretXs_.end()) {
        return cachedCaretOffsets_.back();
    }

    const std::size_t index = static_cast<std::size_t>(std::distance(cachedCaretXs_.begin(), it));
    const float right = cachedCaretXs_[index];
    const float left = cachedCaretXs_[index - 1];
    return std::abs(localX - left) <= std::abs(localX - right)
        ? cachedCaretOffsets_[index - 1]
        : cachedCaretOffsets_[index];
}

float TextInputLayoutCache::compositionPrefixWidth(
    const TextInputModel& model,
    std::size_t utf8Offset,
    float fontSize)
{
    ensure(model, {}, cachedObscured_, fontSize);
    if (cachedCompositionOffsets_.empty()) {
        return 0.0f;
    }

    const auto it = std::lower_bound(
        cachedCompositionOffsets_.begin(),
        cachedCompositionOffsets_.end(),
        std::min(utf8Offset, model.compositionText().size()));
    if (it == cachedCompositionOffsets_.end()) {
        return cachedCompositionXs_.empty() ? 0.0f : cachedCompositionXs_.back();
    }

    return cachedCompositionXs_[static_cast<std::size_t>(std::distance(cachedCompositionOffsets_.begin(), it))];
}

std::size_t TextInputLayoutCache::compositionCursorFromLocalX(
    const TextInputModel& model,
    float localX,
    float fontSize)
{
    if (model.compositionText().empty()) {
        return 0;
    }

    ensure(model, {}, cachedObscured_, fontSize);
    const float compositionStart = prefixWidth(model, model.compositionReplaceStart(), fontSize);
    const float relativeX = std::max(0.0f, localX - compositionStart);

    const auto it = std::lower_bound(cachedCompositionXs_.begin(), cachedCompositionXs_.end(), relativeX);
    if (it == cachedCompositionXs_.begin()) {
        return cachedCompositionOffsets_.front();
    }
    if (it == cachedCompositionXs_.end()) {
        return cachedCompositionOffsets_.back();
    }

    const std::size_t index = static_cast<std::size_t>(std::distance(cachedCompositionXs_.begin(), it));
    const float right = cachedCompositionXs_[index];
    const float left = cachedCompositionXs_[index - 1];
    return std::abs(relativeX - left) <= std::abs(relativeX - right)
        ? cachedCompositionOffsets_[index - 1]
        : cachedCompositionOffsets_[index];
}

void TextInputLayoutCache::rebuildPrefixCache(
    const std::string& text,
    bool obscured,
    float fontSize,
    std::vector<std::size_t>& offsets,
    std::vector<float>& xs)
{
    offsets.clear();
    xs.clear();
    offsets.push_back(0);
    xs.push_back(0.0f);

    if (text.empty()) {
        return;
    }

    std::string prefix;
    prefix.reserve(obscured ? countCodepointsUntil(text, text.size()) : text.size());

    for (std::size_t offset = 0; offset < text.size();) {
        const std::size_t next = nextUtf8Offset(text, offset);
        if (obscured) {
            prefix.push_back('*');
        } else {
            prefix.append(text, offset, next - offset);
        }
        offset = next;
        offsets.push_back(offset);
        xs.push_back(measureTextMetrics(prefix, fontSize).width);
    }
}

}  // namespace tinalux::ui::detail
