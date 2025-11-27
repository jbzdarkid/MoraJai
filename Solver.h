#pragma once
#include <algorithm>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Grid.h"

struct Solver {
    Solver(const Grid& grid) : _initial(grid) { }

    std::vector<std::pair<int, int>> Solve() {
        _bestDepth = 0xFFFF;
        _visited = std::make_unique<std::unordered_set<Grid, Grid>>();
        _winning.clear();

        // First, brute-force the problem space to find our best solution.
        bool victory = SolveRecursive(_initial, 0);
        if (!victory) return {}; // Puzzle was somehow unsolvable.
        std::cout << "Puzzle is solvable; best solution has <= " << _bestDepth << " moves" << std::endl;

        _visited = nullptr; // Regain memory

        // DFS again
        _bestCost = 2.0 * _bestDepth; // Cost is a double because it now incorporates sub-move costs.
        _bestSolutionPath.clear();
        _scratchPath = {{1, 1}}; // Assume the cursor starts in the middle
        FindBestSolutionRecursive(_initial, 0, 0.0);

        return _bestSolutionPath;
    }

private:
    bool SolveRecursive(const Grid& grid, uint16_t depth) {
        if (depth > _bestDepth) return false; // Slower than the best solution = not really a solution we care about.

        // If the node was already reached, return our cached result
        auto existing = _visited->insert(grid);
        if (!existing.second) return _winning.contains(grid);

        if (grid.Victory()) {
            _winning.insert(grid);
            _bestDepth = depth; // if depth is greater than this we already exited.
            return true;
        }

        bool anyVictory = false;
        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                Grid newGrid = grid;
                newGrid.Click(x, y);
                anyVictory |= SolveRecursive(newGrid, (uint16_t)(depth + 1));
            }
        }

        // If any child state was winning, update our state as winning, too.
        // Tail recursion will mark all other states on the winning path, too.
        if (anyVictory) _winning.insert(grid);
        return anyVictory;
    }

    void FindBestSolutionRecursive(const Grid& grid, uint16_t depth, double cost) {
        if (depth > _bestDepth) return;
        if (!_winning.contains(grid)) return;

        // Check to see if we've already processed this winning node.
        // If we've been here before (at a lower cost) there's no need to explore it again.
        auto existing = _winningCosts.emplace(grid, cost);
        if (!existing.second) { // True: Inserted, False: Element already exists
            if (existing.first->second <= cost) return;
            existing.first->second = cost; // This is a faster path to an existing node, update the cost and continue processing.
        }

        if (grid.Victory()) {
            if (depth < _bestDepth) _bestDepth = depth;
            if (cost < _bestCost) {
                _bestCost = cost;
                _bestSolutionPath = std::vector<std::pair<int, int>>(_scratchPath.begin() + 1, _scratchPath.end());
            }
            return;
        }

        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                Grid newGrid = grid;
                newGrid.Click(x, y);

                auto& [lastX, lastY] = _scratchPath.back();
                double newCost = cost + 1.0 + (0.1 * std::abs(x - lastX)) + (0.1 * std::abs(y - lastY));
                _scratchPath.emplace_back(x, y);

                FindBestSolutionRecursive(newGrid, (uint16_t)(depth + 1), newCost);

                _scratchPath.pop_back();
            }
        }
    }

    Grid _initial;
    uint16_t _bestDepth = 0;

    std::unique_ptr<std::unordered_set<Grid, Grid>> _visited;
    std::unordered_set<Grid, Grid> _winning;
    std::unordered_map<Grid, double, Grid> _winningCosts;

    std::vector<std::pair<int, int>> _scratchPath;
    double _bestCost = 0.0;
    std::vector<std::pair<int, int>> _bestSolutionPath;
};