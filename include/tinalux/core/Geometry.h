#pragma once

#include <algorithm>
#include <cstdint>

namespace tinalux::core {

class Color {
public:
    using Channel = std::uint8_t;

    constexpr Color() = default;
    constexpr Color(std::uint32_t value)
        : value_(value)
    {
    }

    [[nodiscard]] constexpr Channel alpha() const
    {
        return static_cast<Channel>((value_ >> 24) & 0xFFu);
    }

    [[nodiscard]] constexpr Channel red() const
    {
        return static_cast<Channel>((value_ >> 16) & 0xFFu);
    }

    [[nodiscard]] constexpr Channel green() const
    {
        return static_cast<Channel>((value_ >> 8) & 0xFFu);
    }

    [[nodiscard]] constexpr Channel blue() const
    {
        return static_cast<Channel>(value_ & 0xFFu);
    }

    [[nodiscard]] constexpr std::uint32_t value() const
    {
        return value_;
    }

    constexpr operator std::uint32_t() const
    {
        return value_;
    }

    friend constexpr bool operator==(const Color&, const Color&) = default;

private:
    std::uint32_t value_ = 0;
};

class Point {
public:
    constexpr Point() = default;
    constexpr Point(float x, float y)
        : x_(x)
        , y_(y)
    {
    }

    [[nodiscard]] static constexpr Point Make(float x, float y)
    {
        return Point(x, y);
    }

    [[nodiscard]] constexpr float x() const
    {
        return x_;
    }

    [[nodiscard]] constexpr float y() const
    {
        return y_;
    }

    constexpr void offset(float dx, float dy)
    {
        x_ += dx;
        y_ += dy;
    }

    friend constexpr bool operator==(const Point&, const Point&) = default;

private:
    float x_ = 0.0f;
    float y_ = 0.0f;
};

class Size {
public:
    constexpr Size() = default;
    constexpr Size(float width, float height)
        : width_(width)
        , height_(height)
    {
    }

    [[nodiscard]] static constexpr Size Make(float width, float height)
    {
        return Size(width, height);
    }

    [[nodiscard]] constexpr float width() const
    {
        return width_;
    }

    [[nodiscard]] constexpr float height() const
    {
        return height_;
    }

    friend constexpr bool operator==(const Size&, const Size&) = default;

private:
    float width_ = 0.0f;
    float height_ = 0.0f;
};

class Rect {
public:
    constexpr Rect() = default;
    constexpr Rect(float left, float top, float right, float bottom)
        : left_(left)
        , top_(top)
        , right_(right)
        , bottom_(bottom)
    {
    }

    [[nodiscard]] static constexpr Rect MakeLTRB(float left, float top, float right, float bottom)
    {
        return Rect(left, top, right, bottom);
    }

    [[nodiscard]] static constexpr Rect MakeXYWH(float x, float y, float width, float height)
    {
        return Rect(x, y, x + width, y + height);
    }

    [[nodiscard]] static constexpr Rect MakeWH(float width, float height)
    {
        return MakeXYWH(0.0f, 0.0f, width, height);
    }

    [[nodiscard]] static constexpr Rect MakeEmpty()
    {
        return Rect();
    }

    [[nodiscard]] constexpr float left() const
    {
        return left_;
    }

    [[nodiscard]] constexpr float top() const
    {
        return top_;
    }

    [[nodiscard]] constexpr float right() const
    {
        return right_;
    }

    [[nodiscard]] constexpr float bottom() const
    {
        return bottom_;
    }

    [[nodiscard]] constexpr float x() const
    {
        return left_;
    }

    [[nodiscard]] constexpr float y() const
    {
        return top_;
    }

    [[nodiscard]] constexpr float width() const
    {
        return right_ - left_;
    }

    [[nodiscard]] constexpr float height() const
    {
        return bottom_ - top_;
    }

    [[nodiscard]] constexpr float centerX() const
    {
        return (left_ + right_) * 0.5f;
    }

    [[nodiscard]] constexpr float centerY() const
    {
        return (top_ + bottom_) * 0.5f;
    }

    [[nodiscard]] constexpr bool isEmpty() const
    {
        return !(left_ < right_ && top_ < bottom_);
    }

    constexpr void setEmpty()
    {
        left_ = 0.0f;
        top_ = 0.0f;
        right_ = 0.0f;
        bottom_ = 0.0f;
    }

    [[nodiscard]] constexpr bool contains(float x, float y) const
    {
        return !isEmpty() && x >= left_ && x < right_ && y >= top_ && y < bottom_;
    }

    [[nodiscard]] constexpr bool contains(Point point) const
    {
        return contains(point.x(), point.y());
    }

    [[nodiscard]] constexpr bool intersects(const Rect& other) const
    {
        return !(isEmpty() || other.isEmpty() || left_ >= other.right_ || other.left_ >= right_
            || top_ >= other.bottom_ || other.top_ >= bottom_);
    }

    constexpr void join(const Rect& other)
    {
        if (other.isEmpty()) {
            return;
        }

        if (isEmpty()) {
            *this = other;
            return;
        }

        left_ = std::min(left_, other.left_);
        top_ = std::min(top_, other.top_);
        right_ = std::max(right_, other.right_);
        bottom_ = std::max(bottom_, other.bottom_);
    }

    [[nodiscard]] constexpr Rect makeOffset(float dx, float dy) const
    {
        return Rect(left_ + dx, top_ + dy, right_ + dx, bottom_ + dy);
    }

    [[nodiscard]] constexpr Rect makeInset(float dx, float dy) const
    {
        return Rect(left_ + dx, top_ + dy, right_ - dx, bottom_ - dy);
    }

    constexpr void offset(float dx, float dy)
    {
        left_ += dx;
        top_ += dy;
        right_ += dx;
        bottom_ += dy;
    }

    friend constexpr bool operator==(const Rect&, const Rect&) = default;

private:
    float left_ = 0.0f;
    float top_ = 0.0f;
    float right_ = 0.0f;
    float bottom_ = 0.0f;
};

inline constexpr Color colorRGB(Color::Channel red, Color::Channel green, Color::Channel blue)
{
    return Color(
        (0xFFu << 24) | (static_cast<std::uint32_t>(red) << 16)
        | (static_cast<std::uint32_t>(green) << 8) | static_cast<std::uint32_t>(blue));
}

inline constexpr Color colorARGB(
    Color::Channel alpha,
    Color::Channel red,
    Color::Channel green,
    Color::Channel blue)
{
    return Color(
        (static_cast<std::uint32_t>(alpha) << 24) | (static_cast<std::uint32_t>(red) << 16)
        | (static_cast<std::uint32_t>(green) << 8) | static_cast<std::uint32_t>(blue));
}

inline constexpr Color::Channel colorAlpha(Color color)
{
    return color.alpha();
}

inline constexpr Color::Channel colorRed(Color color)
{
    return color.red();
}

inline constexpr Color::Channel colorGreen(Color color)
{
    return color.green();
}

inline constexpr Color::Channel colorBlue(Color color)
{
    return color.blue();
}

}  // namespace tinalux::core
