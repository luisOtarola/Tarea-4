#include <iostream>
#include <vector>
#include <map>
#include <array>
#include <queue>
#include <random>
#include <ctime>
#include <algorithm>
#include <climits>
#include <cmath>

// Constantes del tamaño del mapa
const int WIDTH = 20;
const int HEIGHT = 20;

// Direcciones para adyacencia
enum Direction { UP = 0, RIGHT, DOWN, LEFT };

// Generador de números aleatorios
std::mt19937 rng(time(0));

// Estructura del tile para reglas
struct Tile 
{
    int id;
    std::array<std::vector<int>, 4> compatible;
};

// Pesos para elegir aleatoriamente entre distintos tipos de tiles
std::map<int, float> tile_weights = 
{
    {1, 1.0f},  // Pasto
    {2, 1.0f},  // Camino
    {3, 0.8f},  // Agua
    {4, 0.7f},  // Árbol
    {5, 1.0f}   // Hierba
};

// Definición de las reglas
std::vector<Tile> tiles = 
{
    {1, {{{1,2,3,4,5}, {1,2,3,4,5}, {1,2,3,4,5}, {1,2,3,4,5}}}},     // Pasto
    {2, {{{1,2,5}, {1,2,5}, {1,2,5}, {1,2,5}}}},                     // Camino
    {3, {{{1,3}, {1,3}, {1,3}, {1,3}}}},                             // Agua
    {4, {{{1,4,5}, {1,4,5}, {1,4,5}, {1,4,5}}}},                     // Árbol
    {5, {{{1,4,5}, {1,4,5}, {1,4,5}, {1,4,5}}}}                      // Hierba
};

// Estructura para representar cada celda del mapa
struct Cell 
{
    std::vector<int> options; // Opciones posibles de tile
    bool collapsed = false;   // Si ya fue colapsada
};

// Mapa de celdas y coordenadas de la salida
std::vector<std::vector<Cell>> grid(WIDTH, std::vector<Cell>(HEIGHT));
std::pair<int, int> salida;

// Selección aleatoria según los pesos definidos
int weighted_random(const std::vector<int>& options, const std::map<int, float>& weights) 
{
    float total_weight = 0.0f;
    for (int opt : options)
        total_weight += weights.at(opt);

    std::uniform_real_distribution<float> dist(0.0f, total_weight);
    float r = dist(rng);
    float accum = 0.0f;

    for (int opt : options) 
    {
        accum += weights.at(opt);
        if (r <= accum) return opt;
    }
    return options.back(); // En caso de error, devuelve el ultimo tile
}

// Inicializa el mapa en 0, la entrada/salida y los bordes
void initialize_grid(int entradaX, int entradaY)
{
    std::vector<std::pair<int, int>> posibles_salidas;
    
    // Genera posibles salidas en el borde
    for (int x = 0; x < WIDTH; ++x)
        if (x != entradaX) posibles_salidas.emplace_back(x, 0);
    for (int y = 0; y < HEIGHT; ++y) 
    {
        if (y != entradaY) 
        {
            posibles_salidas.emplace_back(0, y);
            posibles_salidas.emplace_back(WIDTH - 1, y);
        }
    }
    
    // Se elige una salida aleatoriamente
    salida = posibles_salidas[rng() % posibles_salidas.size()];

    // Inicializa todas las celdas con todas las opciones posibles
    for (int x = 0; x < WIDTH; ++x) 
    {
        for (int y = 0; y < HEIGHT; ++y) 
        {
            grid[x][y].collapsed = false;
            grid[x][y].options = {1, 2, 3, 4, 5};

            // Bordes son árboles excepto en entrada/salida
            if (x == 0 || y == 0 || x == WIDTH - 1 || y == HEIGHT - 1) 
            {
                if ((x == entradaX && y == entradaY) || (x == salida.first && y == salida.second)) 
                {
                    grid[x][y].options = {2}; // Camino
                } else 
                {
                    grid[x][y].options = {4}; // Árbol
                }
                grid[x][y].collapsed = true;
            }
        }
    }
    
    // Se colapsan manualmente entrada y salida como caminos
    grid[entradaX][entradaY].options = {2};
    grid[entradaX][entradaY].collapsed = true;
    grid[salida.first][salida.second].options = {2};
    grid[salida.first][salida.second].collapsed = true;
}

// Retorna vecinos válidos de una celda
std::vector<std::pair<int, int>> get_neighbors(int x, int y) 
{
    std::vector<std::pair<int, int>> n;
    if (y > 0) n.emplace_back(x, y - 1);
    if (x < WIDTH - 1) n.emplace_back(x + 1, y);
    if (y < HEIGHT - 1) n.emplace_back(x, y + 1);
    if (x > 0) n.emplace_back(x - 1, y);
    return n;
}

// Colapsa las celdas usando pesos y aleatoriedad
void collapse_cell(int x, int y) 
{
    auto& cell = grid[x][y];
    if (cell.collapsed) return;

    std::map<int, float> local_weights = tile_weights;

    // Aumenta peso de tiles compatibles con vecinos ya colapsados
    for (auto [nx, ny] : get_neighbors(x, y)) 
    {
        auto& neighbor = grid[nx][ny];
        if (neighbor.collapsed) 
        {
            int neighbor_tile = neighbor.options[0];
            local_weights[neighbor_tile] *= 1.5f;
        }
    }

    int chosen = weighted_random(cell.options, local_weights);
    cell.options = {chosen};
    cell.collapsed = true;
}

// Propaga las restricciones a celdas adyacentes
void propagate(int x, int y) 
{
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

// Busca la celda con menor entropía, la con menos opciones
std::pair<int, int> find_low_entropy() 
{
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

// Determina si un tile es transitable, no es un árbol
bool es_transitable(int tile)
{
    return tile != 4;
}

// BFS para verificar si todas las zonas transitables están conectadas 
bool verificar_conectividad() 
{
    std::vector<std::vector<bool>> visitado(WIDTH, std::vector<bool>(HEIGHT, false));
    int total_transitables = 0;
    int startX = WIDTH / 2, startY = HEIGHT - 1;

    for (int x = 0; x < WIDTH; x++)
        for (int y = 0; y < HEIGHT; y++)
            if (es_transitable(grid[x][y].options[0])) total_transitables++;

    std::queue<std::pair<int, int>> q;
    q.emplace(startX, startY);
    visitado[startX][startY] = true;
    int visitados = 1;

    while (!q.empty()) {
        auto [x, y] = q.front(); q.pop();
        for (auto [nx, ny] : get_neighbors(x, y)) {
            if (nx < 0 || ny < 0 || nx >= WIDTH || ny >= HEIGHT) continue;
            if (visitado[nx][ny]) continue;
            if (es_transitable(grid[nx][ny].options[0])) {
                visitado[nx][ny] = true;
                q.emplace(nx, ny);
                visitados++;
            }
        }
    }

    return visitados == total_transitables;
}

// Heurística Manhattan
float heuristic(int x1, int y1, int x2, int y2) 
{
    return std::abs(x1 - x2) + std::abs(y1 - y2);
}

// A* para encontrar camino entre entrada y salida
void astar_pathfinding(int sx, int sy, int ex, int ey) 
{
    std::priority_queue<std::tuple<float, int, int, std::vector<std::pair<int,int>>>,
                        std::vector<std::tuple<float, int, int, std::vector<std::pair<int,int>>>>,
                        std::greater<>> pq;
    pq.emplace(0.0f, sx, sy, std::vector<std::pair<int,int>>{});
    std::vector<std::vector<bool>> visited(WIDTH, std::vector<bool>(HEIGHT, false));

    while (!pq.empty()) {
        auto [cost, x, y, path] = pq.top(); pq.pop();
        if (visited[x][y]) continue;
        visited[x][y] = true;
        path.emplace_back(x, y);

        if (x == ex && y == ey) {
            for (auto& [px, py] : path) {
                grid[px][py].options = {2};  // Marca como camino
                grid[px][py].collapsed = true;
            }
            return;
        }

        for (auto [nx, ny] : get_neighbors(x, y)) {
            if (nx < 0 || ny < 0 || nx >= WIDTH || ny >= HEIGHT) continue;
            if (visited[nx][ny]) continue;
            if (!es_transitable(grid[nx][ny].options[0])) continue;
            float new_cost = cost + 1.0f + heuristic(nx, ny, ex, ey);
            pq.emplace(new_cost, nx, ny, path);
        }
    }
}

// Imprime el mapa con colores
void print_map() 
{
    std::cout << "\n Mapa generado:\n";
    for (int y = 0; y < HEIGHT; y++) 
    {
        for (int x = 0; x < WIDTH; x++) 
        {
            int tile = grid[x][y].options[0];
            switch (tile) 
            {
                case 1: std::cout << "\033[1;35m" << tile << " \033[0m"; break; // Rosa
                case 2: std::cout << "\033[0;33m" << tile << " \033[0m"; break; // Marrón
                case 3: std::cout << "\033[1;34m" << tile << " \033[0m"; break; // Azul
                case 4: std::cout << "\033[1;32m" << tile << " \033[0m"; break; // Verde
                case 5: std::cout << "\033[1;33m" << tile << " \033[0m"; break; // Amarillo
                default: std::cout << tile << " "; break;
            }
        }
        std::cout << std::endl;
    }
}


int main() 
{
    bool exito = false;

    while (!exito) {
        int entradaX = WIDTH / 2;
        int entradaY = HEIGHT - 1;

        // Inicializa y colapsa entrada/salida
        initialize_grid(entradaX, entradaY);
        propagate(entradaX, entradaY);
        propagate(salida.first, salida.second);

        // Colapsa y propaga hasta que todas las celdas estén definidas
        while (true) 
        {
            auto pos = find_low_entropy();
            if (pos.first == -1) break;
            collapse_cell(pos.first, pos.second);
            propagate(pos.first, pos.second);
        }

        // Verifica conectividad y genera camino con A*
        if (verificar_conectividad()) 
        {
            astar_pathfinding(entradaX, entradaY, salida.first, salida.second);
            std::cout << "\n Todas las zonas transitables están conectadas.\n";
            exito = true;
        } else 
        {
            std::cout << "\n Mapa no transitable, reintentando...\n";
        }
    }

    print_map();
    return 0;
}
