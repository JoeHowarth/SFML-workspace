#include <SFML/Graphics.hpp>
#include <chrono>
#include <iomanip>  // Include this header for std::fixed and std::setprecision
#include <iostream>
#include <random>
#include <sstream>
#include <utility>

/**** Math ****/

sf::Vector2f vec(float x, float y) { return {x, y}; }

float to_radians(float degrees) { return degrees * (3.14159265f / 180.f); }
sf::Vector2f move_forward(float degrees, float distance) {
  float radians = to_radians(degrees - 90);
  return {std::cos(radians) * distance, std::sin(radians) * distance};
}

float crossProduct(const sf::Vector2f& v1, const sf::Vector2f& v2) {
  return v1.x * v2.y - v1.y * v2.x;
}

// Function to calculate the magnitude of a vector
float magnitude(const sf::Vector2f& v) {
  return std::sqrt(v.x * v.x + v.y * v.y);
}

// Function to normalize a vector (get the unit vector)
sf::Vector2f normalize(const sf::Vector2f& v) {
  float mag = magnitude(v);
  if (mag == 0) {
    return {0, 0};  // Avoid division by zero
  }
  return {v.x / mag, v.y / mag};
}

float randomFloat(float min, float max) {
  static std::default_random_engine engine;  // Default random engine
  std::uniform_real_distribution<float> dist(min, max);
  return dist(engine);
}

// Function to generate a random sf::Vector2f within the given range
sf::Vector2f randomVector2f(float minX, float maxX, float minY, float maxY) {
  return {randomFloat(minX, maxX), randomFloat(minY, maxY)};
}

sf::Vector2f lerp(const sf::Vector2f& a, const sf::Vector2f& b, float t) {
  return a + (b - a) * t;
}

/**** Printing ****/

template <typename... Args>
void print(Args&&... args) {
  std::stringstream ss;
  (ss << ...
      << std::forward<Args>(
             args));  // Fold expression to handle multiple arguments
  std::cout << ss.str() << std::endl;
}

template <typename T>
T dbg(const T s) {
  std::cout << s << std::endl;
  return s;
}

namespace sf {
std::ostream& operator<<(std::ostream& os, const Vector2f& vec) {
  os << std::fixed << std::setprecision(1);
  os << "(" << std::setw(6) << vec.x << ", " << std::setw(6) << vec.y << ")";
  return os;
}
}  // namespace sf

/**** Drawing ******/

sf::Text makeText(const std::string& str, const sf::Vector2f& pos,
                  sf::Font& font, uint8_t size = 12) {
  sf::Text text;
  text.setFont(font);
  text.setString(str);
  text.setCharacterSize(size);
  text.setFillColor(sf::Color::White);
  text.setPosition(pos);
  return text;
}

template <typename Renderable, typename... Args>
void drawText(Renderable& window, const sf::Vector2f& pos, sf::Font& font,
              Args&&... args) {
  std::stringstream ss;
  (ss << ...
      << std::forward<Args>(
             args));  // Fold expression to handle multiple arguments
  window.draw(makeText(ss.str(), pos, font));
}

sf::Font loadFont(const std::string& path) {
  sf::Font font;
  if (!font.loadFromFile(path)) {
    throw std::runtime_error("Failed to load font: " + path +
                             "\n. Current working directory: " +
                             std::filesystem::current_path().string() + "\n");
  }
  return font;
}

struct Line : public sf::Drawable {
  const std::unique_ptr<sf::Vertex[]> vertices;
  const std::size_t vertexCount;
  const sf::PrimitiveType type;

  Line(const sf::Vector2f start, const sf::Vector2f end)
      : vertices(std::unique_ptr<sf::Vertex[]>{new sf::Vertex[2]{start, end}}),
        // std::make_unique<sf::Vertex[]>( 2) with per-element initialization
        vertexCount(2),
        type(sf::Lines) {}

  Line(const sf::Vertex* vertices, std::size_t vertexCount,
       sf::PrimitiveType type)
      : vertices(std::make_unique<sf::Vertex[]>(vertexCount)),
        vertexCount(vertexCount),
        type(type) {
    for (std::size_t i = 0; i < vertexCount; i++) {
      this->vertices[i] = vertices[i];
    }
  }

  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {
    sf::Vertex* verticesP = this->vertices.get();
    target.draw(verticesP, vertexCount, type, states);
  }
};

struct LayeredDrawer {
  std::vector<std::vector<std::unique_ptr<sf::Drawable>>> layers;

  LayeredDrawer(int numLayers = 1) : layers(numLayers) {}

  void draw(std::unique_ptr<sf::Drawable> drawable, int layer = 0) {
    this->layers[layer].push_back(std::move(drawable));
  }

  void draw(const sf::Vertex* vertices, std::size_t vertexCount,
            sf::PrimitiveType type,
            const sf::RenderStates& states = sf::RenderStates::Default,
            int layer = 0) {
    this->layers[layer].push_back(
        std::make_unique<Line>(vertices, vertexCount, type));
  }

  void line(const sf::Vector2f start, const sf::Vector2f end, int layer = 0) {
    this->layers[layer].push_back(std::make_unique<Line>(start, end));
  }

  void point(const sf::Vector2f point, int layer = 0) {
    std::unique_ptr<sf::CircleShape> circle =
        std::make_unique<sf::CircleShape>(2);
    circle->setPosition(point);
    circle->setFillColor(sf::Color::Red);
    this->layers[layer].push_back(
        std::unique_ptr<sf::Drawable>(std::move(circle)));
  }

  void display(sf::RenderWindow& window) {
    for (auto& layer : this->layers) {
      for (const auto& drawable : layer) {
        window.draw(*drawable);
      }
      layer.clear();
    }
  }
};

struct TextDrawer {
  struct Text {
    sf::Vector2f pos;
    std::string str;
    uint8_t size = 12;
  };

  struct Opts {
    uint8_t size = 12;
    sf::Vector2f pos;
  };

  sf::Font font;
  std::vector<Text> texts;

  TextDrawer(const std::string& fontPath) : font(loadFont(fontPath)) {}

  template <typename... Args>
  void draw(const sf::Vector2f& pos, Args&&... args) {
    std::stringstream ss;
    (ss << ... << std::forward<Args>(args));
    this->texts.push_back({pos, ss.str()});
  }

  template <typename... Args>
  void draw(const Opts& opts, Args&&... args) {
    std::stringstream ss;
    (ss << ... << std::forward<Args>(args));
    this->texts.push_back(
        {.str = ss.str(), .pos = opts.pos, .size = opts.size});
  }

  void display(sf::RenderWindow& window) {
    for (const auto& text : this->texts) {
      window.draw(makeText(text.str, text.pos, font, text.size));
    }
    this->texts.clear();
  }
};

auto now() { return std::chrono::high_resolution_clock::now(); }
