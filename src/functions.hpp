#ifndef MazeGen_functions
#define MazeGen_functions

#include "types.hpp"

namespace mazegen {


const int NOTHING_ID = -1;
const int MAX_ROOMS = 1000000;
const int HALL_ID_START = 0;
const int ROOM_ID_START = MAX_ROOMS;
const int DOOR_ID_START = MAX_ROOMS * 2;

// These 8 functions have to be reimplemented for generator to be able to create custom grid types


// constructs and returns the grid
template<typename TGrid>
inline TGrid init_grid(int width, int height) {
    return TGrid(height, std::vector<int>(width, NOTHING_ID));
}

template<typename TGrid>
inline void clear_grid(TGrid& grid) {
    grid.clear();
}

template<typename TGrid>
inline int maze_height(const TGrid& grid) {
    return grid.size();
}

template<typename TGrid>
inline int maze_width(const TGrid& grid) {
    return grid.empty() ? 0 : grid.front().size();
}

// Returns true if Position is inside the maze boundaries
template<typename TGrid>
inline bool is_in_bounds(const TGrid& grid, int x, int y) {
    return x > 0 && y > 0 && x < maze_width(grid) - 1 && y < maze_height(grid) - 1;
}

// Returns true if (x, y) does not have halls or rooms or doors, but is inside the grid
template<typename TGrid>
inline bool is_wall(const TGrid& grid, int x, int y) {
    return is_in_bounds(grid, x, y) && grid[y][x] == NOTHING_ID;
}

// returns region id at (x, y) or NOTHING_ID if (x, y) is out of bounds or is a wall
template<typename TGrid>
inline int get_region(const TGrid& grid, int x, int y) {
    if (!is_in_bounds(grid, x, y)) return NOTHING_ID;
    return grid[y][x];
}

// sets a region id at (x, y) 
template<typename TGrid>
inline bool set_region(TGrid& grid, int x, int y, int id) {
    if (!is_in_bounds(grid, x, y)) return false;
    grid[y][x] = id;
    return true;
}

inline bool is_id_hall(int id) {
    return id >= HALL_ID_START && id < ROOM_ID_START;
}
inline bool is_id_room(int id) {
    return id >= ROOM_ID_START && id < DOOR_ID_START;
}
inline bool is_id_door(int id) {
    return id >= DOOR_ID_START;
}


//anonymous ns to hide some internals
namespace {

template<typename TGrid>
bool is_in_bounds(const TGrid& grid, const Position& p) {
    return mazegen::is_in_bounds(grid, p.x, p.y);
}
template<typename TGrid>
bool is_wall(const TGrid& grid, const Position& p) {
    return mazegen::is_wall(grid, p.x, p.y);
}
template<typename TGrid>
bool is_empty(const TGrid& grid, const Position& p) {
    return !is_in_bounds(grid, p) || is_wall(grid, p);
}
// returns region id of a Position or NOTHING_ID if Position is out of bounds or not in any maze region, i.e. is wall
template<typename TGrid>
int get_region(const TGrid& grid, const Position& p) {
    return mazegen::get_region(grid, p.x, p.y);
}
template<typename TGrid>
bool set_region(TGrid& grid, const Position& p, int id) {
    return mazegen::set_region(grid, p.x, p.y, id);
}

template<typename TGrid>
Config fix_config(const TGrid& grid, const Config& user_config) {
    Config fixed{user_config};
    if (fixed.DEADEND_CHANCE < 0.0f || fixed.DEADEND_CHANCE > 1.0f ||
            fixed.RECONNECT_DEADENDS_CHANCE < 0.0f || fixed.RECONNECT_DEADENDS_CHANCE > 1.0f ||
            fixed.WIGGLE_CHANCE < 0.0f || fixed.WIGGLE_CHANCE > 1.0f ||
            fixed.EXTRA_CONNECTION_CHANCE < 0.0f || fixed.DEADEND_CHANCE > 1.0f) {
        // warnings.append("Warning! All chances should be between 0.0f and 1.0f. Fixed by clamping.\n");
    }
    if (fixed.ROOM_BASE_NUMBER >= MAX_ROOMS || fixed.ROOM_BASE_NUMBER < 0) {
        if (fixed.ROOM_BASE_NUMBER >= MAX_ROOMS) fixed.ROOM_BASE_NUMBER = MAX_ROOMS - 1;
        if (fixed.ROOM_BASE_NUMBER < 0) fixed.ROOM_BASE_NUMBER = 0;
        // warnings.append("Warning! ROOM_BASE_NUMBER must belong to[0, " + std::to_string(MAX_ROOMS - 1) + "]. Fixed by clamping.\n");
    }
    if (fixed.ROOM_SIZE_MIN % 2 == 0 || fixed.ROOM_SIZE_MAX % 2 == 0) {
        if (fixed.ROOM_SIZE_MIN % 2 == 0) fixed.ROOM_SIZE_MIN -= 1; 
        if (fixed.ROOM_SIZE_MAX % 2 == 0) fixed.ROOM_SIZE_MAX -= 1; 
        // warnings.append("Warning! ROOM_SIZE_MIN and ROOM_SIZE_MAX must be odd. Fixed by subtracting 1.\n");
    }
    int min_dimension = std::min(maze_width(grid), maze_height(grid));
    if (fixed.ROOM_SIZE_MIN > min_dimension || fixed.ROOM_SIZE_MAX > min_dimension) {
        if (fixed.ROOM_SIZE_MIN > min_dimension) fixed.ROOM_SIZE_MIN = min_dimension;
        if (fixed.ROOM_SIZE_MAX > min_dimension) fixed.ROOM_SIZE_MAX = min_dimension;
        // warnings.append("Warning! ROOM_SIZE_MIN and ROOM_SIZE_MAX must less than both width and height of the maze. Fixed.\n");
    }

    if (fixed.ROOM_SIZE_MIN < 0 || fixed.ROOM_SIZE_MAX < 0 || fixed.ROOM_SIZE_MAX < fixed.ROOM_SIZE_MIN) {
        if (fixed.ROOM_SIZE_MIN < 0) fixed.ROOM_SIZE_MIN = 0;
        if (fixed.ROOM_SIZE_MAX < 0) fixed.ROOM_SIZE_MAX = 0;
        if (fixed.ROOM_SIZE_MAX < fixed.ROOM_SIZE_MIN) fixed.ROOM_SIZE_MAX = fixed.ROOM_SIZE_MIN;
        // warnings.append("Warning! ROOM_SIZE_MIN and ROOM_SIZE_MAX must be > 0 and ROOM_SIZE_MAX must be >= ROOM_SIZE_MIN. Fixed.\n");
    }
    return fixed;
}

// makes sure maze width and height are odd and >= 3
std::pair<int, int> fix_boundaries(int width, int height) {
    if (width % 2 == 0 || height % 2 == 0) {
        // warnings.append("Warning! Maze height and width must be odd! Fixed by subtracting 1.\n"
        // );
        if (width % 2 == 0) width -= 1;
        if (height % 2 == 0) height -= 1;
    }
    if (width < 3 || height < 3) {
        // warnings.append("Warning! Maze height and width must be >= 3! Fixed by increasing to 3.\n");
        if (width < 3) width = 3;
        if (height < 3 ) height = 3;
    }
    return std::make_pair(width, height);    
}


// Adds only those constraints that have odd x and y and are not out of grid bounds
template<typename TGrid>
PositionSet fix_constraint_Positions(const TGrid& grid, const PositionSet& hall_constraints) {
    PositionSet constraints;
    for (auto& constraint: hall_constraints) {
        if (!is_in_bounds(grid, constraint)) {
            // warnings.append("Warning! Constraint (" 
            //     + std::to_string(constraint.x) + ", " + std::to_string(constraint.y) 
            //     + ") is out of grid bounds. Skipped.\n"
            // );
        } else if (constraint.x % 2 == 0 || constraint.y % 2 == 0) {
            // warnings.append("Warning! Constraint (" 
                // + std::to_string(constraint.x) + ", " + std::to_string(constraint.y) 
                // + ") must have odd x and y. Skipped.\n"
            // );
        } else {
            constraints.insert(constraint);
        }
    }
    return constraints;
}

}

}

#endif