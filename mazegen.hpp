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


struct Config {
    float DEADEND_CHANCE = 0.3;
    // true if use reconnect_deadends() step - connect deadends adjacent to rooms with a door
    float RECONNECT_DEADENDS_CHANCE = 1.0; 
    float WIGGLE_CHANCE = 0.5;
    float EXTRA_CONNECTION_CHANCE = 0.0;
    int ROOM_NUMBER = 20;
    int ROOM_SIZE_MIN = 7;
    int ROOM_SIZE_MAX = 11;
    int MAX_PLACE_ATTEMPTS = 5;
    // true if hall constaraints are to be exclusively in halls, not in rooms
    bool CONSTRAIN_HALL_ONLY = false;     
};


typedef std::vector<std::vector<int>> Grid;

// used to iterate neighbors aroubd point
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

// represents point on a grid
struct Point {
    int x;
    int y;
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
typedef std::set<Point> PointSet;

// represents a room
struct Room {
    Point min_point;
    Point max_point;
    int id;
    bool too_close(const Room& another, int distance) {
        return 
            min_point.x - distance < another.max_point.x 
            && max_point.x + distance > another.min_point.x 
            && min_point.y - distance < another.max_point.y 
            && max_point.y + distance > another.min_point.y;
    }
    bool has_point(const Point& point) {
        return point.x >= min_point.x && point.x <= max_point.x 
            && point.y >= min_point.y && point.y <= max_point.y;
    }
};

// represents a hall region
struct Hall {
    Point start; // a point belonging to this hall
    int id;
};

// represents a door connecting a room to a maze region
struct Door {
    Point position;
    int id;
    int room_id;
    int hall_id;
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
Grid generate(int cols, int rows, Config config, const PointSet& hall_constraints = {}) noexcept {
    cfg = config;
    clear();
    
    if (rows % 2 == 0) rows -= 1;
    if (cols % 2 == 0) cols -= 1;
    
    grid_rows = rows;
    grid_cols = cols;
    grid = Grid(grid_rows, std::vector<int>(grid_cols, NOTHING_ID));
    if (is_seed_set) {
        rng.seed(random_seed);
    } else {
        std::random_device rd;
        rng.seed(rd());
    }
    place_rooms(hall_constraints);
    build_maze(hall_constraints);
    connect_regions();
    reduce_connectivity();
    reduce_maze(hall_constraints);
    reconnect_dead_ends();
    return grid;
}


// predefines generation seed
void set_seed(unsigned int seed) noexcept {
    is_seed_set = true;
    random_seed = seed;
}

const unsigned int get_seed() const noexcept {
    return random_seed;
}

// returns region id of a point or NOTHING_ID if point is out of bounds or not in any maze region, i.e. is wall
int get_region_id(const Point& p) const noexcept {
    if (!is_in_bounds(p)) return NOTHING_ID;
    return grid[p.y][p.x];
}

const std::vector<Room> get_rooms() const noexcept {
    return rooms;
}

const std::vector<Hall> get_halls() const noexcept {
    return halls;
}

const std::vector<Door> get_doors() const noexcept {
    return doors;
}


private:

Config cfg{};

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
unsigned int random_seed;


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
void place_rooms(const PointSet &hall_constraints = {}) {
    std::uniform_int_distribution<> room_size_distribution(cfg.ROOM_SIZE_MIN, cfg.ROOM_SIZE_MAX);
    int room_avg = cfg.ROOM_SIZE_MIN + (cfg.ROOM_SIZE_MAX - cfg.ROOM_SIZE_MIN) / 2;
    std::uniform_int_distribution<> room_position_x_distribution(0, grid_cols - room_avg);
    std::uniform_int_distribution<> room_position_y_distribution(0, grid_rows - room_avg);

    for (int i = 0; i < cfg.ROOM_NUMBER; i++) {
        bool room_is_placed = false;
        int attempts = 0;
        while (not room_is_placed && attempts <= cfg.MAX_PLACE_ATTEMPTS) {
            attempts += 1;

            int width = room_size_distribution(rng) / 2 * 2 + 1;
            int height = room_size_distribution(rng) / 2 * 2 + 1;
            int room_x = room_position_x_distribution(rng) / 2 * 2 + 1;
            int room_y = room_position_y_distribution(rng) / 2 * 2 + 1;

            if (room_x + width >= grid_cols) width = (grid_cols - room_x) / 2 * 2 - 1;
            if (room_y + height >= grid_rows) height = (grid_rows - room_y) / 2 * 2 - 1;

            Room room{{room_x, room_y}, {room_x + width - 1, room_y + height - 1}, room_id};
            bool too_close = false;
            for (const auto& another_room : rooms) {
                if (room.too_close(another_room, 1)) {
                    too_close = true;
                    break;
                }
            }
            if (too_close) continue;
            if (cfg.CONSTRAIN_HALL_ONLY) {
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
            for (int x = room.min_point.x; x <= room.max_point.x; x++) {
                for (int y = room.min_point.y; y <= room.max_point.y; y++) {
                    grid[y][x]= room_id;
                }
            }
            room_id++;
        }
    }
}

// adds only those constraints that have odd x and y and are not out of grid bounds
Points fix_constraint_points(const PointSet& hall_constraints) {
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
void build_maze(const PointSet& hall_constraints) {
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
bool is_in_bounds(const Point& p) const {
    return p.x > 0 && p.y > 0 && p.x < grid_cols - 1 && p.y < grid_rows - 1;
}


bool is_cell_empty(const Point& p) const {
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

    std::set<Point> dead_ends_set{p};
    std::stack<Point> test_points;
    test_points.push(Point(p));
    std::uniform_real_distribution<double> directions_distribution(0.0, 1.0);
    Directions random_dirs {CARDINALS};
    bool dead_end = false;
    Direction dir;
    // std::cout << " Grow from ";
    // std::cout << " " << p.x  << " " << p.y << std::endl;



    while (!test_points.empty()) {
        if (directions_distribution(rng) < cfg.WIGGLE_CHANCE) {
            std::shuffle(random_dirs.begin(), random_dirs.end(), rng);
            for (auto& d: random_dirs) {
                if (d == dir) {
                    std::swap(d, random_dirs.back());
                    break;
                }
            }
        }
        dead_end = true;
        for (Direction& d : random_dirs) {
            Point test = p.neighbour_to(d * 2);
            // test for unoccupied space to grow
            if (is_cell_empty(test)) {
                dir = d;
                dead_end = false;
                break;
            }
        }
        // nowhere to grow
        if (dead_end) {
            if (is_dead_end(p)) {
                dead_ends_set.insert(p);
                // std::cout << " p " << p.x  << " " << p.y << std::endl;
            }
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
    dead_ends.insert(dead_ends.end(), dead_ends_set.begin(), dead_ends_set.end());
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
        // add all potential connectors around the room to other regions
        std::unordered_map<int, Points> connectors_map;
        for (int x = room.min_point.x; x <= room.max_point.x; x += 2) {
            add_connector(Point{x, room.min_point.y - 2}, Point{x, room.min_point.y - 1}, connectors_map);
            add_connector(Point{x, room.max_point.y + 2}, Point{x, room.max_point.y + 1}, connectors_map);
        }
        for (int y = room.min_point.y; y <= room.max_point.y; y += 2) {
            add_connector(Point{room.min_point.x - 2, y}, Point{room.min_point.x - 1, y}, connectors_map);
            add_connector(Point{room.max_point.x + 2, y}, Point{room.max_point.x + 1, y}, connectors_map);                
        }
        // select random connector from the connector map
        for (auto& [hall_id, region_connect_points]: connectors_map) {
            if (connected_rooms.find(hall_id) != connected_rooms.end()) continue;
            std::uniform_int_distribution<> connector_distribution(0, region_connect_points.size() - 1);
            Point p = region_connect_points[connector_distribution(rng)];
            grid[p.y][p.x] = ++door_id;
            doors.push_back({p, door_id, room.id, hall_id});
        }
        connected_rooms.insert(room.id);
    }
}


// returns true if a point is a dead end
bool is_dead_end(const Point& p) {
    int passways = 0;
    for (const auto& d : CARDINALS) {
        Point test_p = p.neighbour_to(d);
        if (get_region_id(test_p) != NOTHING_ID) {
            passways += 1;
        }
    }
    return passways == 1;
}


// removes blind parts of the maze with (1.0 - DEADEND_CHANCE) probability
void reduce_maze(const PointSet& hall_constraints) {

    // std::cout << "before: " << std::endl;  
    // for (auto& end_p : dead_ends) {
    //     std::cout << "(" << end_p.x << ", " << end_p.y << ") ";
    // }
    // std::cout << std::endl;

    bool done = false;
    std::uniform_real_distribution<double> reduce_distribution(0.0, 1.0);

    for (auto& end_p : dead_ends) {
        if (reduce_distribution(rng) < cfg.DEADEND_CHANCE) continue;
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
    dead_ends.erase(
        std::remove_if(
            dead_ends.begin(),
            dead_ends.end(),
            [this](Point p){return !is_dead_end(p);}),
        dead_ends.end()
    );
    // std::cout << "after: " << std::endl;
    // for (auto& end_p : dead_ends) {
    //     std::cout << "(" << end_p.x << ", " << end_p.y << ") ";
    // }
    // std::cout << std::endl;
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
        int parent_room_id = find(region_sets, door.room_id); 
        int parent_hall_id = find(region_sets, door.hall_id); 
        if (parent_room_id == parent_hall_id) {
            if (connection_chance_distribution(rng) > cfg.EXTRA_CONNECTION_CHANCE) {
                door.is_hidden = true;
                grid[door.position.y][door.position.x] = NOTHING_ID;
            }
            continue;
        }
        int min_parent = std::min(parent_room_id, parent_hall_id);
        int max_parent = std::max(parent_room_id, parent_hall_id);
        region_sets[max_parent] = min_parent;
    }
}

// if a dead end is adjacent to the room, connects it by the door
void reconnect_dead_ends() {
    std::uniform_real_distribution<double> reconnect_distribution(0, 1);
        
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
        if (reconnect_distribution(rng) >= cfg.RECONNECT_DEADENDS_CHANCE) continue;
        if (connection_number > 1) continue; // not a true dead end
        if (candidates.size() == 0) continue;

        auto& [door_p, room_id] = *candidates.begin();

        grid[door_p.y][door_p.x] = ++door_id;
        doors.push_back({door_p, door_id, room_id, hall_id});
    }
}

};
}

#endif