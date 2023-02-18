#ifndef MazeGen
#define MazeGen

#include <vector>
#include <stack>
#include <set>
#include <map>
#include <unordered_map>
#include <array>
#include <random>
#include <iostream>
#include <algorithm>


namespace mazegen {

const int NOTHING_ID = -1;
const int DOOR_ID_START = 2000000;
const int ROOM_ID_START = 1000000;
const int MAZE_ID_START = 0;

static float DEADEND_CHANCE = 0.3;
static float WIGGLE_CHANCE = 0.5;
static float EXTRA_CONNECTION_CHANCE = 0.2;
static int ROOM_NUMBER = 20;
static int ROOM_SIZE_MIN = 7;
static int ROOM_SIZE_MAX = 11;
static int MAX_PLACE_ATTEMPTS = 5;
static bool CONSTRAINTS_HALL_ONLY = false;


typedef std::vector<std::vector<int>> Grid;


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

struct Point {
    int x, y;
    bool operator<(const Point& another) const {
        return x < another.x || y < another.y;
    }
    bool operator==(const Point& another) const {
        return x == another.x && y == another.y;
    }
    Point neighbour_to(const Direction& d) const {
        return Point{x + d.dx, y + d.dy};
    }
};

typedef std::vector<Point> Points;
typedef std::set<Point> ConstraintSet;

struct Room {
    int x1;
    int y1;
    int x2;
    int y2;
    int id;    

    bool too_close(const Room& another, int distance) {
        return x1 - distance < another.x2 && x2 + distance > another.x1 && 
            y1 - distance < another.y2 && y2 + distance > another.y1;
    }

    bool has_point(const Point& point) {
        return point.x >= x1 && point.x <= x2 && point.y >= y1 && point.y <= y2;
    }
};


struct Hall {
    Point start; // a point belonging to this hall
    int id;
};


struct Door {
    Point position;
    int id;
    int id_from;
    int id_to;
    bool is_hidden = false; // the doors removed in the "reduce_connectivity" step become hidden
};


inline bool is_hall(int id) {
    return id >= MAZE_ID_START && id < ROOM_ID_START;
}


inline bool is_room(int id) {
    return id >= ROOM_ID_START && id < DOOR_ID_START;
}


inline bool is_door(int id) {
    return id >= DOOR_ID_START;
}


// class to generate the maze
class Generator {

public:

// constraints are between (1, 1) and (rows - 2, cols - 2)
// those points are fixed on the generation
Grid generate(int cols, int rows, const ConstraintSet &hall_constraints = {}) {
    clear();
    
    if (rows % 2 == 0) rows -= 1;
    if (cols % 2 == 0) cols -= 1;
    
    grid_rows = rows;
    grid_cols = cols;
    grid = Grid(grid_rows, std::vector<int>(grid_cols, NOTHING_ID));
    if (is_seed_set) {
        rng.seed(predefined_seed);
    } else {
        std::random_device rd;
        rng.seed(rd());
    }
    place_rooms(hall_constraints);
    build_maze(hall_constraints);
    connect_regions();
    reduce_maze(hall_constraints);
    reduce_connectivity();
    reconnect_dead_ends();
    return grid;
}


// predefines generation seed
void set_seed(unsigned int seed) {
    is_seed_set = true;
    predefined_seed = seed;
}


// returns region id of a point or NOTHING_ID if point is out of bounds or not in any maze region, i.e. is wall
int get_region_id(const Point& p) {
    if (!is_in_bounds(p)) return NOTHING_ID;
    return grid[p.y][p.x];
}

const std::vector<Room> get_rooms() const {
    return rooms;
}

const std::vector<Hall> get_halls() const {
    return halls;
}

const std::vector<Door> get_doors() const {
    return doors;
}


private:

std::vector<Room> rooms;
std::vector<Door> doors;
std::vector<Hall> halls;


std::mt19937 rng;
int grid_rows;
int grid_cols;
Grid grid;
int maze_region_id = MAZE_ID_START;
int room_id = ROOM_ID_START;
int door_id = DOOR_ID_START;
Points dead_ends;


bool is_seed_set = false;
unsigned int predefined_seed;


// clears all the generated data
void clear() {
    grid.clear();
    rooms.clear();
    halls.clear();
    doors.clear();
    maze_region_id = MAZE_ID_START;
    room_id = ROOM_ID_START;
    door_id = DOOR_ID_START;
}


// places the rooms randomly
void place_rooms(const ConstraintSet &hall_constraints = {}) {
    std::uniform_int_distribution<> room_size_distribution(ROOM_SIZE_MIN, ROOM_SIZE_MAX);
    std::uniform_int_distribution<> room_position_x_distribution(0, grid_cols - ROOM_SIZE_MIN - (ROOM_SIZE_MAX - ROOM_SIZE_MIN) / 2);
    std::uniform_int_distribution<> room_position_y_distribution(0, grid_rows - ROOM_SIZE_MIN - (ROOM_SIZE_MAX - ROOM_SIZE_MIN) / 2);

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

            Room room{room_x, room_y, room_x + width - 1, room_y + height - 1, room_id};
            bool too_close = false;
            for (const auto& another_room : rooms) {
                if (room.too_close(another_room, 1)) {
                    too_close = true;
                    break;
                }
            }
            if (too_close) continue;
            if (CONSTRAINTS_HALL_ONLY) {
                bool breaks_hall_constraint = false;
                for (const Point& point: hall_constraints) {
                    if (room.has_point(point)) {
                        breaks_hall_constraint = true;
                        break;
                    }  
                }
                if (breaks_hall_constraint) continue;
            }
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

// adds only those constraints that have odd x and y and are not out of grid bounds
Points fix_constraint_points(const ConstraintSet& hall_constraints) {
    Points constraints;
    constraints.reserve(hall_constraints.size());
    for (auto& constraint: hall_constraints) {
        if (!is_in_bounds(constraint)) {
            std::cout << "Warning! Constraint (" << constraint.x << ", " <<  constraint.y << 
                ") is out of grid bounds (should be in [1, n - 2]), skipping" << std::endl;
        } else if (constraint.x % 2 == 0 || constraint.y % 2 == 0) {
            std::cout << "Warning! Constraint (" << constraint.x << ", " <<  constraint.y << 
                ") must have odd x and y, skipping" << std::endl;
        } else {
            constraints.push_back(constraint);
        }
    }
    return constraints;
}


// constraints are the points which are always in the maze, never empty
void build_maze(const ConstraintSet& hall_constraints) {
    Points unmet_constraints = fix_constraint_points(hall_constraints);
    // first grow from the constraints
    while (!unmet_constraints.empty()) {
        Point p = unmet_constraints.back();
        auto [x, y] = p;
        unmet_constraints.pop_back();
        if (x % 2 != 1 || y % 2 != 1) {
            std::cout << "Error: unreachable constraint" << std::endl;
            continue;
        }
        if (grid[y][x] != NOTHING_ID) {
            continue;
        }
        grow_maze(p);
    }
    // then from all the empty points 
    for (int x = 0; x < grid_cols / 2; x++) {
        for (int y = 0; y < grid_rows / 2; y++) {
            if (grid[y * 2 + 1][x * 2 + 1] == NOTHING_ID) grow_maze({x * 2 + 1, y * 2 + 1});
        }
    }
}


// returns true if point is inside the maze boundaries
bool is_in_bounds(const Point& p) {
    return p.x > 0 && p.y > 0 && p.x < grid_cols - 1 && p.y < grid_rows - 1;
}


bool is_cell_empty(const Point& p) {
    return is_in_bounds(p) && grid[p.y][p.x] == NOTHING_ID;
}


// grows a hall from the point
// maze_region_id is an id of an interconnected part of the maze
void grow_maze(Point start_p) {
    Point p {start_p};
    if (!is_cell_empty(p)) {
        return;
    }
    ++maze_region_id;
    halls.push_back({p, maze_region_id});

    grid[p.y][p.x] = maze_region_id;
    std::stack<Point> test_points;
    test_points.push(Point(p));
    std::uniform_real_distribution<double> dir_chance_distribution(0, 1);
    Directions random_dirs {CARDINALS};
    bool dead_end = false;
    Direction dir;
    while (!test_points.empty()) {
        if (dir_chance_distribution(rng) < WIGGLE_CHANCE)
            std::shuffle(random_dirs.begin(), random_dirs.end(), rng);
        dead_end = true;
        for (Direction& d : random_dirs) {
            Point test = p.neighbour_to(d * 2);
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
            p = p.neighbour_to(dir);
            grid[p.y][p.x] = maze_region_id;
            p = p.neighbour_to(dir);
            grid[p.y][p.x] = maze_region_id;
            test_points.push(Point(p));
        }
    }
}

// adding potential doors
void add_connector(const Point& test_point, const Point& connect_point, 
        std::unordered_map<int, Points>& connections) {
    int region_id = get_region_id(test_point);
    if (region_id != NOTHING_ID) {
        if (connections.find(region_id) == connections.end()) {
            connections[region_id] = Points{};
        }
        connections[region_id].push_back(connect_point);
    }
}


// connects rooms to the adjacent halls at least once for each maze region
void connect_regions() {
    if (rooms.empty()) return;
    std::set<int> connected_rooms; // prevent room double connections to the same region
    for (auto& room: rooms) {
        std::unordered_map<int, Points> connections;
        for (int x = room.x1; x <= room.x2; x += 2) {
            add_connector(Point{x, room.y1 - 2}, Point{x, room.y1 - 1}, connections);
            add_connector(Point{x, room.y2 + 2}, Point{x, room.y2 + 1}, connections);
        }
        for (int y = room.y1; y <= room.y2; y += 2) {
            add_connector(Point{room.x1 - 2, y}, Point{room.x1 - 1, y}, connections);
            add_connector(Point{room.x2 + 2, y}, Point{room.x2 + 1, y}, connections);                
        }
        for (auto& region_connectors: connections) {
            int region_id = region_connectors.first;
            if (connected_rooms.find(region_id) != connected_rooms.end()) continue;
            std::uniform_int_distribution<> connector_distribution(0, region_connectors.second.size() - 1);
            Point p = region_connectors.second[connector_distribution(rng)];
            grid[p.y][p.x] = ++door_id;
            doors.push_back({p, door_id, room.id, region_id});
        }
        connected_rooms.insert(room.id);
    }
}


// returns true if a point is a dead end
bool is_dead_end(const Point& p) {
    int passways = 0;
    for (const auto& d : CARDINALS) {
        Point test_p = p.neighbour_to(d);
        if (get_region_id(test_p) != NOTHING_ID) passways += 1;
    }
    return passways == 1;
}


// removes blind parts of the maze with (1.0 - DEADEND_CHANCE) probability
void reduce_maze(const ConstraintSet& hall_constraints) {
    bool done = false;
    std::uniform_real_distribution<double> reduce_distribution(0, 1);
    for (auto& end_p : dead_ends) {
        if (reduce_distribution(rng) < DEADEND_CHANCE) continue;
        Point p{end_p};
        while (is_dead_end(p)) {
            if (hall_constraints.find(p) != hall_constraints.end()) break; // do not remove constrained points
            for (const auto& d : CARDINALS) {
                Point test_point = p.neighbour_to(d);
                if (get_region_id(test_point) != NOTHING_ID) {
                    grid[p.y][p.x] = NOTHING_ID;
                    p = test_point;
                    break;
                }
            }
        }
        end_p.x = p.x;
        end_p.y = p.y;
    }
}


// disjoint sets (union-find)
int find(const std::unordered_map<int, int>& regions, int region_id) {
    if (regions.at(region_id) == region_id) {
        return region_id;
    } 
    return find(regions, regions.at(region_id));
}


// deletes duplicate doors created previously with (1.0 - EXTRA_CONNECTION_CHANCE) probability
void reduce_connectivity() {
    std::unordered_map<int, int> region_sets;
    for (const auto& room: rooms) {
        region_sets[room.id] = room.id;
    }
    for (const auto& hall: halls) {
        region_sets[hall.id] = hall.id;
    }

    std::uniform_real_distribution<double> connection_chance_distribution(0, 1);
    
    for (Door& door: doors) {
        int parent_from = find(region_sets, door.id_from); 
        int parent_to = find(region_sets, door.id_to); 
        if (parent_from == parent_to) {
            if (connection_chance_distribution(rng) > EXTRA_CONNECTION_CHANCE) {
                door.is_hidden = true;
                grid[door.position.y][door.position.x] = NOTHING_ID;
            }
            continue;
        }
        int min_parent = std::min(parent_from, parent_to);
        int max_parent = std::max(parent_from, parent_to);
        region_sets[max_parent] = min_parent;
    }
}

// if a dead end is adjacent to the room, connects it by the door
void reconnect_dead_ends() {
    for (const Point& dead_end: dead_ends) {
        int hall_id = get_region_id(dead_end);
        if (hall_id == NOTHING_ID) continue;
        std::map<Point, int> candidates;
        int connection_number = 0;
        for (const Direction& dir : CARDINALS) {
            Point test_p = dead_end.neighbour_to(dir * 2);
            int neighbor_id = get_region_id(test_p);
            if (neighbor_id != NOTHING_ID) {
                Point door_p {dead_end.x + dir.dx, dead_end.y + dir.dy};
                if (get_region_id(door_p) != NOTHING_ID) {
                    ++connection_number;
                    continue;
                }
                if (hall_id == neighbor_id) continue;
                candidates[door_p] = neighbor_id;
            }
        }
        if (connection_number > 1) continue; // not a true dead end
        if (candidates.size() == 0) continue;

        auto& [door_p, neighbor_id] = *candidates.begin();

        grid[door_p.y][door_p.x] = ++door_id;
        doors.push_back({door_p, door_id, hall_id, neighbor_id});
    }
}

};
}

#endif