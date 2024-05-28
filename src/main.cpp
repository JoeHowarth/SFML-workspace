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

LayeredDrawer drawer(1);
long frame = 0;

struct Asteroid {
  enum AsteroidSize { SMALL, MEDIUM, BIG };

  uint id;
  sf::ConvexShape shape;
  sf::Vector2f velocity;
  AsteroidSize size;

  static inline int NEXT_ID = 0;
  static inline int SMALL_RADIUS = 20;
  static inline int MED_RADIUS = 50;
  static inline int BIG_RADIUS = 100;
  static inline int NUM_POINTS = 8;

  Asteroid(sf::Vector2f position, sf::Vector2f velocity, AsteroidSize size);

  bool isPointInsideAsteroid(const sf::Vector2f& P, bool debug = true);

  sf::ConvexShape makeRandomAsteroid(sf::Vector2f position, AsteroidSize size);
};

struct Ship {
  sf::ConvexShape shape;
  sf::Vector2f velocity;

  Ship();
};

struct Bullet {
  sf::ConvexShape shape;
  sf::Vector2f velocity;
  float range;

  Bullet(sf::Vector2f pos, float rotation);
};

bool isPointInsideConvexPolygon(const sf::ConvexShape& polygon,
                                const sf::Vector2f& P);
void applyVelocityToObject(sf::ConvexShape& shape, const sf::Vector2f& velocity,
                           const sf::Vector2f& viewSize);
std::vector<Asteroid> generateAsteroids(int count, float minX, float maxX,
                                        float minY, float maxY);
bool isPointInsideRadialPolygon(const sf::Vector2f& P, sf::ConvexShape poly,
                                float magLimit = 200, bool debug = false);
float normalizeAngle(float angle);

template <typename... Args>
void print_frame(Args&&... args) {
  if (frame % 60 != 0) {
    return;
  }
  print(args...);
}

/*
 * MAIN
 */
int main() {
  auto window = sf::RenderWindow{{1920u, 1080u}, "Asteroids"};
  window.setFramerateLimit(144);
  sf::Vector2u windowSize = window.getSize();
  sf::View view;
  view.setCenter(0, 0);
  view.setSize(static_cast<float>(windowSize.x),
               static_cast<float>(windowSize.y));
  window.setView(view);
  sf::Vector2f viewSize = view.getSize();

  TextDrawer textDrawer("../../open-sans/OpenSans-Regular.ttf");

  Ship ship;
  std::vector<Asteroid> asteroids;
  std::vector<Bullet> bullets;
  uint score = 0;
  std::vector<int> bulletsToRemove;
  std::vector<int> asteroidsToRemove;
  std::vector<Asteroid> asteroidsToAdd;

  int newRoundFrame = 0;
  int resetFrame = -1;
  int numAsteroids = 5;
  bool debug = false;

  while (window.isOpen()) {
    window.clear(sf::Color::Black);

    if (resetFrame == frame) {
      newRoundFrame = frame;
      score = 0;
      numAsteroids = 5;
      asteroids.clear();
    }
    if (resetFrame > frame) {
      sf::RectangleShape gameOverRect(sf::Vector2f(300, 110));
      gameOverRect.setPosition({-150, -40});
      gameOverRect.setFillColor({30, 30, 35, 240});
      // gameOverRect.setOutlineColor(sf::Color(100, 100, 100));
      // gameOverRect.setOutlineThickness(1);
      drawer.draw(std::make_unique<sf::RectangleShape>(gameOverRect));

      textDrawer.draw({.pos = vec(-100, -30), .size = 24}, "Game Over!");
      textDrawer.draw({.pos = vec(-100, 0), .size = 24}, "Score: ", score);
      textDrawer.draw({.pos = vec(-100, 30), .size = 24}, "Press R to restart");
      if (sf::Keyboard::isKeyPressed(sf::Keyboard::R)) {
        resetFrame = frame + 1;
      }
    } else if (asteroids.size() == 0) {
      if (newRoundFrame == frame) {
        numAsteroids += 2;
        asteroids =
            generateAsteroids(numAsteroids, -viewSize.x / 2, viewSize.x / 2,
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
            case sf::Keyboard::Q:
              debug = !debug;
              break;
            default:
              break;
          }
          break;
        default:
          break;
      }
    }

    // Update the ship's velocity based on input
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
    // Debugging key to check if the ship is inside an asteroid
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::E)) {
      if (asteroids[0].isPointInsideAsteroid(ship.shape.getPosition()), true) {
        textDrawer.draw(ship.shape.getPosition() + vec(20, 20), "Inside!");
      } else {
        textDrawer.draw(ship.shape.getPosition() + vec(20, 20), "Outside :(");
      }
    }

    // Wrap Objects around the screen
    for (auto& asteroid : asteroids) {
      applyVelocityToObject(asteroid.shape, asteroid.velocity, viewSize);
    }

    applyVelocityToObject(ship.shape, ship.velocity, viewSize);

    for (int i = 0; i < bullets.size(); ++i) {
      auto& bullet = bullets[i];
      applyVelocityToObject(bullet.shape, bullet.velocity, viewSize);

      // Update bullet range
      bullet.range -= magnitude(bullet.velocity);
      if (bullet.range <= 0) {
        bulletsToRemove.push_back(i);
      }
    }

    // Detect collision between ship and asteroids
    if (resetFrame < frame) {
      bool shouldReset = false;
      for (int i = 0; i < asteroids.size(); ++i) {
        auto& asteroid = asteroids[i];
        for (int j = 0; j < 3; ++j) {
          auto pt = ship.shape.getPoint(j);
          pt = ship.shape.getTransform().transformPoint(pt);
          if (isPointInsideRadialPolygon(pt, asteroid.shape,
                                         Asteroid::BIG_RADIUS * 2, debug)) {
            shouldReset = true;
            break;
          }
        }
      }
      // Reset the game if the ship is hit by an asteroid
      if (shouldReset) {
        print("Ship hit by asteroid!");
        bullets.clear();
        resetFrame = frame + 300;
      }
    }

    // Detect collisions between bullets and asteroids
    for (int i = 0; i < bullets.size(); ++i) {
      auto& bullet = bullets[i];

      print_frame("Bullet Position: ", bullet.shape.getPosition());

      for (int j = 0; j < asteroids.size(); ++j) {
        auto& asteroid = asteroids[j];
        print_frame("Checking Asteroid ", asteroid);

        if (asteroid.isPointInsideAsteroid(bullet.shape.getPosition(), debug)) {
          print("Hit!");
          switch (asteroid.size) {
            case Asteroid::BIG:
              score += 20;
              asteroidsToAdd.push_back(Asteroid(
                  asteroid.shape.getPosition() + randomVector2f(-5, 5, -5, 5),
                  asteroid.velocity + randomVector2f(-1, 1, -1, 1), Asteroid::MEDIUM));
              asteroidsToAdd.push_back(Asteroid(
                  asteroid.shape.getPosition() + randomVector2f(-5, 5, -5, 5),
                  asteroid.velocity + randomVector2f(-1, 1, -1, 1), Asteroid::MEDIUM));
              break;
            case Asteroid::MEDIUM:
              score += 50;
              asteroidsToAdd.push_back(Asteroid(
                  asteroid.shape.getPosition() + randomVector2f(-1, 1, -1, 1),
                  asteroid.velocity + randomVector2f(-1, 1, -1, 1), Asteroid::SMALL));
              asteroidsToAdd.push_back(Asteroid(
                  asteroid.shape.getPosition() + randomVector2f(-2, 2, -2, 2),
                  asteroid.velocity + randomVector2f(-1, 1, -1, 1), Asteroid::SMALL));
              break;
            case Asteroid::SMALL:
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
      bullets.erase(bullets.begin() + i);
    }
    for (int i : asteroidsToRemove) {
      asteroids.erase(asteroids.begin() + i);
    }
    if (asteroidsToAdd.size() > 0) {
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

      if (debug) {
        textDrawer.draw(asteroid.shape.getPosition(), "ID: ", asteroid.id,
                        " Pos: ", asteroid.shape.getPosition());
      }
    }
    print_frame("");

    // Draw Ship
    window.draw(ship.shape);

    // Draw score
    sf::RectangleShape scoreRect(sf::Vector2f(200, 50));
    scoreRect.setPosition(-viewSize.x / 2 + 1 + 15, -viewSize.y / 2 + 1 + 15);
    scoreRect.setFillColor(sf::Color::Black);
    scoreRect.setOutlineColor(sf::Color(100, 100, 100));
    scoreRect.setOutlineThickness(1);
    window.draw(scoreRect);

    textDrawer.draw(scoreRect.getPosition() + vec(75, 20), "Score: ", score);

    drawer.display(window);
    textDrawer.display(window);
    window.display();
    ++frame;
  }
}

// Moves the shape by the velocity and wraps it around the screen
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

// Generates count number of asteroids with random positions and velocities
std::vector<Asteroid> generateAsteroids(int count, float minX, float maxX,
                                        float minY, float maxY) {
  std::vector<Asteroid> asteroids;
  for (int i = 0; i < count; ++i) {
    auto pos = vec(0, 0);
    // ensure the asteroid is not too close to the ship
    while (magnitude(pos) < 200) {
      pos = randomVector2f(minX, maxX, minY, maxY);
    }
    asteroids.emplace_back(pos, randomVector2f(-1, 1, -1, 1), Asteroid::BIG);
  }
  return asteroids;
}

// Function to normalize an angle to the range [0, 2 * pi)
float normalizeAngle(float angle) {
  std::fmod(angle, 2 * M_PI);
  if (angle < 0) {
    angle += 2 * M_PI;
  }
  return angle;
}

// Function to check if the point P is inside the regular radial polygon
// Note: all vertices must have equal angles between them
bool isPointInsideRadialPolygon(const sf::Vector2f& P, sf::ConvexShape poly,
                                float magLimit, bool debug) {
  if (poly.getPointCount() < 3) {
    return false;
  }
  auto center = poly.getPosition();
  auto Pc = P - center;  // vector from center of poly to point
  float Pc_mag = magnitude(Pc);

  if (Pc_mag > magLimit) {
    return false;
  }

  if (debug) {
    drawer.line(center, P);
    drawer.point(P);
  }

  float Pc_angle = normalizeAngle(std::atan2(Pc.y, Pc.x));
  float angleIncrement = 2 * M_PI / poly.getPointCount();
  int preVertexInd = Pc_angle / angleIncrement;
  int nextVertexInd = (preVertexInd + 1) % poly.getPointCount();
  float t = (Pc_angle - angleIncrement * preVertexInd) / angleIncrement;
  auto transform = poly.getTransform();
  auto preV = transform.transformPoint(poly.getPoint(preVertexInd));
  auto nextV = transform.transformPoint(poly.getPoint(nextVertexInd));
  auto onCurve = lerp(preV, nextV, t) - center;
  float r = magnitude(onCurve);

  if (debug) {
    drawer.line(center, center + onCurve);
    drawer.point(center + onCurve);
    drawer.point(preV);
    drawer.point(nextV);
  }

  return Pc_mag < r;
}

// Function to check if the point P is inside the convex polygon
// Note: not all polygons in ConvexShape are actually convex, but all are radial
bool isPointInsideConvexPolygon(const sf::Vector2f& P,
                                const sf::ConvexShape& polygon,
                                float magLimit = 200) {
  int n = polygon.getPointCount();
  if (n < 3) return false;  // A polygon must have at least 3 vertices

  if (magLimit > 0 && magnitude(P - polygon.getPosition()) > magLimit) {
    return false;
  }

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

/**** Asteroid Impl ****/

Asteroid::Asteroid(sf::Vector2f position, sf::Vector2f velocity,
                   AsteroidSize size)
    : id(NEXT_ID++), velocity(velocity), size(size) {
  this->shape = makeRandomAsteroid(position, size);
}

bool Asteroid::isPointInsideAsteroid(const sf::Vector2f& P, bool debug) {
  return isPointInsideRadialPolygon(P, this->shape, BIG_RADIUS * 2, debug);
}

sf::ConvexShape Asteroid::makeRandomAsteroid(sf::Vector2f position,
                                             AsteroidSize size) {
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
    float r = radius + randomFloat(-radius / 3, radius / 3);
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

std::ostream& operator<<(std::ostream& os, const Asteroid& asteroid) {
  os << "Asteroid " << asteroid.id << " at " << asteroid.shape.getPosition()
     << " with velocity " << asteroid.velocity;
  return os;
}

/**** Ship Impl ****/

Ship::Ship() : velocity(0, 0) {
  this->shape.setPointCount(3);
  this->shape.setPoint(0, sf::Vector2f(0, -10));
  this->shape.setPoint(1, sf::Vector2f(7, 10));
  this->shape.setPoint(2, sf::Vector2f(-7, 10));
  this->shape.setFillColor(sf::Color::Black);
  this->shape.setOutlineColor(sf::Color::White);
  this->shape.setOutlineThickness(1);
}

/**** Bullet Impl ****/

Bullet::Bullet(sf::Vector2f pos, float rotation) : range(1000) {
  this->shape.setPointCount(4);
  this->shape.setPoint(0, {0, 0});
  this->shape.setPoint(1, {2, 0});
  this->shape.setPoint(2, {2, 4});
  this->shape.setPoint(3, {0, 4});
  this->shape.setFillColor(sf::Color::White);
  this->shape.setPosition(pos.x, pos.y);
  this->shape.setRotation(rotation);

  this->velocity = move_forward(rotation, bulletVelocity);
}

/**** Misc Drawing Functions ****/

sf::ConvexShape makeAlienShip() {
  sf::ConvexShape alienShip;
  alienShip.setPointCount(6);
  alienShip.setPoint(0, sf::Vector2f(-20, -10));
  alienShip.setPoint(1, sf::Vector2f(20, -10));
  alienShip.setPoint(2, sf::Vector2f(10, 0));
  alienShip.setPoint(3, sf::Vector2f(20, 10));
  alienShip.setPoint(4, sf::Vector2f(-20, 10));
  alienShip.setPoint(5, sf::Vector2f(-10, 0));
  alienShip.setFillColor(sf::Color::Black);
  alienShip.setOutlineColor(sf::Color::White);
  alienShip.setOutlineThickness(1);

  return alienShip;
}