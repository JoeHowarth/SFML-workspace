#include <SFML/Graphics.hpp>
#include <iostream>
#include <random>

const float shipAcceleration = 0.1f;
const float bulletVelocity = 5;
enum AsteroidSize { SMALL, MEDIUM, BIG };

sf::ConvexShape makeShip();
sf::ConvexShape makeBigAsteroid(sf::Vector2f position);
sf::ConvexShape makeBullet(sf::Vector2f position, float rotation);
sf::ConvexShape makeRectange();
float to_radians(float degrees) { return degrees * (3.14159265f / 180.f); }
sf::Vector2f move_forward(float degrees, float distance) {
  float radians = to_radians(degrees - 90);
  return {std::cos(radians) * distance, std::sin(radians) * distance};
}

struct Ship {
  sf::ConvexShape shape;
  sf::Vector2f velocity;

  Ship() : velocity(0, 0) { this->shape = makeShip(); }
};

struct Asteroid {
  sf::ConvexShape shape;
  sf::Vector2f velocity;
  AsteroidSize size;

  Asteroid(sf::Vector2f position, sf::Vector2f velocity, AsteroidSize size)
      : velocity(velocity), size(size) {
    this->shape = makeBigAsteroid(position);
  }
};

struct Bullet {
  sf::ConvexShape shape;
  sf::Vector2f velocity;
  float range;

  Bullet(sf::Vector2f pos, float rotation) : range(1000) {
    this->shape = makeBullet(pos, rotation);
    this->velocity = move_forward(rotation, bulletVelocity);
  }
};

bool isPointInsideConvexPolygon(const sf::ConvexShape& polygon,
                                const sf::Vector2f& P);
void applyVelocityToObject(sf::ConvexShape& shape, sf::Vector2f velocity,
                           const sf::Vector2f& viewSize);

float crossProduct(const sf::Vector2f& v1, const sf::Vector2f& v2) {
  return v1.x * v2.y - v1.y * v2.x;
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

template <typename T>
T dbg(T t) {
  std::cout << t << std::endl;
  return t;
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

int main() {
  auto window = sf::RenderWindow{{1920u, 1080u}, "CMake SFML Project"};
  window.setFramerateLimit(144);
  sf::Vector2u windowSize = window.getSize();
  sf::View view;
  view.setCenter(0, 0);
  view.setSize(static_cast<float>(windowSize.x),
               static_cast<float>(windowSize.y));
  window.setView(view);
  sf::Vector2f size = view.getSize();

  // Declare and initialize the ship and asteroid
  Ship ship;

  std::vector<Asteroid> asteroids = generateAsteroids(
      5, -size.x / 2, size.x / 2, dbg(-size.y / 2), dbg(size.y / 2));

  std::vector<Bullet> bullets;
  uint score = 0;

  while (window.isOpen()) {
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
              // {makeBullet(ship.shape.getPosition(),
              //             ship.shape.getRotation()),
              //  move_forward(ship.shape.getRotation(), bulletVelocity),
              //  1000});
              break;
            case sf::Keyboard::E:
              if (isPointInsideConvexPolygon(asteroids[0].shape,
                                             ship.shape.getPosition())) {
                std::cout << "Inside!" << std::endl;
              } else {
                std::cout << "Outside!" << std::endl;
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
      ship.shape.rotate(-5);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
      ship.shape.rotate(5);
    }

    window.clear(sf::Color::Black);
    std::vector<int> bulletsToRemove;
    std::vector<int> asteroidsToRemove;
    std::vector<Asteroid> asteroidsToAdd;

    // Wrap Objects around the screen
    for (auto& asteroid : asteroids) {
      applyVelocityToObject(asteroid.shape, asteroid.velocity, size);
    }
    applyVelocityToObject(ship.shape, ship.velocity, size);
    for (int i = 0; i < bullets.size(); ++i) {
      auto& bullet = bullets[i];
      applyVelocityToObject(bullet.shape, bullet.velocity, size);
      bullet.range -= magnitude(bullet.velocity);
      if (bullet.range <= 0) {
        bulletsToRemove.push_back(i);
      }
    }

    // Detect collisions between bullets and asteroids
    for (int i = 0; i < bullets.size(); ++i) {
      auto& bullet = bullets[i];

      std::cout << "Bullet Position: " << bullet.shape.getPosition().x << ", "
                << bullet.shape.getPosition().y << std::endl;

      for (int j = 0; j < asteroids.size(); ++j) {
        auto& asteroid = asteroids[j];
        std::cout << "Checking Asteroid at: " << asteroid.shape.getPosition().x
                  << ", " << asteroid.shape.getPosition().y << std::endl;

        if (isPointInsideConvexPolygon(asteroid.shape,
                                       bullet.shape.getPosition())) {
          std::cout << "Hit!" << std::endl;
          switch (asteroid.size) {
            case BIG:
              score += 25;
              asteroidsToAdd.push_back(Asteroid(asteroid.shape.getPosition(),
                                                randomVector2f(-1, 1, -1, 1),
                                                MEDIUM));

            case MEDIUM:
              score += 50;
            case SMALL:
              score += 100;
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

    for (auto& bullet : bullets) {
      window.draw(bullet.shape);
    }

    // Draw the ship
    window.draw(ship.shape);

    // Draw the asteroid
    for (auto& asteroid : asteroids) {
      window.draw(asteroid.shape);
    }

    auto rect = sf::RectangleShape({10, 10});
    rect.setPosition(850, 500);
    window.draw(rect);

    window.display();
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

sf::ConvexShape makeBigAsteroid(sf::Vector2f position) {
  sf::ConvexShape asteroid;
  asteroid.setPointCount(14);
  asteroid.setPoint(0, sf::Vector2f(0, 0));
  asteroid.setPoint(1, sf::Vector2f(20, 0));
  asteroid.setPoint(2, sf::Vector2f(40, 20));
  asteroid.setPoint(3, sf::Vector2f(60, 0));
  asteroid.setPoint(4, sf::Vector2f(80, 0));
  asteroid.setPoint(5, sf::Vector2f(100, 20));
  asteroid.setPoint(6, sf::Vector2f(80, 40));
  asteroid.setPoint(7, sf::Vector2f(60, 60));
  asteroid.setPoint(8, sf::Vector2f(40, 80));
  asteroid.setPoint(9, sf::Vector2f(20, 100));
  asteroid.setPoint(10, sf::Vector2f(0, 80));
  asteroid.setPoint(11, sf::Vector2f(-20, 60));
  asteroid.setPoint(12, sf::Vector2f(-40, 40));
  asteroid.setPoint(13, sf::Vector2f(-20, 20));
  asteroid.setFillColor(sf::Color::Black);
  asteroid.setOutlineColor(sf::Color::White);
  asteroid.setOutlineThickness(1);
  asteroid.setPosition(position);

  return asteroid;
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

sf::ConvexShape makeRectange() {
  sf::ConvexShape asteroid;
  asteroid.setPointCount(4);
  asteroid.setPoint(0, sf::Vector2f(10, 300));
  asteroid.setPoint(1, sf::Vector2f(400, 300));
  asteroid.setPoint(2, sf::Vector2f(400, -300));
  asteroid.setPoint(3, sf::Vector2f(10, -300));
  asteroid.setFillColor(sf::Color::Black);
  asteroid.setOutlineColor(sf::Color::White);
  asteroid.setOutlineThickness(1);

  return asteroid;
}

void applyVelocityToObject(sf::ConvexShape& shape, sf::Vector2f velocity,
                           const sf::Vector2f& viewSize) {
  shape.move(velocity);
  if (shape.getPosition().x < -viewSize.x / 2) {
    shape.setPosition(viewSize.x / 2, shape.getPosition().y);
  }
  if (shape.getPosition().x > viewSize.x / 2) {
    shape.setPosition(-viewSize.x / 2, shape.getPosition().y);
  }
  if (shape.getPosition().y < -viewSize.y / 2) {
    shape.setPosition(shape.getPosition().x, viewSize.y / 2);
  }
  if (shape.getPosition().y > viewSize.y / 2) {
    shape.setPosition(shape.getPosition().x, -viewSize.y / 2);
  }
}