#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <deque>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace
{
constexpr int WindowWidth = 800;
constexpr int WindowHeight = 600;
constexpr int CellSize = 25;
constexpr int GridColumns = WindowWidth / CellSize;
constexpr int GridRows = WindowHeight / CellSize;
constexpr float BaseStepTime = 0.14f;
constexpr float MinimumStepTime = 0.055f;
constexpr float SpeedGainPerFood = 0.006f;

enum class Direction
{
    Up,
    Down,
    Left,
    Right
};

enum class GameState
{
    Start,
    Playing,
    GameOver
};

sf::Vector2i directionToVector(Direction direction)
{
    switch (direction)
    {
    case Direction::Up:
        return {0, -1};
    case Direction::Down:
        return {0, 1};
    case Direction::Left:
        return {-1, 0};
    case Direction::Right:
        return {1, 0};
    }

    return {1, 0};
}

bool isOpposite(Direction a, Direction b)
{
    const sf::Vector2i va = directionToVector(a);
    const sf::Vector2i vb = directionToVector(b);
    return va.x + vb.x == 0 && va.y + vb.y == 0;
}

float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

sf::Color withAlpha(const sf::Color& color, float alpha)
{
    return sf::Color(
        color.r,
        color.g,
        color.b,
        static_cast<std::uint8_t>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f)
    );
}

class Snake
{
public:
    Snake()
    {
        reset();
    }

    void reset()
    {
        segments_.clear();
        segments_.push_back({GridColumns / 2, GridRows / 2});
        segments_.push_back({GridColumns / 2 - 1, GridRows / 2});
        segments_.push_back({GridColumns / 2 - 2, GridRows / 2});
        direction_ = Direction::Right;
        queuedDirection_ = direction_;
        grewLastStep_ = false;
        previousSegments_ = segments_;
    }

    void queueDirection(Direction direction)
    {
        if (!isOpposite(direction, direction_))
        {
            queuedDirection_ = direction;
        }
    }

    std::optional<sf::Vector2i> step()
    {
        previousSegments_ = segments_;
        if (!isOpposite(queuedDirection_, direction_))
        {
            direction_ = queuedDirection_;
        }

        const sf::Vector2i nextHead = head() + directionToVector(direction_);
        segments_.push_front(nextHead);

        if (growthPending_ > 0)
        {
            --growthPending_;
            grewLastStep_ = true;
        }
        else
        {
            segments_.pop_back();
            grewLastStep_ = false;
        }

        return nextHead;
    }

    void grow()
    {
        ++growthPending_;
    }

    bool hitWall() const
    {
        const sf::Vector2i pos = head();
        return pos.x < 0 || pos.x >= GridColumns || pos.y < 0 || pos.y >= GridRows;
    }

    bool hitSelf() const
    {
        const sf::Vector2i pos = head();
        return std::count(std::next(segments_.begin()), segments_.end(), pos) > 0;
    }

    bool occupies(const sf::Vector2i& cell) const
    {
        return std::find(segments_.begin(), segments_.end(), cell) != segments_.end();
    }

    const std::deque<sf::Vector2i>& segments() const
    {
        return segments_;
    }

    const std::deque<sf::Vector2i>& previousSegments() const
    {
        return previousSegments_;
    }

    Direction direction() const
    {
        return direction_;
    }

    bool grewLastStep() const
    {
        return grewLastStep_;
    }

    sf::Vector2i head() const
    {
        return segments_.front();
    }

private:
    std::deque<sf::Vector2i> segments_;
    std::deque<sf::Vector2i> previousSegments_;
    Direction direction_ = Direction::Right;
    Direction queuedDirection_ = Direction::Right;
    int growthPending_ = 0;
    bool grewLastStep_ = false;
};

class Food
{
public:
    void respawn(const Snake& snake, std::mt19937& rng)
    {
        std::vector<sf::Vector2i> freeCells;
        freeCells.reserve(GridColumns * GridRows);

        for (int y = 0; y < GridRows; ++y)
        {
            for (int x = 0; x < GridColumns; ++x)
            {
                const sf::Vector2i cell{x, y};
                if (!snake.occupies(cell))
                {
                    freeCells.push_back(cell);
                }
            }
        }

        if (freeCells.empty())
        {
            position_ = {0, 0};
            return;
        }

        std::uniform_int_distribution<std::size_t> dist(0, freeCells.size() - 1);
        position_ = freeCells[dist(rng)];
        spawnPulse_ = 0.0f;
    }

    void update(float dt)
    {
        spawnPulse_ = std::min(1.0f, spawnPulse_ + dt * 4.0f);
    }

    const sf::Vector2i& position() const
    {
        return position_;
    }

    float spawnPulse() const
    {
        return spawnPulse_;
    }

private:
    sf::Vector2i position_{4, 4};
    float spawnPulse_ = 1.0f;
};

class Game
{
public:
    Game()
        : window_(sf::VideoMode({WindowWidth, WindowHeight}), "Snake - SFML")
        , rng_(std::random_device{}())
    {
        window_.setVerticalSyncEnabled(false);
        window_.setFramerateLimit(120);
        window_.setKeyRepeatEnabled(false);
        loadFont();
        buildStaticShapes();
        resetGame();
    }

    void run()
    {
        sf::Clock clock;

        while (window_.isOpen())
        {
            const float dt = std::min(clock.restart().asSeconds(), 0.1f);
            processEvents();
            update(dt);
            render();
        }
    }

private:
    void loadFont()
    {
        const std::array<std::string, 5> candidates = {
            "assets/fonts/arial.ttf",
            "assets/fonts/DejaVuSans.ttf",
            "C:/Windows/Fonts/arial.ttf",
            "C:/Windows/Fonts/segoeui.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
        };

        for (const std::string& path : candidates)
        {
            if (font_.openFromFile(path))
            {
                fontLoaded_ = true;
                break;
            }
        }
    }

    void buildStaticShapes()
    {
        cellShape_.setSize(sf::Vector2f(static_cast<float>(CellSize - 2), static_cast<float>(CellSize - 2)));
        cellShape_.setOrigin(cellShape_.getSize() * 0.5f);

        foodShape_.setSize(sf::Vector2f(static_cast<float>(CellSize - 6), static_cast<float>(CellSize - 6)));
        foodShape_.setOrigin(foodShape_.getSize() * 0.5f);

        fadeOverlay_.setSize(sf::Vector2f(static_cast<float>(WindowWidth), static_cast<float>(WindowHeight)));
    }

    void resetGame()
    {
        snake_.reset();
        food_.respawn(snake_, rng_);
        score_ = 0;
        movementAccumulator_ = 0.0f;
        moveProgress_ = 0.0f;
        stateTime_ = 0.0f;
    }

    void startGame()
    {
        resetGame();
        transitionTo(GameState::Playing);
    }

    void transitionTo(GameState nextState)
    {
        state_ = nextState;
        stateTime_ = 0.0f;
    }

    float currentStepTime() const
    {
        return std::max(MinimumStepTime, BaseStepTime - static_cast<float>(score_) * SpeedGainPerFood);
    }

    void processEvents()
    {
        while (const std::optional event = window_.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window_.close();
            }
            else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
            {
                handleKeyPressed(keyPressed->code);
            }
        }
    }

    void handleKeyPressed(sf::Keyboard::Key key)
    {
        if (key == sf::Keyboard::Key::Escape)
        {
            window_.close();
            return;
        }

        if (state_ == GameState::Start)
        {
            if (key == sf::Keyboard::Key::Enter || key == sf::Keyboard::Key::Space)
            {
                startGame();
            }
            return;
        }

        if (state_ == GameState::GameOver)
        {
            if (key == sf::Keyboard::Key::Enter || key == sf::Keyboard::Key::R)
            {
                startGame();
            }
            return;
        }

        switch (key)
        {
        case sf::Keyboard::Key::Up:
            snake_.queueDirection(Direction::Up);
            break;
        case sf::Keyboard::Key::Down:
            snake_.queueDirection(Direction::Down);
            break;
        case sf::Keyboard::Key::Left:
            snake_.queueDirection(Direction::Left);
            break;
        case sf::Keyboard::Key::Right:
            snake_.queueDirection(Direction::Right);
            break;
        default:
            break;
        }
    }

    void update(float dt)
    {
        stateTime_ += dt;
        food_.update(dt);

        if (state_ != GameState::Playing)
        {
            return;
        }

        movementAccumulator_ += dt;
        const float stepTime = currentStepTime();

        while (movementAccumulator_ >= stepTime)
        {
            movementAccumulator_ -= stepTime;
            advanceSnake();

            if (state_ != GameState::Playing)
            {
                moveProgress_ = 0.0f;
                break;
            }
        }

        moveProgress_ = std::clamp(movementAccumulator_ / stepTime, 0.0f, 1.0f);
    }

    void advanceSnake()
    {
        snake_.step();

        if (snake_.hitWall() || snake_.hitSelf())
        {
            transitionTo(GameState::GameOver);
            return;
        }

        if (snake_.head() == food_.position())
        {
            snake_.grow();
            ++score_;
            food_.respawn(snake_, rng_);
        }
    }

    sf::Vector2f cellCenter(const sf::Vector2i& cell) const
    {
        return {
            static_cast<float>(cell.x * CellSize + CellSize / 2),
            static_cast<float>(cell.y * CellSize + CellSize / 2)
        };
    }

    sf::Vector2f interpolatedSegmentPosition(std::size_t index) const
    {
        const auto& current = snake_.segments();
        const auto& previous = snake_.previousSegments();

        const sf::Vector2i currentCell = index < current.size() ? current[index] : current.back();
        const sf::Vector2i previousCell = index < previous.size() ? previous[index] : previous.back();

        const sf::Vector2f from = cellCenter(previousCell);
        const sf::Vector2f to = cellCenter(currentCell);
        return {
            lerp(from.x, to.x, moveProgress_),
            lerp(from.y, to.y, moveProgress_)
        };
    }

    void renderBackground()
    {
        window_.clear(sf::Color(18, 24, 32));

        sf::RectangleShape tile(sf::Vector2f(static_cast<float>(CellSize), static_cast<float>(CellSize)));
        for (int y = 0; y < GridRows; ++y)
        {
            for (int x = 0; x < GridColumns; ++x)
            {
                const bool alternate = (x + y) % 2 == 0;
                tile.setPosition({static_cast<float>(x * CellSize), static_cast<float>(y * CellSize)});
                tile.setFillColor(alternate ? sf::Color(24, 32, 42) : sf::Color(28, 37, 49));
                window_.draw(tile);
            }
        }
    }

    void renderFood()
    {
        const sf::Vector2f center = cellCenter(food_.position());
        const float pulse = 0.88f + 0.12f * food_.spawnPulse() + std::sin(stateTime_ * 8.0f) * 0.03f;

        foodShape_.setPosition(center);
        foodShape_.setScale({pulse, pulse});
        foodShape_.setFillColor(sf::Color(244, 91, 105));
        window_.draw(foodShape_);
    }

    void renderSnake()
    {
        const auto& segments = snake_.segments();
        for (std::size_t i = 0; i < segments.size(); ++i)
        {
            cellShape_.setPosition(interpolatedSegmentPosition(i));
            const float scale = i == 0 ? 1.0f : 0.92f;
            cellShape_.setScale({scale, scale});
            cellShape_.setFillColor(i == 0 ? sf::Color(122, 232, 111) : sf::Color(76, 185, 95));
            window_.draw(cellShape_);
        }
    }

    void drawText(const std::string& content, unsigned int size, const sf::Vector2f& position,
                  const sf::Color& color, bool centered = false)
    {
        if (!fontLoaded_)
        {
            return;
        }

        sf::Text text(font_, content, size);
        text.setFillColor(color);

        if (centered)
        {
            const sf::FloatRect bounds = text.getLocalBounds();
            text.setOrigin({
                bounds.position.x + bounds.size.x * 0.5f,
                bounds.position.y + bounds.size.y * 0.5f
            });
        }

        text.setPosition(position);
        window_.draw(text);
    }

    void renderHud()
    {
        std::ostringstream stream;
        stream << "Score: " << score_;
        drawText(stream.str(), 24, {18.0f, 12.0f}, sf::Color(236, 240, 241));

        std::ostringstream speedStream;
        speedStream << "Speed: " << static_cast<int>(std::round((1.0f / currentStepTime()) * 10.0f));
        drawText(speedStream.str(), 18, {18.0f, 42.0f}, sf::Color(170, 180, 190));
    }

    void renderOverlay()
    {
        float overlayAlpha = 0.0f;
        std::string title;
        std::string subtitle;

        if (state_ == GameState::Start)
        {
            overlayAlpha = 0.78f;
            title = "Snake";
            subtitle = "Press Enter to Start";
        }
        else if (state_ == GameState::GameOver)
        {
            overlayAlpha = 0.72f;
            title = "Game Over";
            subtitle = "Press Enter or R to Restart";
        }

        if (overlayAlpha <= 0.0f)
        {
            return;
        }

        const float fadeIn = std::min(1.0f, stateTime_ * 2.5f);
        fadeOverlay_.setFillColor(withAlpha(sf::Color(8, 12, 16), overlayAlpha * fadeIn));
        window_.draw(fadeOverlay_);

        drawText(title, 56, {WindowWidth * 0.5f, WindowHeight * 0.42f}, withAlpha(sf::Color(247, 250, 252), fadeIn), true);
        drawText(subtitle, 24, {WindowWidth * 0.5f, WindowHeight * 0.52f}, withAlpha(sf::Color(201, 209, 217), fadeIn), true);

        if (state_ == GameState::GameOver)
        {
            std::ostringstream finalScore;
            finalScore << "Final Score: " << score_;
            drawText(finalScore.str(), 22, {WindowWidth * 0.5f, WindowHeight * 0.60f}, withAlpha(sf::Color(244, 91, 105), fadeIn), true);
        }
    }

    void render()
    {
        renderBackground();

        if (state_ != GameState::Start)
        {
            renderFood();
            renderSnake();
            renderHud();
        }

        renderOverlay();
        window_.display();
    }

    sf::RenderWindow window_;
    sf::Font font_;
    bool fontLoaded_ = false;
    Snake snake_;
    Food food_;
    GameState state_ = GameState::Start;
    std::mt19937 rng_;
    int score_ = 0;
    float movementAccumulator_ = 0.0f;
    float moveProgress_ = 0.0f;
    float stateTime_ = 0.0f;
    sf::RectangleShape cellShape_;
    sf::RectangleShape foodShape_;
    sf::RectangleShape fadeOverlay_;
};
} // namespace

int main()
{
    Game game;
    game.run();
    return 0;
}
