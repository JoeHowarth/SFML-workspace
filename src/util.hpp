#include <SFML/Graphics.hpp>
#include <chrono>
#include <iomanip>  // Include this header for std::fixed and std::setprecision
#include <iostream>
#include <random>
#include <sstream>
#include <utility>

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
    return sf::Vector2f(0, 0);  // Avoid division by zero
  }
  return sf::Vector2f(v.x / mag, v.y / mag);
}

float randomFloat(float min, float max) {
  static std::default_random_engine engine;  // Default random engine
  std::uniform_real_distribution<float> dist(min, max);
  return dist(engine);
}

// Function to generate a random sf::Vector2f within the given range
sf::Vector2f randomVector2f(float minX, float maxX, float minY, float maxY) {
  return sf::Vector2f(randomFloat(minX, maxX), randomFloat(minY, maxY));
}

sf::Vector2f lerp(const sf::Vector2f& a, const sf::Vector2f& b, float t) {
  return a + (b - a) * t;
}

/* Printing */

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

void drawLine(sf::RenderWindow& window, const sf::Vector2f& start,
              const sf::Vector2f& end) {
  sf::Vertex line[] = {
      sf::Vertex(start),
      sf::Vertex(end),
  };
  window.draw(line, 2, sf::Lines);
}

void drawPoint(sf::RenderWindow& window, const sf::Vector2f& point) {
  sf::CircleShape circle(2);
  circle.setPosition(point);
  circle.setFillColor(sf::Color::Red);
  window.draw(circle);
}

sf::Text makeText(const std::string& str, const sf::Vector2f& pos,
                  sf::Font& font) {
  sf::Text text;
  text.setFont(font);
  text.setString(str);
  text.setCharacterSize(12);
  text.setFillColor(sf::Color::White);
  text.setPosition(pos);
  return text;
}

template <typename... Args>
void drawText(sf::RenderTexture& window, const sf::Vector2f& pos,
              sf::Font& font, Args&&... args) {
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

struct TextDrawer {
  sf::Font font;
  sf::RenderTexture texture;
  sf::Sprite sprite;

  TextDrawer(sf::View& view, const std::string& fontPath)
      : font(loadFont(fontPath)) {
    texture.create(static_cast<unsigned int>(view.getSize().x),
                   static_cast<unsigned int>(view.getSize().y));
    texture.setView(view);
    texture.setSmooth(false);
    sprite.setTexture(texture.getTexture());
    sprite.setPosition(view.getCenter() - view.getSize() / 2.f);
  }

  template <typename... Args>
  void draw(const sf::Vector2f& pos, Args&&... args) {
    drawText(this->texture, pos, this->font, std::forward<Args>(args)...);
  }

  void clear() { texture.clear(sf::Color::Transparent); }
  void display(sf::RenderWindow& window) {
    texture.display();
    window.draw(sprite);
  }
};

auto now() { return std::chrono::high_resolution_clock::now(); }

sf::Vector2f vec(float x, float y) { return {x, y}; }