#include <SFML/Graphics.hpp>
#include "map_gen_lib.hpp"

const int TILE_SIZE = 16;
const int ROWS = 63;
const int COLS = 115;

sf::Texture floor_texture;

void prepare() {
    if (!floor_texture.loadFromFile("assets/cell.png"))
        return;
    floor_texture.setRepeated(true);
}


mapgen::Grid generate_dungeon() {
    auto gen = mapgen::Generator();
    // mapgen::PointSet constraints ;
    return gen.generate(ROWS, COLS, {{1, 1}, {11, 11}});
}


sf::VertexArray make_dungeon_map() {
    auto grid = generate_dungeon();
    
    sf::VertexArray map_vertices;
    map_vertices.setPrimitiveType(sf::Quads);
    map_vertices.resize(ROWS * COLS * 4);

    for (int y = 0; y < grid.size(); y++) {
         for (int x = 0; x < grid[0].size(); x++) {
            if (grid[y][x] != mapgen::NOTHING_ID) {
                sf::Vertex* quad = &map_vertices[(x + y * COLS) * 4];

                quad[0].position = sf::Vector2f(x * TILE_SIZE, y * TILE_SIZE);
                quad[1].position = sf::Vector2f((x + 1) * TILE_SIZE, y * TILE_SIZE);
                quad[2].position = sf::Vector2f((x + 1) * TILE_SIZE, (y + 1) * TILE_SIZE);
                quad[3].position = sf::Vector2f(x  * TILE_SIZE, (y + 1) * TILE_SIZE);

                quad[0].texCoords = sf::Vector2f(0, 0);
                quad[1].texCoords = sf::Vector2f(TILE_SIZE, 0);
                quad[2].texCoords = sf::Vector2f(TILE_SIZE, TILE_SIZE);
                quad[3].texCoords = sf::Vector2f(0, TILE_SIZE);
            }
        }
    }
    return map_vertices;
}


void render_game(sf::RenderWindow &window) {
    auto map = make_dungeon_map();
    window.clear();
    window.draw(map, &floor_texture);
    window.display();
}


int main()
{
    prepare();
    sf::RenderWindow window(sf::VideoMode(COLS * TILE_SIZE, ROWS * TILE_SIZE), "Map");
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
            render_game(window);
        }
    }

    return 0;
}
