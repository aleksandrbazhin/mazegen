#include <iomanip>
#include <mazegen.hpp>

int main()
{
    const int HEIGHT = 27;
    const int WIDTH = 43;

    mazegen::Config cfg;
    cfg.ROOM_SIZE_MIN = 3;
    cfg.ROOM_SIZE_MAX = 9;

    mazegen::PositionSet constraints {{1, 1}, {WIDTH - 2, HEIGHT - 2}};
    auto gen = mazegen::Generator();
    // gen.set_seed(0);
    auto grid = gen.generate(WIDTH, HEIGHT, cfg, constraints);

    if (!gen.get_warnings().empty()) {
        std::cout << gen.get_warnings() << std::endl;
    }

    for (int y = 0; y < mazegen::maze_height(grid); y++) {
        for (int x = 0; x < mazegen::maze_width(grid); x++) {
            int region_id = mazegen::get_region(grid, x, y);
            if (region_id == mazegen::NOTHING_ID) {
                std::cout << "██";
            } else if (constraints.find(mazegen::Position{x, y}) != constraints.end()) {
                std::cout << "[]"; // print constraints
            } else if (mazegen::is_id_door(region_id)){
                std::cout << "▒▒"; // print doors
            } else {
                std::cout << std::setw(2) << region_id % 100; // print last 2 digits of region ids
            }
        }
    }
    std::cout << "Generated maze with seed " << gen.get_seed() << std::endl;
    return 0;
}
