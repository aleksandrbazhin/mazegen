#include <iomanip>
#include <mazegen.hpp>

class MyGrid {

};


namespace mazegen{

template<>
MyGrid init_grid(int width, int height) {
    return MyGrid();
}

template<>
void clear_grid(MyGrid& grid) {
    
}

template<>
int maze_width(const MyGrid& grid) {
    return 0;
}

template<>
int maze_height(const MyGrid& grid) {
    return 0;
}

template<>
bool is_wall(const MyGrid& grid, int x, int y) {
    return false;
}

template<>
int get_region(const MyGrid& grid, int x, int y) {
    return mazegen::NOTHING_ID;
}

template<>
bool set_region(MyGrid& grid, int x, int y, int id) {
    return true;
}
}


int main()
{
    const int HEIGHT = 27;
    const int WIDTH = 43;

    mazegen::Config cfg;
    cfg.ROOM_SIZE_MIN = 3;
    cfg.ROOM_SIZE_MAX = 9;

    mazegen::PositionSet constraints {{1, 1}, {WIDTH - 2, HEIGHT - 2}};
    auto gen = mazegen::Generator<MyGrid>();
    // auto gen = mazegen::Generator();
    // gen.set_seed(0);
    auto grid = gen.generate(WIDTH, HEIGHT, cfg, constraints);

    if (!gen.get_warnings().empty()) {
        std::cout << gen.get_warnings() << std::endl;
    }

    for (int y = 0; y < mazegen::maze_height(grid); y++) {
        for (int x = 0; x < mazegen::maze_width(grid); x++) {
            int region = mazegen::get_region(grid, x, y);
            if (region == mazegen::NOTHING_ID) {
                std::cout << "██";
            } else if (constraints.find(mazegen::Position{x, y}) != constraints.end()) {
                std::cout << "[]"; // print constraints
            } else if (mazegen::is_door(region)){
                std::cout << "▒▒"; // print doors
            } else {
                std::cout << std::setw(2) << region % 100; // print last 2 digits of region ids
            }
        }
        std::cout << std::endl;
    }
    std::cout << "Generated maze with seed " << gen.get_seed() << std::endl;
    return 0;
}
