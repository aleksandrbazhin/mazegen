#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <random>
#include <chrono>
#include "mazegen.hpp"

const int TILE_SIZE = 16;
const int HEIGHT = 61;
const int WIDTH = 115;

// const int TILE_SIZE = 8;
// const int HEIGHT = 121;
// const int WIDTH = 231;

// const int TILE_SIZE = 1;
// const int HEIGHT = 961;
// const int WIDTH = 1841;

// const int TILE_SIZE = 32;
// const int HEIGHT = 25;
// const int WIDTH = 25;


sf::Texture floor_texture;

void prepare() {
    if (!floor_texture.loadFromFile("assets/cell.png"))
        return;
    floor_texture.setRepeated(true);
}


// mazegen::Grid generate_dungeon() {
//     auto gen = mazegen::Generator();
//     // gen.set_seed(101);
//     mazegen::ConstraintSet constraints {{1, 1}, {WIDTH - 2, HEIGHT - 2}};
//     return gen.generate(WIDTH, HEIGHT, constraints);
// }


// sf::VertexArray make_dungeon_map() {
    // auto grid = generate_dungeon();
    
    // sf::VertexArray map_vertices;
    // map_vertices.setPrimitiveType(sf::Quads);
    // map_vertices.resize(HEIGHT * WIDTH * 4);

    // for (int y = 0; y < grid.size(); y++) {
    //      for (int x = 0; x < grid[0].size(); x++) {
    //         if (grid[y][x] != mapgen::NOTHING_ID) {
    //             sf::Vertex* quad = &map_vertices[(x + y * WIDTH) * 4];

    //             quad[0].position = sf::Vector2f(x * TILE_SIZE, y * TILE_SIZE);
    //             quad[1].position = sf::Vector2f((x + 1) * TILE_SIZE, y * TILE_SIZE);
    //             quad[2].position = sf::Vector2f((x + 1) * TILE_SIZE, (y + 1) * TILE_SIZE);
    //             quad[3].position = sf::Vector2f(x  * TILE_SIZE, (y + 1) * TILE_SIZE);

    //             quad[0].texCoords = sf::Vector2f(0, 0);
    //             quad[1].texCoords = sf::Vector2f(TILE_SIZE, 0);
    //             quad[2].texCoords = sf::Vector2f(TILE_SIZE, TILE_SIZE);
    //             quad[3].texCoords = sf::Vector2f(0, TILE_SIZE);
    //         }
    //     }
    // }
//     return map_vertices;
// }

float ch = 0.0;

template <typename T>
std::unordered_map<int, sf::Color> get_random_region_colors(
    const std::vector<T>& regions, bool use_seed = false, int seed = 0) {

    std::mt19937 rng;
    if (use_seed) {
        rng.seed(seed);
    } else {
        std::random_device rd;
        rng.seed(rd());
    }

    std::unordered_map<int, sf::Color> color_map;

    std::uniform_int_distribution<uint8_t> distribution(0, 255);
    for (auto& region: regions) {
        color_map[region.id] = sf::Color(distribution(rng), distribution(rng), distribution(rng));
    }

    return color_map;

}

void render_game(sf::RenderWindow &window) {
    // auto map = make_dungeon_map();
    // window.clear();
    // window.draw(map, &floor_texture);
    // auto grid = generate_dungeon();

    mazegen::EXTRA_CONNECTION_CHANCE = 0.0;
    mazegen::WIGGLE_CHANCE = 0.3;
    mazegen::DEADEND_CHANCE = 0.0;

    mazegen::ROOM_NUMBER = 150;
    mazegen::ROOM_SIZE_MIN = 5;
    mazegen::ROOM_SIZE_MAX = 11;
    // mazegen::DEADEND_CHANCE = 1.0 - ch;
    // ch += 0.05;

    auto gen = mazegen::Generator();
    // gen.set_seed(101);
    mazegen::ConstraintSet constraints {{1, 1}, {WIDTH - 2, HEIGHT - 2}};
    auto grid = gen.generate(WIDTH, HEIGHT, constraints);
    auto doors = gen.get_doors();
    auto hall_colors = get_random_region_colors(gen.get_halls());
    // auto hall_colors = get_random_region_colors(gen.get_halls(), true, 101);

    sf::VertexArray map_vertices;
    map_vertices.setPrimitiveType(sf::Quads);
    map_vertices.resize(HEIGHT * WIDTH * 4);
    for (int y = 0; y < grid.size(); y++) {
         for (int x = 0; x < grid[0].size(); x++) {
            int id = grid[y][x];
            if (id  != mazegen::NOTHING_ID) {
                sf::Vertex* quad = &map_vertices[(x + y * WIDTH) * 4];

                quad[0].position = sf::Vector2f(x * TILE_SIZE, y * TILE_SIZE);
                quad[1].position = sf::Vector2f((x + 1) * TILE_SIZE, y * TILE_SIZE);
                quad[2].position = sf::Vector2f((x + 1) * TILE_SIZE, (y + 1) * TILE_SIZE);
                quad[3].position = sf::Vector2f(x  * TILE_SIZE, (y + 1) * TILE_SIZE);

                quad[0].texCoords = sf::Vector2f(0, 0);
                quad[1].texCoords = sf::Vector2f(TILE_SIZE, 0);
                quad[2].texCoords = sf::Vector2f(TILE_SIZE, TILE_SIZE);
                quad[3].texCoords = sf::Vector2f(0, TILE_SIZE);
                
                if (hall_colors.find(id) != hall_colors.end()) {
                    quad[0].color = hall_colors[id];
                    quad[1].color = hall_colors[id];
                    quad[2].color = hall_colors[id];
                    quad[3].color = hall_colors[id];
                }
            }
        }
    }
    window.clear();
    window.draw(map_vertices, &floor_texture);
    
    // sf::Font font;
    // if (font.loadFromFile("assets/RobotoMono-Bold.ttf")) {
    //     for (int y = 0; y < grid.size(); y++) {
    //         for (int x = 0; x < grid[0].size(); x++) {
    //             if (grid[y][x] != mapgen::NOTHING_ID) {
    //                 sf::Text text;
    //                 text.setPosition(x * TILE_SIZE + 2, y * TILE_SIZE + 2);
    //                 text.setFont(font); // font is a sf::Font
    //                 text.setString(std::to_string(grid[y][x]));
    //                 text.setCharacterSize(11); // in pixels, not points!
    //                 // text.setFillColor(sf::Color::Red);
    //                 // text.setStyle(sf::Text::Bold | sf::Text::Underlined);
    //                 window.draw(text);
    //             }
    //         }
    //     }
    // }

    window.display();
}


int main()
{
    prepare();
    sf::RenderWindow window(sf::VideoMode(WIDTH * TILE_SIZE, HEIGHT * TILE_SIZE), "Map");
    render_game(window);
    while (window.isOpen())
    {
        bool is_render_needed = false;
        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type)
            {
                case sf::Event::Closed:
                    window.close();
                    break;
                case sf::Event::KeyPressed:
                    if (event.key.code == sf::Keyboard::Escape) {
                        window.close();
                    } else {
                        is_render_needed = true;
                    }
                    break;
            }
        }
        if (is_render_needed) {
            std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
            render_game(window);
            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
            std::cout << "Generated a " << WIDTH << "x" << HEIGHT << " maze in " 
                << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() 
                << " ms" << std::endl;
        }
    }

    return 0;
}
