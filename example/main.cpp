#include <iomanip>
#include <mazegen.hpp>

int main()
{
    const int HEIGHT = 27;
    const int WIDTH = 43;

    mazegen::Config cfg;

    cfg.DEADEND_CHANCE = 0.3;
    cfg.RECONNECT_DEADENDS_CHANCE = 0.5;
    cfg.WIGGLE_CHANCE = 0.3;
    cfg.EXTRA_CONNECTION_CHANCE = 0.3;

    cfg.ROOM_BASE_NUMBER = 30;
    cfg.ROOM_SIZE_MIN = 5;
    cfg.ROOM_SIZE_MAX = 7;
    cfg.CONSTRAIN_HALL_ONLY = true;

    mazegen::PositionSet constraints {{1, 1}, {WIDTH - 2, HEIGHT - 2}};

    auto gen = mazegen::Generator();
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
                // print constraints
                std::cout << "[]";
            } else if (mazegen::is_door(region)){
                // print doors
                std::cout << "▒▒";
            } else {
                // for rooms and halls we just print last 2 digits of their ids
                std::cout << std::setw(2) << region % 100;
            }
        }
        std::cout << std::endl;
    }
    return 0;
}
