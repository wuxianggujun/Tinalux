#include "ShowcasePages.h"

#include <vector>

#include "ShowcasePageFactories.h"

namespace tinalux::ui::showcase {

std::vector<ShowcasePage> buildShowcasePages(Theme theme)
{
    return {
        {
            .category = "Foundations",
            .title = "Overview",
            .summary =
                "High-level project shell and layout split. Start here to understand how the demo is organized before diving into a specific capability page.",
            .content = createOverviewPage(theme),
        },
        {
            .category = "Foundations",
            .title = "Layout",
            .summary =
                "Flex and responsive layout behaviors isolated from business widgets so layout regressions remain easy to spot.",
            .content = createLayoutPage(theme),
        },
        {
            .category = "Input",
            .title = "Auth Form",
            .summary =
                "TextInput, button, validation, and action feedback isolated on a single authentication-flavored page.",
            .content = createAuthFormPage(theme),
        },
        {
            .category = "Input",
            .title = "Controls",
            .summary =
                "Checkbox, toggle, radio, and slider widgets isolated from the form page so control state does not mix with auth flow state.",
            .content = createControlsPage(theme),
        },
        {
            .category = "Text",
            .title = "Rich Text",
            .summary =
                "Structured document display with headings, lists, quotes, inline code, code blocks, and style overrides.",
            .content = createRichTextPage(theme),
        },
        {
            .category = "Media",
            .title = "Images and Resources",
            .summary =
                "Image fitting, icon rendering, and future resource previews isolated from form and list pages.",
            .content = createImageResourcePage(theme),
        },
        {
            .category = "Data Display",
            .title = "Activity Feed",
            .summary =
                "Searchable list showcase for list view, filtering, and card-based content density without unrelated form controls around it.",
            .content = createActivityPage(theme),
        },
        {
            .category = "Overlay",
            .title = "Dialog",
            .summary =
                "Dialog structure isolated from the main pages so overlay and popup evolution stays local.",
            .content = createDialogPage(theme),
        },
        {
            .category = "Theming",
            .title = "Theme",
            .summary =
                "ThemeManager-driven global theme switching isolated from page-local content state.",
            .content = createThemePage(theme),
        },
    };
}

}  // namespace tinalux::ui::showcase
