#pragma once

#include <vector>
#include <stack>
#include <set>
#include <random>
#include <iostream>

namespace mapgen {

const int NOTHING = 0;
const int FLOOR = 1;
const int ROOM_FLOOR = 2;
const int MAZE_FLOOR = 3;

const int ROOM_NUMBER = 40;
const int ROOM_SIZE_MIN = 7;
const int ROOM_SIZE_MAX = 11;
const int MAX_PLACE_ATTEMPTS = 5;


typedef std::vector<std::vector<int>> Grid;

struct Point {
    int x, y;
    bool operator<(const Point& another) const {
        return x < another.x || y < another.y;
    }
    bool operator==(const Point& another) const {
        return x == another.x && y == another.y;
    }
};

typedef std::vector<Point> Points;

struct Room {
    int x1;
    int y1;
    int x2;
    int y2;
    int id;

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
        // Point& return_point{p.x, p.y};
        return Point{p.x + dx, p.y + dy};
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

Grid generate(int rows, int cols, const std::set<Point> &constraints = {}) {
    if (rows % 2 != 1 || cols % 2 != 1) {
        std::cout << "Warning: map size should be odd" << std::endl;
    }
    
    grid_rows = rows;
    grid_cols = cols;
    grid = Grid(grid_rows, std::vector<int>(grid_cols, NOTHING));
    std::random_device seed_gen;
    rng.seed(seed_gen());

    place_rooms();
    build_maze(constraints);
    connect_regions();
    reduce_maze(constraints);
    return grid;
}

private:

std::mt19937 rng;
int grid_rows;
int grid_cols;
std::vector<Room> rooms;
Grid grid;
int _maze_region_id = 10000;
Points dead_ends;


void place_rooms() {
    std::uniform_int_distribution<> room_size_distribution(ROOM_SIZE_MIN, ROOM_SIZE_MAX);
    std::uniform_int_distribution<> room_position_x_distribution(0, grid_cols - ROOM_SIZE_MIN - (ROOM_SIZE_MAX - ROOM_SIZE_MIN) / 2);
    std::uniform_int_distribution<> room_position_y_distribution(0, grid_rows - ROOM_SIZE_MIN - (ROOM_SIZE_MAX - ROOM_SIZE_MIN) / 2);
    
    int room_id = 10;

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

            Room room{room_x, room_y, room_x + width - 1, room_y + height - 1};
            bool too_close = false;
            for (const auto& another_room : rooms) {
                if (room.too_close(another_room, 1)) {
                    too_close = true;
                    break;
                }
            }
            if (too_close) continue;
            
            room.id = room_id;
            rooms.push_back(room);
            room_is_placed = true;
            for (int x = room.x1; x <= room.x2; x++) {
                for (int y = room.y1; y <= room.y2; y++) {
                    grid[y][x]= room_id;
                }
            }
            room_id++;
        }
    }
}

// constraints are the points which are always in the maze, never empty
void build_maze(const std::set<Point>& constraints) {
    Points unmet_constraints(constraints.begin(), constraints.end());
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
        grow_maze(p);
    }
    for (int x = 0; x < grid_cols / 2; x++) {
        for (int y = 0; y < grid_rows / 2; y++) {
            if (grid[y * 2 + 1][x * 2 + 1] == NOTHING) grow_maze(Point{x * 2 + 1, y * 2 + 1});
        }
    }
}


bool is_in_bounds(const Point& p) {
    return p.x > 0 && p.y > 0 && p.x < grid_cols && p.y < grid_rows;
}


bool is_cell_empty(const Point& p) {
    return is_in_bounds(p) && grid[p.y][p.x] == NOTHING;
}


int get_region_id(const Point& p) {
    if (!is_in_bounds(p)) return NOTHING;
    return grid[p.y][p.x];
}


void grow_maze(Point start_p) {
    _maze_region_id++;
    Point p {start_p};
    Direction dir;
    if (is_cell_empty(p)) {
        grid[p.y][p.x] = _maze_region_id;
    }
    std::stack<Point> test_points;
    test_points.push(Point(p));
    std::vector<Direction> random_dirs {c_dirs.dirs};
    bool dead_end = false;

    while (!test_points.empty()) {
        std::shuffle(random_dirs.begin(), random_dirs.end(), rng);
        dead_end = true;
        for (Direction& d : random_dirs) {
            Point test = (d * 2).step(p);
            if (is_cell_empty(test)) {
                dir = d;
                dead_end = false;
                break;
            }
        }
        if (dead_end) {
            if (is_dead_end(p)) dead_ends.push_back(p);
            p = test_points.top();
            test_points.pop();
        } else {
            p = dir.step(p);
            grid[p.y][p.x] = _maze_region_id;
            p = dir.step(p);
            grid[p.y][p.x] = _maze_region_id;
            test_points.push(Point(p));
        }
    }
}

void add_connector(const Point& test_point, const Point& connect_point, std::map<int, Points>& connections) {
    int region_id = get_region_id(test_point);
    if (region_id != NOTHING) {
        if (connections.find(region_id) == connections.end()) {
            connections[region_id] = Points{};
        }
        connections[region_id].push_back(connect_point);
    }
}


void connect_regions() {
    if (rooms.empty()) return;
    std::set<int> connected_rooms; // prevent room double connections
    for (auto& r: rooms) {
        std::map<int, Points> connections;
        for (int x = r.x1; x <= r.x2; x += 2) {
            add_connector(Point{x, r.y1 - 2}, Point{x, r.y1 - 1}, connections);
            add_connector(Point{x, r.y2 + 2}, Point{x, r.y2 + 1}, connections);
        }
        for (int y = r.y1; y <= r.y2; y += 2) {
            add_connector(Point{r.x1 - 2, y}, Point{r.x1 - 1, y}, connections);
            add_connector(Point{r.x2 + 2, y}, Point{r.x2 + 1, y}, connections);                
        }
        // std::cout<< std::endl << "room " << r.id << ": " << r.x1 << " " << r.y1 << " " << r.x2 << " " << r.y2  << std::endl; 
        for (auto& region_connectors: connections) {
            int region_id = region_connectors.first;
            if (connected_rooms.find(region_id) != connected_rooms.end()) continue;
            std::uniform_int_distribution<> connector_distribution(0, region_connectors.second.size() - 1);
            Point p = region_connectors.second[connector_distribution(rng)];
            // std::cout << "(" << p.x << ", " << p.y << ") to " << region_id << std::endl;
            grid[p.y][p.x] = FLOOR;
        }
        connected_rooms.insert(r.id);
    }
}


bool is_dead_end(const Point& p) {
    int passways = 0;
    for (const auto& d : c_dirs.dirs) {
        Point test_p = d.step(p);
        if (get_region_id(test_p) != NOTHING) passways += 1;
    }
    return passways == 1;
}


void reduce_maze(const std::set<Point>& constraints) {
    bool done = false;
    // std::cout << dead_ends.size() << std::endl;
    for (const auto& end_p : dead_ends) {
        Point p{end_p};
        while (is_dead_end(p)) {
            if (constraints.find(p) != constraints.end()) break; // do not remove constrained points
            for (const auto& d : c_dirs.dirs) {
                Point test_point = d.step(p);
                if (get_region_id(test_point) != NOTHING) {
                    grid[p.y][p.x] = NOTHING;
                    p = test_point;
                    break;
                }
            }
        }
    }
}
};
}