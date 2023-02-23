#include <iomanip>
#include "../mazegen.hpp"

int main()
{
    const int HEIGHT = 27;
    const int WIDTH = 43;

    mazegen::Config cfg;
    cfg.ROOM_BASE_NUMBER = 25;
    cfg.ROOM_SIZE_MIN = 5;
    cfg.ROOM_SIZE_MAX = 7;
 
    cfg.EXTRA_CONNECTION_CHANCE = 0.0;
    cfg.WIGGLE_CHANCE = 0.5;
    cfg.DEADEND_CHANCE = 0.5;
    cfg.RECONNECT_DEADENDS_CHANCE = 0.5;
    
    mazegen::PointSet constraints {{1, 1}, {WIDTH - 2, HEIGHT - 2}};

    auto gen = mazegen::Generator();
    auto grid = gen.generate(WIDTH, HEIGHT, cfg, constraints);

    if (!gen.get_warnings().empty()) {
        std::cout << gen.get_warnings() << std::endl;
    }

    for (int y = 0; y < grid.size(); y++) {
        for (int x = 0; x < grid[0].size(); x++) {
            if (grid[y][x] == mazegen::NOTHING_ID) {
                std::cout << "██";
            } else if (constraints.find(mazegen::Point{x, y}) != constraints.end()) {
                std::cout << "[]";
            } else if (mazegen::is_door(grid[y][x])){
                std::cout << "▒▒";
            } else {
                std::cout << std::setw(2) << grid[y][x] % mazegen::MAX_ROOMS;
            }
        }
        std::cout << std::endl;
    }
    return 0;
}
