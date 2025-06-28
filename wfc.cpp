#include <iostream>
#include <vector>
#include <map>
#include <array>
#include <queue>
#include <random>
#include <ctime>
#include <algorithm>
#include <climits>

const int WIDTH = 20;
const int HEIGHT = 20;

enum Direction { UP = 0, RIGHT, DOWN, LEFT };

std::mt19937 rng(time(0));

// ----- Tile structure -----
struct Tile {
    int id;
    std::array<std::vector<int>, 4> compatible; // [UP, RIGHT, DOWN, LEFT]
};

std::map<int, float> tile_weights = {
    {1, 1.0f},  // Pasto
    {2, 1.0f},  // Tierra
    {3, 0.5f},  // Agua
    {4, 1.5f},  // Árbol
    {5, 1.0f}   // Camino
};

std::vector<Tile> tiles = {
    {1, {{ {1,2,4}, {1,2,4}, {1,2,4}, {1,2,4} }}},
    {2, {{ {1,2,5}, {1,2,5}, {1,2,5}, {1,2,5} }}},
    {3, {{ {3}, {3}, {3}, {3} }}},
    {4, {{ {1,4}, {1,4}, {1,4}, {1,4} }}},
    {5, {{ {2,5}, {2,5}, {2,5}, {2,5} }}}
};

// ----- Cell -----
struct Cell {
    std::vector<int> options;
    bool collapsed = false;
};

std::vector<std::vector<Cell>> grid(WIDTH, std::vector<Cell>(HEIGHT));

// ----- Weighted random tile selection -----
int weighted_random(const std::vector<int>& options, const std::map<int, float>& weights) {
    float total_weight = 0.0f;
    for (int opt : options)
        total_weight += weights.at(opt);

    std::uniform_real_distribution<float> dist(0.0f, total_weight);
    float r = dist(rng);

    float accum = 0.0f;
    for (int opt : options) {
        accum += weights.at(opt);
        if (r <= accum) return opt;
    }

    return options.back(); // fallback
}

// ----- Initialization -----
void initialize_grid() {
    for (int x = 0; x < WIDTH; x++)
        for (int y = 0; y < HEIGHT; y++)
            grid[x][y].options = {1, 2, 3, 4, 5};
}

// ----- Neighbors -----
std::vector<std::pair<int, int>> get_neighbors(int x, int y) {
    std::vector<std::pair<int, int>> n;
    if (y > 0) n.emplace_back(x, y - 1);     // UP
    if (x < WIDTH - 1) n.emplace_back(x + 1, y); // RIGHT
    if (y < HEIGHT - 1) n.emplace_back(x, y + 1); // DOWN
    if (x > 0) n.emplace_back(x - 1, y);     // LEFT
    return n;
}

// ----- Collapse -----
void collapse_cell(int x, int y) {
    auto& cell = grid[x][y];
    if (cell.collapsed) return;
    int chosen = weighted_random(cell.options, tile_weights);
    cell.options = {chosen};
    cell.collapsed = true;
}

// ----- Propagate -----
void propagate(int x, int y) {
    std::queue<std::pair<int, int>> q;
    q.emplace(x, y);

    while (!q.empty()) {
        auto [cx, cy] = q.front(); q.pop();
        auto& current = grid[cx][cy];

        for (int dir = 0; dir < 4; dir++) {
            int nx = cx, ny = cy;
            if (dir == UP) ny--;
            else if (dir == RIGHT) nx++;
            else if (dir == DOWN) ny++;
            else if (dir == LEFT) nx--;

            if (nx < 0 || ny < 0 || nx >= WIDTH || ny >= HEIGHT) continue;
            auto& neighbor = grid[nx][ny];
            if (neighbor.collapsed) continue;

            std::vector<int> valid_options;
            for (int option : neighbor.options) {
                for (int cur : current.options) {
                    if (std::find(tiles[cur - 1].compatible[dir].begin(),
                                  tiles[cur - 1].compatible[dir].end(), option) != tiles[cur - 1].compatible[dir].end()) {
                        valid_options.push_back(option);
                        break;
                    }
                }
            }

            if (valid_options.size() < neighbor.options.size()) {
                neighbor.options = valid_options;
                q.emplace(nx, ny);
            }
        }
    }
}

// ----- Lowest entropy -----
std::pair<int, int> find_lowest_entropy_cell() {
    int min_options = INT_MAX;
    std::vector<std::pair<int, int>> candidates;

    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            auto& cell = grid[x][y];
            if (!cell.collapsed && cell.options.size() < min_options && cell.options.size() > 0) {
                min_options = cell.options.size();
                candidates.clear();
                candidates.emplace_back(x, y);
            } else if (!cell.collapsed && cell.options.size() == min_options) {
                candidates.emplace_back(x, y);
            }
        }
    }

    if (candidates.empty()) return {-1, -1};
    return candidates[rng() % candidates.size()];
}

// ----- Map Generation -----
void run_wfc() {
    initialize_grid();

    int startX = WIDTH / 2;
    int startY = HEIGHT - 1;

    collapse_cell(startX, startY);
    propagate(startX, startY);

    while (true) {
        auto [x, y] = find_lowest_entropy_cell();
        if (x == -1) break;

        collapse_cell(x, y);
        propagate(x, y);
    }
}

void print_initial_map(int startX, int startY, int tileID) {
    std::cout << "🧩 Mapa inicial:\n";
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (x == startX && y == startY) {
                // Mostrar tile inicial en color correspondiente
                switch (tileID) {
                    case 1: std::cout << "\033[1;35m" << tileID << " \033[0m"; break; // rosa
                    case 2: std::cout << "\033[0;33m" << tileID << " \033[0m"; break; // marrón
                    case 3: std::cout << "\033[1;34m" << tileID << " \033[0m"; break; // azul
                    case 4: std::cout << "\033[1;32m" << tileID << " \033[0m"; break; // verde
                    case 5: std::cout << "\033[1;33m" << tileID << " \033[0m"; break; // amarillo
                    default: std::cout << tileID << " "; break;
                }
            } else {
                std::cout << "0 ";
            }
        }
        std::cout << std::endl;
    }
}

// ----- Print -----
void print_map() {
    std::cout << "\n🌱 Mapa generado:\n";
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (!grid[x][y].collapsed) {
                std::cout << "? ";
                continue;
            }

            int tile = grid[x][y].options[0];

            switch (tile) {
                case 1: std::cout << "\033[1;35m" << tile << " \033[0m"; break; // rosa
                case 2: std::cout << "\033[0;33m" << tile << " \033[0m"; break; // marrón
                case 3: std::cout << "\033[1;34m" << tile << " \033[0m"; break; // azul
                case 4: std::cout << "\033[1;32m" << tile << " \033[0m"; break; // verde
                case 5: std::cout << "\033[1;33m" << tile << " \033[0m"; break; // amarillo
                default: std::cout << tile << " "; break;
            }
        }
        std::cout << std::endl;
    }
}

// ----- Main -----
int main() {
    initialize_grid();

    int startX = WIDTH / 2;
    int startY = HEIGHT - 1;
    int initialTile = weighted_random({1,2,3,4,5}, tile_weights);

    // Colapsar solo el tile inicial
    grid[startX][startY].options = {initialTile};
    grid[startX][startY].collapsed = true;

    // Mostrar mapa inicial
    print_initial_map(startX, startY, initialTile);

    // Propagación y generación
    propagate(startX, startY);
    while (true) {
        auto pos = find_lowest_entropy_cell();
        int x = pos.first;
        int y = pos.second;
        if (x == -1) break;
        collapse_cell(x, y);
        propagate(x, y);
    }

    // Mostrar mapa final
    print_map();
    return 0;
}
