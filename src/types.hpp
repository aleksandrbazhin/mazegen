#ifndef MazeGen_types
#define MazeGen_types

#include <array>
#include <vector>
#include <set>


namespace mazegen {

struct Config {
    // Probability to not remove deadends
    float DEADEND_CHANCE = 0.3;
    // True if use reconnect_deadends() step - connect deadends adjacent to rooms with a door
    float RECONNECT_DEADENDS_CHANCE = 0.5;
    // Probability for a hall to change direction during growth
    float WIGGLE_CHANCE = 0.3;
    // All unneccessary doors are removed with this probability
    float EXTRA_CONNECTION_CHANCE = 0.3;
    // How many times room placement is attempted
    int ROOM_BASE_NUMBER = 30;
    // Room minimum dimension
    int ROOM_SIZE_MIN = 5;
    // Room maximum dimension
    int ROOM_SIZE_MAX = 7;
    // True if hall constaraints are to be exclusively in halls, not in rooms
    bool CONSTRAIN_HALL_ONLY = true;
};



// Used to iterate through neighbors to a Position
struct Direction {
    int dx = 0, dy = 0;
    bool operator==(const Direction& rhs) const {
        return dx == rhs.dx && dy == rhs.dy;
    }
    Direction operator-() const {
        return Direction{-1 * dx, -1 * dy};
    }
    Direction operator*(const int a) const {
        return Direction{a * dx, a * dy};
    }
};
typedef std::array<Direction, 4> Directions;
const Directions CARDINALS  {{ {0, -1}, {1, 0}, {0, 1}, {-1, 0} }};


// Represents a Position on a grid
struct Position {
    int x;
    int y;
    Position(int _x, int _y): x(_x), y(_y) {}
    Position(std::array<int, 2> a): x(a.at(0)), y(a.at(1)) {}
    bool operator<(const Position& another) const {
        return x < another.x || y < another.y;
    }
    bool operator==(const Position& another) const {
        return x == another.x && y == another.y;
    }
    Position neighbour_to(const Direction& d) const {
        return Position{x + d.dx, y + d.dy};
    }
};
typedef std::vector<Position> Positions;
typedef std::set<Position> PositionSet;


// Represents a room with dimensions and id
struct Room {
    Position min_point;
    Position max_point;
    int id;
    bool too_close(const Room& another, int distance) {
        return 
            min_point.x - distance < another.max_point.x 
            && max_point.x + distance > another.min_point.x 
            && min_point.y - distance < another.max_point.y 
            && max_point.y + distance > another.min_point.y;
    }
    bool has_point(const Position& point) {
        return point.x >= min_point.x && point.x <= max_point.x 
            && point.y >= min_point.y && point.y <= max_point.y;
    }
};


// Represents a hall region
struct Hall {
    Position start; // a Position belonging to this hall
    int id;
};


// Represents a door connecting a room to a hall region
struct Door {
    Position position;
    int id;
    int room_id;
    int hall_id;
    bool is_hidden = false; // the doors removed in the "reduce_connectivity" step become hidden
};


}

#endif