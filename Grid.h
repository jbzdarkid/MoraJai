#pragma once
#include <cstdint>

enum Color : uint8_t {
    Invalid,
    Red,
    Orange,
    Yellow,
    Green,
    Purple,
    Pink,
    White,
    Gray,
    Black,

    NUM_COLORS,
};

struct Grid {
    Grid() = default; // C++ requires this constructor to make an unordered_set<Grid>

    Grid(std::initializer_list<Color> colors, Color target) {
        // The natural way of writing the colors is (0, 0), (0, 1), (0, 2),
        // but this doesn't line up with the order of elements in the actual grid.
        int i = 0;
        for (Color color : colors) {
            int y = i / 3;
            int x = i % 3;
            _grid[x * 3 + y] = color;
            i++;
        }
        _target = target;
    }

    bool operator==(const Grid& other) const {
        return std::memcmp(&_grid[0], &other._grid[0], sizeof(_grid) / sizeof(_grid[0])) == 0
            && _target == other._target;
    }

    // TODO: Even a basic hashing operator would probably save a bunch of time here.
    std::size_t operator()(const Grid& grid) const { return 0; }

    // TODO: perf hotspot due to branching, I suspect. We could probably make this internally a 5x5 grid to address that?
    Color Get(int x, int y) const {
        if (x < 0 || x >= 3) return Invalid;
        if (y < 0 || y >= 3) return Invalid;
        return _grid[x * 3 + y];
    }

    bool Victory() const {
        return (Get(0, 0) == _target && Get(0, 2) == _target
            && Get(2, 0) == _target && Get(2, 2) == _target);
    }

    void Click(int x, int y) {
        switch (Get(x, y)) {
        case Red: // White -> Gray, Gray -> Black, Black -> Red
            for (int i = 0; i < 9; i++) {
                if (_grid[i] == White) _grid[i] = Gray;
                else if (_grid[i] == Gray) _grid[i] = Black;
                else if (_grid[i] == Black) _grid[i] = Red;
            }
            break;
        case Orange:
        {
            // Count the number of colors from adjacent cells
            int8_t counts[NUM_COLORS] = {};
            counts[Get(x - 1, y)]++;
            counts[Get(x + 1, y)]++;
            counts[Get(x, y - 1)]++;
            counts[Get(x, y + 1)]++;

            // Find the color which is the most common (ties == no change)
            int8_t max = -1;
            Color maxColor = Invalid;
            bool tied = false;
            for (int color = 1; color < NUM_COLORS; color++) { // Skipping counts[Invalid]
                int8_t count = counts[color];
                if (count == 0) continue;
                if (count < max) continue;
                if (count == max) tied = true; // Another color already set this max value
                if (count > max) {
                    tied = false;
                    max = count;
                    maxColor = (Color)color;
                }
            }

            // If there is one adjacent cell color which is not tied, set our color to it.
            if (!tied && maxColor != Invalid) Set(x, y, maxColor);
            break;
        }
        case Yellow: // Swap with the cell above (if there is a cell above)
            if (y == 0) break;
            Set(x, y, Get(x, y - 1));
            Set(x, y - 1, Yellow);
            break;
        case Green: // Swap with the cell across the middle.
            if (x == 1 && y == 1) break;
            Set(x, y, Get(2 - x, 2 - y));
            Set(2 - x, 2 - y, Green);
            break;
        case Purple: // Swap with the cell below (if there is a cell below)
            if (y == 2) break;
            Set(x, y, Get(x, y + 1));
            Set(x, y + 1, Purple);
            break;
        case Pink: // Rotate clockwise... yikes.
        {
            static const std::pair<int, int> adjacencies[8] = { {-1, -1}, {0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0} };
            
            // Save each cell's color, then write in the previous color
            // Start by finding the last color in the rotation set; this is the color we'll set to the first valid element.
            Color prevColor = Invalid;
            for (const auto& [dx, dy] : adjacencies) {
                Color color = Get(x + dx, y + dy);
                if (color == Invalid) continue; // Skip OOB cells
                if (color == prevColor) continue; // Small optimization
                if (prevColor != Invalid) Set(x + dx, y + dy, prevColor);
                prevColor = color;
            }

            // Finally, write the last color back into the first slot.
            for (const auto& [dx, dy] : adjacencies) {
                Color color = Get(x + dx, y + dy);
                if (color == Invalid) continue; // Skip OOB cells
                Set(x + dx, y + dy, prevColor);
                break;
            }
            break;
        }
        case White: // Change any adjacent gray squares to white, or white squares to gray.
        {
            static const std::pair<int, int> adjacencies[4] = { {-1, 0}, {0, -1}, {1, 0}, {0, 1} };
            for (const auto& [dx, dy] : adjacencies) {
                Color color = Get(x + dx, y + dy);
                if (color == Invalid) continue;
                if (color == White) Set(x + dx, y + dy, Gray);
                else if (color == Gray) Set(x + dx, y + dy, White);
                else throw std::runtime_error("Not implemented yet -- no data");
            }

            Set(x, y, Gray);
            break;
        }
        case Gray: // Does nothing, I think.
            break;
        case Black: // Cycle colors to the right.
            Set(x, y, Get((x + 2) % 3, y));
            Set((x + 2) % 3, y, Get((x + 1) % 3, y));
            Set((x + 1) % 3, y, Black);
            break;
        case Invalid:
        case NUM_COLORS:
            std::cerr << "Cell at " << x << ", " << y << " is not initialized";
            throw std::runtime_error("Invalid color");
        default:
            std::cerr << "Cell at " << x << ", " << y << " is color " + (int)Get(x, y) << " which is not handled";
            throw std::runtime_error("Invalid color");
        }
    }


private:
    void Set(int x, int y, Color color) { _grid[x * 3 + y] = color; }

    Color _grid[9] = {};
    Color _target = Invalid;
};
