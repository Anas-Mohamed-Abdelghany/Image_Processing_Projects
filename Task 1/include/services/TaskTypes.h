#pragma once

// ----------------------------------------------------------------------------
// Small enums shared between the services, the controller and the UI.
// Task 3 must preview the X and Y directions separately for Sobel, Roberts and
// Prewitt, so every edge operator takes a Direction.
// ----------------------------------------------------------------------------

// Edge detection method used by Task 3.
enum class EdgeMethod {
    Sobel,
    Roberts,
    Prewitt
};

// Gradient direction of an edge operator:
//   X -> derivative along the X axis (horizontal gradient, vertical edges)
//   Y -> derivative along the Y axis (vertical gradient, horizontal edges)
enum class Direction {
    X,
    Y
};
