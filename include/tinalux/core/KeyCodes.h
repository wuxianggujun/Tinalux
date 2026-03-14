#pragma once

namespace tinalux::core::keys {

inline constexpr int kSpace = 32;        // GLFW_KEY_SPACE
inline constexpr int kA = 65;            // GLFW_KEY_A
inline constexpr int kC = 67;            // GLFW_KEY_C
inline constexpr int kV = 86;            // GLFW_KEY_V
inline constexpr int kX = 88;            // GLFW_KEY_X
inline constexpr int kEscape = 256;      // GLFW_KEY_ESCAPE
inline constexpr int kEnter = 257;       // GLFW_KEY_ENTER
inline constexpr int kTab = 258;         // GLFW_KEY_TAB
inline constexpr int kBackspace = 259;   // GLFW_KEY_BACKSPACE
inline constexpr int kDelete = 261;      // GLFW_KEY_DELETE
inline constexpr int kRight = 262;       // GLFW_KEY_RIGHT
inline constexpr int kLeft = 263;        // GLFW_KEY_LEFT
inline constexpr int kDown = 264;        // GLFW_KEY_DOWN
inline constexpr int kUp = 265;          // GLFW_KEY_UP
inline constexpr int kPageUp = 266;      // GLFW_KEY_PAGE_UP
inline constexpr int kPageDown = 267;    // GLFW_KEY_PAGE_DOWN
inline constexpr int kHome = 268;        // GLFW_KEY_HOME
inline constexpr int kEnd = 269;         // GLFW_KEY_END
inline constexpr int kKpEnter = 335;     // GLFW_KEY_KP_ENTER

}  // namespace tinalux::core::keys

namespace tinalux::core::mods {

inline constexpr int kShift = 0x0001;    // GLFW_MOD_SHIFT
inline constexpr int kControl = 0x0002;  // GLFW_MOD_CONTROL

}  // namespace tinalux::core::mods

namespace tinalux::core::mouse {

inline constexpr int kLeft = 0;  // GLFW_MOUSE_BUTTON_LEFT

}  // namespace tinalux::core::mouse
