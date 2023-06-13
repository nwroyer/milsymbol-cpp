#pragma once

namespace milsymbol {

/**
 * @brief Represents a two-dimensional vector
 */
struct Vector2 {
    using base_t = float;

    base_t x = 0; /// X component
    base_t y = 0; /// Y component

    /// Default to (0, 0)
    inline constexpr Vector2() noexcept : x{0}, y{0} {}

    /// Constructs a vector with the given X, Y component
    inline constexpr Vector2(base_t x, base_t y) noexcept : x{x}, y{y} {}

    /// Constructs a vector with both X and Y components equal to the given scalar
    inline constexpr Vector2(base_t d) noexcept : x{d}, y{d} {}

    /// Vector addition
    inline constexpr Vector2 operator+(const Vector2& other) const noexcept {
        return Vector2{x + other.x, y + other.y};
    }

    /// Vector subtraction
    inline constexpr Vector2 operator-(const Vector2& other) const noexcept {
        return Vector2{x - other.x, y - other.y};
    }
};

/**
 * @brief Represents a bounding box
 */
struct BoundingBox {
    using base_t = Vector2::base_t;

    base_t x1 = 100; /// Minimum X component
    base_t y1 = 100; /// Minimum Y component
    base_t x2 = 100; /// Maximum X component
    base_t y2 = 100; /// Maximum Y component

    /// Returns the height of the bounding box
    inline constexpr base_t width() const noexcept {
        return x2 - x1;
    }

    /// Returns the width of the bounding box
    inline constexpr base_t height() const noexcept {
        return y2 - y1;
    }

    /// Return the center of the bounding box
    inline constexpr Vector2 center() const noexcept {
        return Vector2{x1 + (width() / 2), y1 + (height() / 2)};
    }

    /// Returns the size as a vector
    inline constexpr Vector2 size() const noexcept {
        return Vector2{width(), height()};
    }

    /// Merges this bounding box with another one, modifying it
    /// in place and returning a reference
    inline constexpr BoundingBox& merge(const BoundingBox& other) noexcept {
        x1 = (other.x1 <= this->x1 ? other.x1 : this->x1);
        y1 = (other.y1 <= this->y1 ? other.y1 : this->y1);
        x2 = (other.x2 >= this->x2 ? other.x2 : this->x2);
        y2 = (other.y2 >= this->y2 ? other.y2 : this->y2);
        return *this;
    }

    /// Merges this bounding box with a point, modifying it
    /// in place and returning a reference
    inline constexpr BoundingBox& merge(const Vector2& point) noexcept {
        if (point.x < this->x1)
            this->x1 = point.x;
        if (point.x > this->x2)
            this->x2 = point.x;
        if (point.y < this->y1)
            this->y1 = point.y;
        if (point.y > this->y2)
            this->y2 = point.y;
        return *this;
    }

    /// Returns the same bounding box but with the given Y1 value
    inline constexpr BoundingBox with_y1(base_t y1) const noexcept {
        BoundingBox ret = *this;
        ret.y1 = y1;
        return ret;
    }

    /// Returns the same bounding box but with the given Y2 value
    inline constexpr BoundingBox with_y2(base_t y2) const noexcept {
        BoundingBox ret = *this;
        ret.y2 = y2;
        return ret;
    }

    /// Translates the bounding box by the given Vector2 delta
    inline constexpr BoundingBox translated(const Vector2& delta) const noexcept {
        return BoundingBox{x1 + delta.x, y1 + delta.y, x2 + delta.x, y2 + delta.y};
    }

    /// Return the upper left corner
    inline constexpr Vector2 point_1() const noexcept {
        return Vector2{x1, y1};
    }

    /// Return the upper right corner
    inline constexpr Vector2 point_2() const noexcept {
        return Vector2{x2, y2};
    }
};

}
