#pragma once

#include <vector>
#include <stack>
#include <random>
#include <iostream>

namespace mapgen {

const int NOTHING = 0;
const int FLOOR = 1;
const int ROOM_FLOOR = 2;
const int ROOM_NUMBER = 40;
const int ROOM_SIZE_MIN = 7;
const int ROOM_SIZE_MAX = 11;
const int MAX_PLACE_ATTEMPTS = 5;


typedef std::vector<std::vector<int>> Grid;
typedef std::pair<int, int> Point;
typedef std::vector<Point> Points;


struct Room
{
    int x1;
    int y1;
    int x2;
    int y2;

    bool overlaps(const Room &another) {
        return x1 < another.x2 && x2 > another.x1 && y1 < another.y2 && y2 > another.y1;
    }

    bool too_close(const Room &another, int distance) {
        return x1 - distance < another.x2 && x2 + distance > another.x1 && 
            y1 - distance < another.y2 && y2 + distance > another.y1;
    }
};

struct Direction {
    int dx = 0, dy = 0;
    Point step(const Point& p) const {
        // Point& return_point{p.first, p.second};
        return Point(p.first + dx, p.second + dy);
    }
    bool operator==(const Direction& rhs) const {
        return dx == rhs.dx && dy == rhs.dy;
    }
    Direction operator-() {
        return Direction{-1 * dx, -1 * dy};
    }
    Direction operator*(int a) {
        return Direction{a * dx, a * dy};
    }
};

struct CardinalDirections{
    std::vector<Direction> dirs {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};

    const Direction pick(int index) const {
        if (index < 0 || index > 3) {
            std::cout << "Error: wrong direction" << std::endl;
            return dirs[0];
        }
        return dirs[index];
    }
};
CardinalDirections c_dirs;

class Generator {

public:

Grid generate(int rows, int cols, const Points &constraints = {}) {
    if (rows % 2 != 1 || cols % 2 != 1) {
        std::cout << "Warning: map size should be odd" << std::endl;
    }

    grid_rows = rows;
    grid_cols = cols;
    Grid grid(grid_rows, std::vector<int>(grid_cols, NOTHING));
    std::random_device seed_gen;
    rng.seed(seed_gen());

    place_rooms(grid);
    build_maze(grid, constraints);
    return grid;
}

private:

std::mt19937 rng;
int grid_rows;
int grid_cols;

void place_rooms(Grid &grid) {
    std::uniform_int_distribution<> room_size_distribution(ROOM_SIZE_MIN, ROOM_SIZE_MAX);
    std::uniform_int_distribution<> room_position_x_distribution(0, grid_cols - ROOM_SIZE_MIN - (ROOM_SIZE_MAX - ROOM_SIZE_MIN) / 2);
    std::uniform_int_distribution<> room_position_y_distribution(0, grid_rows - ROOM_SIZE_MIN - (ROOM_SIZE_MAX - ROOM_SIZE_MIN) / 2);
    
    std::vector<Room> rooms;

    for (int i = 0; i < ROOM_NUMBER; i++) {
        bool room_is_placed = false;
        int attempts = 0;
        while (not room_is_placed && attempts <= MAX_PLACE_ATTEMPTS) {
            attempts += 1;

            int width = room_size_distribution(rng) / 2 * 2 + 1;
            int height = room_size_distribution(rng) / 2 * 2 + 1;
            int room_x = room_position_x_distribution(rng) / 2 * 2 + 1;
            int room_y = room_position_y_distribution(rng) / 2 * 2 + 1;

            if (room_x + width >= grid_cols) width = (grid_cols - room_x) / 2 * 2 - 1;
            if (room_y + height >= grid_rows) height = (grid_rows - room_y) / 2 * 2 - 1;

            Room room{room_x, room_y, room_x + width, room_y + height};
            bool too_close = false;
            for (auto another_room : rooms) {
                if (room.too_close(another_room, 1)) {
                    too_close = true;
                    break;
                }
            }
            if (too_close) continue;
            rooms.push_back(room);
            room_is_placed = true;
            for (int x = room.x1; x < room.x2; x++) {
                for (int y = room.y1; y < room.y2; y++) {
                    grid[y][x]= ROOM_FLOOR;
                }
            }
        }
    }
}

// constraints are the points which are always in the maze, never discarded
void build_maze(Grid &grid, const Points &constraints = {}) {
    Points unmet_constraints {constraints};
    while (!unmet_constraints.empty()) {
        Point p = unmet_constraints.back();
        auto [x, y] = p;
        unmet_constraints.pop_back();
        if (x % 2 != 1 || y % 2 != 1) {
            std::cout << "Error: unreachable constraint" << std::endl;
            continue;
        }
        if (grid[y][x] != NOTHING) {
            continue;
        }
        grow_maze(p, grid);
    }
    for (int x = 0; x < grid_cols / 2; x++) {
        for (int y = 0; y < grid_rows / 2; y++) {
            if (grid[y * 2 + 1][x * 2 + 1] != NOTHING) grow_maze(Point{x * 2 + 1, y * 2 + 1}, grid);
        }
    }
}

bool can_be_carved(const Grid& grid, const Point& p) {
    return p.first > 0 && p.second > 0 && 
        p.first < grid_cols && p.second < grid_rows && 
        grid[p.second][p.first] == NOTHING;
}


void grow_maze(Point start_p, Grid& grid) {
    Point p {start_p};
    Direction dir;
    if (can_be_carved(grid, p)) {
        grid[p.second][p.first] = FLOOR;
    }

    std::stack<Point> test_points;
    test_points.push(Point(p));
    std::vector<Direction> random_dirs {c_dirs.dirs};
    bool dead_end = false;

    while (!test_points.empty()) {
        std::shuffle(random_dirs.begin(), random_dirs.end(), rng);

        dead_end = true;
        for (Direction d : random_dirs) {
            Point test = (d * 2).step(p);
            if (can_be_carved(grid, test)) {
                dir = d;
                dead_end = false;
                break;
            }
        }
        if (dead_end) {
            p = test_points.top();
            test_points.pop();
        } else {
            p = dir.step(p);
            grid[p.second][p.first] = FLOOR;
            p = dir.step(p);
            grid[p.second][p.first] = FLOOR;
            test_points.push(Point(p));
        }
    }
}


};
}