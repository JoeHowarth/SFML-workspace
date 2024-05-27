// TODO: Known bugs:
// - Asteroids are not actually Convex, so the collision detection is not
// perfect

#include <SFML/Graphics.hpp>
#include <chrono>
#include <filesystem>
#include <iomanip>  // Include this header for std::fixed and std::setprecision
#include <iostream>
#include <random>
#include <sstream>
#include <utility>

#include "util.hpp"

const float shipAcceleration = 0.1f;
const float bulletVelocity = 5;
enum AsteroidSize { SMALL, MEDIUM, BIG };

long frame = 0;

template <typename... Args>
void print_frame(Args&&... args) {
  if (frame % 60 != 0) {
    return;
  }
  std::stringstream ss;
  (ss << ...
      << std::forward<Args>(
             args));  // Fold expression to handle multiple arguments
  std::cout << ss.str() << std::endl;
}

struct Asteroid;

sf::ConvexShape makeShip();
sf::ConvexShape makeBigAsteroid(sf::Vector2f position);
sf::ConvexShape makeRandomAsteroid(sf::Vector2f position, AsteroidSize size);
sf::ConvexShape makeBullet(sf::Vector2f position, float rotation);
bool isPointInsideConvexPolygon(const sf::ConvexShape& polygon,
                                const sf::Vector2f& P);
void applyVelocityToObject(sf::ConvexShape& shape, const sf::Vector2f& velocity,
                           const sf::Vector2f& viewSize);
std::vector<Asteroid> generateAsteroids(int count, float minX, float maxX,
                                        float minY, float maxY);

struct Ship {
  sf::ConvexShape shape;
  sf::Vector2f velocity;

  Ship() : velocity(0, 0) { this->shape = makeShip(); }
};

struct Asteroid {
  uint id;
  sf::ConvexShape shape;
  sf::Vector2f velocity;
  AsteroidSize size;

  static inline int NEXT_ID = 0;
  static inline int SMALL_RADIUS = 20;
  static inline int MED_RADIUS = 50;
  static inline int BIG_RADIUS = 100;
  static inline int NUM_POINTS = 8;

  Asteroid(sf::Vector2f position, sf::Vector2f velocity, AsteroidSize size)
      : id(NEXT_ID++), velocity(velocity), size(size) {
    this->shape = makeRandomAsteroid(position, size);
  }

  bool isPointInsideAsteroid(const sf::Vector2f& P, sf::RenderWindow& window) {
    sf::Vector2f center = this->shape.getPosition();
    auto Pc = P - center;  // vector from center of asteroid to point
    float mag = magnitude(Pc);

    drawLine(window, center, P);
    drawPoint(window, P);

    if (mag > 150) {
      return false;
    }
    float angle = std::atan2(Pc.y, Pc.x);
    float angleIncrement = 2 * 3.14159265358979323846f / NUM_POINTS;
    int preVertexInd = angle / angleIncrement;
    int nextVertexInd = (preVertexInd + 1) % NUM_POINTS;
    float t = angle - angleIncrement * preVertexInd;
    auto preV = this->shape.getPoint(preVertexInd);
    auto nextV = this->shape.getPoint(nextVertexInd);
    auto onCurve = lerp(preV, nextV, t);
    float r = magnitude(onCurve);

    if (std::abs(std::atan2(onCurve.y, onCurve.x) - angle) > 0.1) {
      std::cout << "OnCurve angle is off" << std::endl;
      return false;
    }

    if (mag < r) {
      std::cout << "Inside!" << std::endl;
      return true;
    }
    std::cout << "Outside :(" << std::endl;
    return false;
  }

  sf::ConvexShape makeRandomAsteroid(sf::Vector2f position, AsteroidSize size) {
    sf::ConvexShape shape;
    const float pi = 3.14159265358979323846f;
    float angleIncrement = 2 * pi / NUM_POINTS;
    shape.setPointCount(NUM_POINTS);

    float radius;
    switch (size) {
      case SMALL:
        radius = 20;
        break;
      case MEDIUM:
        radius = 50;
        break;
      case BIG:
        radius = 100;
        break;
    }
    for (int i = 0; i < NUM_POINTS; ++i) {
      float angle = i * angleIncrement;
      float r = radius;  //+ randomFloat(-radius / 3, radius / 3);
      float x = r * std::cos(angle);
      float y = r * std::sin(angle);
      shape.setPoint(i, {x, y});
    }

    shape.setFillColor(sf::Color::Black);
    shape.setOutlineColor(sf::Color::White);
    shape.setOutlineThickness(1);
    shape.setPosition(position);

    return shape;
  }
};

std::ostream& operator<<(std::ostream& os, const Asteroid& asteroid) {
  os << "Asteroid " << asteroid.id << " at " << asteroid.shape.getPosition()
     << " with velocity " << asteroid.velocity;
  return os;
}

struct Bullet {
  sf::ConvexShape shape;
  sf::Vector2f velocity;
  float range;

  Bullet(sf::Vector2f pos, float rotation) : range(1000) {
    this->shape = makeBullet(pos, rotation);
    this->velocity = move_forward(rotation, bulletVelocity);
  }
};

/*
 * MAIN
 */
int main() {
  auto window = sf::RenderWindow{{1920u, 1080u}, "CMake SFML Project"};
  window.setFramerateLimit(144);
  sf::Vector2u windowSize = window.getSize();
  sf::View view;
  view.setCenter(0, 0);
  view.setSize(static_cast<float>(windowSize.x),
               static_cast<float>(windowSize.y));
  window.setView(view);
  sf::Vector2f viewSize = view.getSize();

  TextDrawer textDrawer(view, "../../open-sans/OpenSans-Regular.ttf");

  Ship ship;
  std::vector<Asteroid> asteroids;
  std::vector<Bullet> bullets;
  uint score = 0;
  std::vector<int> bulletsToRemove;
  std::vector<int> asteroidsToRemove;
  std::vector<Asteroid> asteroidsToAdd;

  int newRoundFrame = 0;

  while (window.isOpen()) {
    window.clear(sf::Color::Black);
    textDrawer.clear();

    if (asteroids.size() == 0) {
      if (newRoundFrame == frame) {
        asteroids = generateAsteroids(1, -viewSize.x / 2, viewSize.x / 2,
                                      -viewSize.y / 2, viewSize.y / 2);
        bullets.clear();
        ship.shape.setPosition(0, 0);
        ship.velocity = {0, 0};
      }
      if (newRoundFrame < frame) {
        newRoundFrame = frame + 100;
      }
    }

    for (auto event = sf::Event{}; window.pollEvent(event);) {
      switch (event.type) {
        case sf::Event::Closed:
          window.close();
          break;
        case sf::Event::KeyPressed:
          switch (event.key.code) {
            case sf::Keyboard::Escape:
              window.close();
              break;
            case sf::Keyboard::Space:
              // Shoot
              bullets.push_back(
                  Bullet(ship.shape.getPosition(), ship.shape.getRotation()));
              break;
            case sf::Keyboard::E:
              if (asteroids[0].isPointInsideAsteroid(ship.shape.getPosition(),
                                                     window)) {
                textDrawer.draw(ship.shape.getPosition() + vec(20, 20),
                                "Inside!");
                dbg("Inside!");
              } else {
                dbg("Outside :(");
                textDrawer.draw(ship.shape.getPosition() + vec(20, 20),
                                "Outside :(");
              }
            default:
              break;
          }
          break;
        default:
          break;
      }
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
      ship.velocity += move_forward(ship.shape.getRotation(), shipAcceleration);
    } else if (std::abs(ship.velocity.x) > 0 || std::abs(ship.velocity.y) > 0) {
      // Decelerate ship smoothly to a standstill
      if (std::abs(ship.velocity.x) > 0 || std::abs(ship.velocity.y) > 0) {
        ship.velocity +=
            normalize(ship.velocity) *
            -std::min(shipAcceleration / 2, magnitude(ship.velocity));
      }
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
      ship.shape.rotate(-2);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
      ship.shape.rotate(2);
    }

    // Wrap Objects around the screen
    for (auto& asteroid : asteroids) {
      applyVelocityToObject(asteroid.shape, asteroid.velocity, viewSize);
    }
    applyVelocityToObject(ship.shape, ship.velocity, viewSize);
    for (int i = 0; i < bullets.size(); ++i) {
      auto& bullet = bullets[i];
      applyVelocityToObject(bullet.shape, bullet.velocity, viewSize);
      bullet.range -= magnitude(bullet.velocity);
      print_frame("Bullet Range: ", bullet.range);
      if (bullet.range <= 0) {
        print("Bullet out of range");
        bulletsToRemove.push_back(i);
      }
    }

    // Detect collisions between bullets and asteroids
    for (int i = 0; i < bullets.size(); ++i) {
      auto& bullet = bullets[i];

      print_frame("Bullet Position: ", bullet.shape.getPosition());

      for (int j = 0; j < asteroids.size(); ++j) {
        auto& asteroid = asteroids[j];
        print_frame("Checking Asteroid ", asteroid);

        if (isPointInsideConvexPolygon(asteroid.shape,
                                       bullet.shape.getPosition())) {
          print("Hit!");
          switch (asteroid.size) {
            case BIG:
              score += 25;
              // asteroidsToAdd.push_back(Asteroid(
              //     asteroid.shape.getPosition() + randomVector2f(-5, 5, -5,
              //     5), randomVector2f(-1, 1, -1, 1), MEDIUM));
              // asteroidsToAdd.push_back(Asteroid(
              //     asteroid.shape.getPosition() + randomVector2f(-5, 5, -5,
              //     5), randomVector2f(-1, 1, -1, 1), MEDIUM));
              break;
            case MEDIUM:
              score += 50;
              // asteroidsToAdd.push_back(Asteroid(
              //     asteroid.shape.getPosition() + randomVector2f(-1, 1, -1,
              //     1), randomVector2f(-1, 1, -1, 1), SMALL));
              // asteroidsToAdd.push_back(Asteroid(
              //     asteroid.shape.getPosition() + randomVector2f(-2, 2, -2,
              //     2), randomVector2f(-1, 1, -1, 1), SMALL));
              break;
            case SMALL:
              score += 100;
              break;
          }

          // Mark the bullet and asteroid for removal
          bulletsToRemove.push_back(i);
          asteroidsToRemove.push_back(j);
          break;
        }
      }
    }

    // Sort indices in descending order before removal to avoid invalidating
    // indices
    std::sort(bulletsToRemove.rbegin(), bulletsToRemove.rend());
    std::sort(asteroidsToRemove.rbegin(), asteroidsToRemove.rend());

    for (int i : bulletsToRemove) {
      print("Removing bullet");
      bullets.erase(bullets.begin() + i);
    }
    for (int i : asteroidsToRemove) {
      print("Removing asteroid");
      asteroids.erase(asteroids.begin() + i);
    }
    if (asteroidsToAdd.size() > 0) {
      print("Adding ", asteroidsToAdd.size(), " asteroids");
      asteroids.insert(asteroids.end(),
                       std::make_move_iterator(asteroidsToAdd.begin()),
                       std::make_move_iterator(asteroidsToAdd.end()));
    }

    // Clear the vectors of bullets and asteroids to remove
    bulletsToRemove.clear();
    asteroidsToRemove.clear();
    asteroidsToAdd.clear();

    /*
     * Draw the objects
     */

    // Draw Bullets
    for (auto& bullet : bullets) {
      window.draw(bullet.shape);
    }

    // Draw Asteroids
    for (auto& asteroid : asteroids) {
      print_frame(asteroid);
      window.draw(asteroid.shape);

      textDrawer.draw(asteroid.shape.getPosition(), "ID: ", asteroid.id,
                      " Pos: ", asteroid.shape.getPosition());
    }
    print_frame("");

    // Draw Ship
    window.draw(ship.shape);
    textDrawer.draw(ship.shape.getPosition(), "Pos: ", ship.shape.getPosition(),
                    "\nVel: ", ship.velocity);

    // Draw score
    sf::RectangleShape scoreRect(sf::Vector2f(200, 50));
    scoreRect.setPosition(-viewSize.x / 2 + 1 + 15, -viewSize.y / 2 + 1 + 15);
    scoreRect.setFillColor(sf::Color::Black);
    scoreRect.setOutlineColor(sf::Color(100, 100, 100));
    scoreRect.setOutlineThickness(1);
    window.draw(scoreRect);

    textDrawer.draw(scoreRect.getPosition() + vec(75, 20), "Score: ", score);
    textDrawer.draw(scoreRect.getPosition() + vec(75, 30), "Frame: ", frame);
    textDrawer.draw(scoreRect.getPosition() + vec(75, 40),
                    "NewRoundFrame: ", newRoundFrame);

    textDrawer.display(window);
    window.display();
    ++frame;
  }
}

// Function to check if the point P is inside the convex polygon defined by a
// sf::ConvexShape
bool isPointInsideConvexPolygon(const sf::ConvexShape& polygon,
                                const sf::Vector2f& P) {
  int n = polygon.getPointCount();
  if (n < 3) return false;  // A polygon must have at least 3 vertices

  auto& trans = polygon.getTransform();

  sf::Vector2f prevVertex = trans.transformPoint(polygon.getPoint(n - 1));
  sf::Vector2f firstVertex = trans.transformPoint(polygon.getPoint(0));
  bool initialSign =
      crossProduct(firstVertex - prevVertex, P - prevVertex) >= 0;

  for (int i = 0; i < n; ++i) {
    sf::Vector2f currentVertex = trans.transformPoint(polygon.getPoint(i));
    sf::Vector2f nextVertex =
        trans.transformPoint(polygon.getPoint((i + 1) % n));
    if (crossProduct(nextVertex - currentVertex, P - currentVertex) >= 0 !=
        initialSign) {
      return false;
    }
  }

  return true;
}

sf::ConvexShape makeShip() {
  sf::ConvexShape ship;
  ship.setPointCount(3);
  ship.setPoint(0, sf::Vector2f(0, -10));
  ship.setPoint(1, sf::Vector2f(7, 10));
  ship.setPoint(2, sf::Vector2f(-7, 10));
  ship.setFillColor(sf::Color::Black);
  ship.setOutlineColor(sf::Color::White);
  ship.setOutlineThickness(1);

  return ship;
}

sf::ConvexShape makeBullet(sf::Vector2f position, float rotation) {
  sf::ConvexShape bullet;
  bullet.setPointCount(4);
  bullet.setPoint(0, sf::Vector2f(0, 0));
  bullet.setPoint(1, sf::Vector2f(2, 0));
  bullet.setPoint(2, sf::Vector2f(2, 4));
  bullet.setPoint(3, sf::Vector2f(0, 4));
  bullet.setFillColor(sf::Color::White);
  bullet.setPosition(position.x, position.y);
  bullet.setRotation(rotation);
  return bullet;
}

void applyVelocityToObject(sf::ConvexShape& shape, const sf::Vector2f& velocity,
                           const sf::Vector2f& viewSize) {
  shape.move(velocity);
  if (shape.getPosition().x < -viewSize.x / 2) {
    print("Wrapping X, pos: ", shape.getPosition());
    shape.setPosition(viewSize.x / 2, shape.getPosition().y);
    print("Wrapped  X, pos: ", shape.getPosition());
  }
  if (shape.getPosition().x > viewSize.x / 2) {
    print("Wrapping X, pos: ", shape.getPosition());
    shape.setPosition(-viewSize.x / 2, shape.getPosition().y);
    print("Wrapped  X, pos: ", shape.getPosition());
  }
  if (shape.getPosition().y < -viewSize.y / 2) {
    print("Wrapping Y, pos: ", shape.getPosition());
    shape.setPosition(shape.getPosition().x, viewSize.y / 2);
    print("Wrapped Y, pos: ", shape.getPosition());
  }
  if (shape.getPosition().y > viewSize.y / 2) {
    print("Wrapping Y, pos: ", shape.getPosition());
    shape.setPosition(shape.getPosition().x, -viewSize.y / 2);
    print("Wrapped Y, pos: ", shape.getPosition());
  }
}

std::vector<Asteroid> generateAsteroids(int count, float minX, float maxX,
                                        float minY, float maxY) {
  std::vector<Asteroid> asteroids;
  for (int i = 0; i < count; ++i) {
    sf::Vector2f position = randomVector2f(minX, maxX, minY, maxY);
    std::cout << "Asteroid Position: " << position.x << ", " << position.y
              << std::endl;
    asteroids.push_back(Asteroid(position, randomVector2f(-1, 1, -1, 1), BIG));
  }
  return asteroids;
}
