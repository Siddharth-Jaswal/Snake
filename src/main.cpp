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

enum class Theme
{
    Ember,
    Ocean,
    Monochrome
};

enum class SnakeStyle
{
    Lime,
    Gold,
    Cyan
};

enum class Difficulty
{
    Chill,
    Classic,
    Expert
};

struct Palette
{
    sf::Color background;
    sf::Color ambientA;
    sf::Color ambientB;
    sf::Color boardBase;
    sf::Color boardOutline;
    sf::Color tileA;
    sf::Color tileB;
    sf::Color gridLine;
    sf::Color sheen;
    sf::Color hudFill;
    sf::Color hudOutline;
    sf::Color textPrimary;
    sf::Color textSecondary;
    sf::Color accent;
    sf::Color food;
    sf::Color foodGlow;
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

sf::Vector2f toVector2f(const sf::Vector2i& value)
{
    return {static_cast<float>(value.x), static_cast<float>(value.y)};
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

Palette makePalette(Theme theme)
{
    switch (theme)
    {
    case Theme::Ocean:
        return {
            sf::Color(7, 14, 20),
            sf::Color(18, 74, 92),
            sf::Color(22, 36, 86),
            sf::Color(11, 24, 31),
            sf::Color(77, 134, 150),
            sf::Color(23, 47, 58),
            sf::Color(18, 37, 46),
            sf::Color(210, 245, 255, 18),
            sf::Color(255, 255, 255, 14),
            sf::Color(8, 17, 24, 176),
            sf::Color(112, 176, 194, 165),
            sf::Color(239, 249, 252),
            sf::Color(166, 196, 205),
            sf::Color(110, 214, 228),
            sf::Color(255, 132, 95),
            sf::Color(255, 132, 95, 50)
        };
    case Theme::Monochrome:
        return {
            sf::Color(10, 10, 12),
            sf::Color(46, 46, 54),
            sf::Color(24, 24, 28),
            sf::Color(19, 19, 23),
            sf::Color(124, 124, 136),
            sf::Color(36, 36, 42),
            sf::Color(28, 28, 34),
            sf::Color(255, 255, 255, 15),
            sf::Color(255, 255, 255, 10),
            sf::Color(12, 12, 15, 178),
            sf::Color(154, 154, 164, 160),
            sf::Color(245, 245, 247),
            sf::Color(188, 188, 195),
            sf::Color(210, 210, 215),
            sf::Color(255, 110, 110),
            sf::Color(255, 110, 110, 48)
        };
    case Theme::Ember:
    default:
        return {
            sf::Color(9, 13, 18),
            sf::Color(27, 55, 47),
            sf::Color(74, 34, 28),
            sf::Color(15, 22, 28),
            sf::Color(58, 79, 72),
            sf::Color(24, 34, 33),
            sf::Color(20, 29, 28),
            sf::Color(255, 255, 255, 18),
            sf::Color(255, 255, 255, 18),
            sf::Color(8, 12, 16, 178),
            sf::Color(95, 126, 116, 170),
            sf::Color(242, 247, 245),
            sf::Color(173, 191, 184),
            sf::Color(140, 196, 164),
            sf::Color(244, 91, 105),
            sf::Color(255, 99, 120, 52)
        };
    }
}

sf::Color snakeHeadColor(SnakeStyle style)
{
    switch (style)
    {
    case SnakeStyle::Gold:
        return sf::Color(247, 216, 96);
    case SnakeStyle::Cyan:
        return sf::Color(105, 236, 230);
    case SnakeStyle::Lime:
    default:
        return sf::Color(152, 245, 126);
    }
}

sf::Color snakeBodyColor(SnakeStyle style)
{
    switch (style)
    {
    case SnakeStyle::Gold:
        return sf::Color(212, 170, 58);
    case SnakeStyle::Cyan:
        return sf::Color(49, 181, 191);
    case SnakeStyle::Lime:
    default:
        return sf::Color(66, 186, 104);
    }
}

std::string themeLabel(Theme theme)
{
    switch (theme)
    {
    case Theme::Ocean:
        return "Ocean";
    case Theme::Monochrome:
        return "Monochrome";
    case Theme::Ember:
    default:
        return "Ember";
    }
}

std::string snakeStyleLabel(SnakeStyle style)
{
    switch (style)
    {
    case SnakeStyle::Gold:
        return "Gold";
    case SnakeStyle::Cyan:
        return "Cyan";
    case SnakeStyle::Lime:
    default:
        return "Lime";
    }
}

std::string difficultyLabel(Difficulty difficulty)
{
    switch (difficulty)
    {
    case Difficulty::Chill:
        return "Chill";
    case Difficulty::Expert:
        return "Expert";
    case Difficulty::Classic:
    default:
        return "Classic";
    }
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
        cellShape_.setSize(sf::Vector2f(static_cast<float>(CellSize - 4), static_cast<float>(CellSize - 4)));
        cellShape_.setOrigin(cellShape_.getSize() * 0.5f);

        foodShape_.setSize(sf::Vector2f(static_cast<float>(CellSize - 10), static_cast<float>(CellSize - 10)));
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
        float baseTime = BaseStepTime;
        switch (difficulty_)
        {
        case Difficulty::Chill:
            baseTime += 0.03f;
            break;
        case Difficulty::Expert:
            baseTime -= 0.025f;
            break;
        case Difficulty::Classic:
        default:
            break;
        }

        return std::max(MinimumStepTime, baseTime - static_cast<float>(score_) * SpeedGainPerFood);
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
                return;
            }

            switch (key)
            {
            case sf::Keyboard::Key::A:
                cycleTheme(-1);
                break;
            case sf::Keyboard::Key::D:
                cycleTheme(1);
                break;
            case sf::Keyboard::Key::W:
                cycleDifficulty(1);
                break;
            case sf::Keyboard::Key::S:
                cycleDifficulty(-1);
                break;
            case sf::Keyboard::Key::Q:
                cycleSnakeStyle(-1);
                break;
            case sf::Keyboard::Key::E:
                cycleSnakeStyle(1);
                break;
            default:
                break;
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
        case sf::Keyboard::Key::W:
            snake_.queueDirection(Direction::Up);
            break;
        case sf::Keyboard::Key::Down:
        case sf::Keyboard::Key::S:
            snake_.queueDirection(Direction::Down);
            break;
        case sf::Keyboard::Key::Left:
        case sf::Keyboard::Key::A:
            snake_.queueDirection(Direction::Left);
            break;
        case sf::Keyboard::Key::Right:
        case sf::Keyboard::Key::D:
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

    sf::FloatRect boardBounds() const
    {
        return {{0.0f, 0.0f}, {static_cast<float>(GridColumns * CellSize), static_cast<float>(GridRows * CellSize)}};
    }

    void cycleTheme(int direction)
    {
        const int count = 3;
        const int next = (static_cast<int>(theme_) + direction + count) % count;
        theme_ = static_cast<Theme>(next);
        palette_ = makePalette(theme_);
    }

    void cycleSnakeStyle(int direction)
    {
        const int count = 3;
        const int next = (static_cast<int>(snakeStyle_) + direction + count) % count;
        snakeStyle_ = static_cast<SnakeStyle>(next);
    }

    void cycleDifficulty(int direction)
    {
        const int count = 3;
        const int next = (static_cast<int>(difficulty_) + direction + count) % count;
        difficulty_ = static_cast<Difficulty>(next);
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
        window_.clear(palette_.background);

        sf::CircleShape glow(220.0f);
        glow.setOrigin({220.0f, 220.0f});
        glow.setScale({1.6f, 0.85f});
        glow.setFillColor(withAlpha(palette_.ambientA, 0.42f));
        glow.setPosition({WindowWidth * 0.18f, WindowHeight * 0.10f});
        window_.draw(glow);

        glow.setScale({1.25f, 0.72f});
        glow.setFillColor(withAlpha(palette_.ambientB, 0.38f));
        glow.setPosition({WindowWidth * 0.82f, WindowHeight * 0.16f});
        window_.draw(glow);

        const sf::FloatRect board = boardBounds();
        sf::RectangleShape boardShadow({board.size.x + 24.0f, board.size.y + 24.0f});
        boardShadow.setPosition({board.position.x - 12.0f, board.position.y - 2.0f});
        boardShadow.setFillColor(sf::Color(0, 0, 0, 90));
        window_.draw(boardShadow);

        sf::RectangleShape boardBase(board.size);
        boardBase.setPosition(board.position);
        boardBase.setFillColor(palette_.boardBase);
        boardBase.setOutlineThickness(2.0f);
        boardBase.setOutlineColor(palette_.boardOutline);
        window_.draw(boardBase);

        sf::RectangleShape tile(sf::Vector2f(static_cast<float>(CellSize), static_cast<float>(CellSize)));
        for (int y = 0; y < GridRows; ++y)
        {
            for (int x = 0; x < GridColumns; ++x)
            {
                const bool alternate = (x + y) % 2 == 0;
                const int distanceToCenter = std::abs(x - GridColumns / 2) + std::abs(y - GridRows / 2);
                const int vignette = std::min(distanceToCenter, 18);
                tile.setPosition({static_cast<float>(x * CellSize), static_cast<float>(y * CellSize)});
                tile.setFillColor(alternate
                    ? sf::Color(
                          static_cast<std::uint8_t>(std::min(255, palette_.tileA.r + vignette)),
                          static_cast<std::uint8_t>(std::min(255, palette_.tileA.g + vignette / 2)),
                          static_cast<std::uint8_t>(std::min(255, palette_.tileA.b + vignette / 3)))
                    : sf::Color(
                          static_cast<std::uint8_t>(std::min(255, palette_.tileB.r + vignette)),
                          static_cast<std::uint8_t>(std::min(255, palette_.tileB.g + vignette / 2)),
                          static_cast<std::uint8_t>(std::min(255, palette_.tileB.b + vignette / 3))));
                window_.draw(tile);
            }
        }

        sf::RectangleShape line;
        line.setFillColor(palette_.gridLine);
        for (int x = 1; x < GridColumns; ++x)
        {
            line.setSize({1.0f, board.size.y});
            line.setPosition({static_cast<float>(x * CellSize), 0.0f});
            window_.draw(line);
        }

        for (int y = 1; y < GridRows; ++y)
        {
            line.setSize({board.size.x, 1.0f});
            line.setPosition({0.0f, static_cast<float>(y * CellSize)});
            window_.draw(line);
        }

        sf::RectangleShape sheen({board.size.x, 90.0f});
        sheen.setPosition({0.0f, 0.0f});
        sheen.setFillColor(palette_.sheen);
        window_.draw(sheen);
    }

    void renderFood()
    {
        const sf::Vector2f center = cellCenter(food_.position());
        const float pulse = 0.88f + 0.12f * food_.spawnPulse() + std::sin(stateTime_ * 8.0f) * 0.03f;

        sf::CircleShape halo(static_cast<float>(CellSize) * 0.52f);
        halo.setOrigin({halo.getRadius(), halo.getRadius()});
        halo.setPosition(center);
        halo.setFillColor(palette_.foodGlow);
        halo.setScale({pulse * 1.15f, pulse * 1.15f});
        window_.draw(halo);

        foodShape_.setPosition(center);
        foodShape_.setScale({pulse, pulse});
        foodShape_.setFillColor(palette_.food);
        foodShape_.setOutlineThickness(2.0f);
        foodShape_.setOutlineColor(sf::Color(255, 214, 220, 110));
        window_.draw(foodShape_);

        sf::CircleShape seed(2.0f);
        seed.setFillColor(sf::Color(255, 235, 220, 180));
        seed.setOrigin({2.0f, 2.0f});
        seed.setPosition({center.x - 3.5f, center.y - 2.0f});
        window_.draw(seed);
        seed.setPosition({center.x + 2.5f, center.y + 1.0f});
        window_.draw(seed);
    }

    void renderSnake()
    {
        const auto& segments = snake_.segments();
        for (std::size_t i = 0; i < segments.size(); ++i)
        {
            const sf::Vector2f position = interpolatedSegmentPosition(i);
            cellShape_.setPosition(position);
            const float scale = i == 0 ? 1.0f : 0.92f;
            cellShape_.setScale({scale, scale});
            cellShape_.setFillColor(i == 0 ? snakeHeadColor(snakeStyle_) : snakeBodyColor(snakeStyle_));
            cellShape_.setOutlineThickness(i == 0 ? 2.0f : 1.0f);
            cellShape_.setOutlineColor(i == 0 ? sf::Color(230, 255, 215, 105) : sf::Color(20, 64, 37, 120));
            window_.draw(cellShape_);

            sf::RectangleShape highlight({cellShape_.getSize().x * 0.56f, cellShape_.getSize().y * 0.20f});
            highlight.setOrigin(highlight.getSize() * 0.5f);
            highlight.setPosition({position.x, position.y - 5.0f});
            highlight.setScale({scale, scale});
            highlight.setFillColor(i == 0 ? sf::Color(246, 255, 240, 65) : sf::Color(220, 255, 225, 35));
            window_.draw(highlight);
        }

        if (!segments.empty())
        {
            const sf::Vector2f headPosition = interpolatedSegmentPosition(0);
            const sf::Vector2f forward = toVector2f(directionToVector(snake_.direction()));
            const sf::Vector2f side{-forward.y, forward.x};

            sf::CircleShape eye(2.4f);
            eye.setOrigin({2.4f, 2.4f});
            eye.setFillColor(sf::Color(18, 26, 20));
            eye.setPosition(headPosition + forward * 4.0f + side * 4.0f);
            window_.draw(eye);
            eye.setPosition(headPosition + forward * 4.0f - side * 4.0f);
            window_.draw(eye);
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
        text.setOutlineThickness(centered ? 2.0f : 0.0f);
        text.setOutlineColor(sf::Color(5, 8, 11, 170));

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
        sf::RectangleShape hudPanel({162.0f, 58.0f});
        hudPanel.setPosition({WindowWidth - 178.0f, 16.0f});
        hudPanel.setFillColor(palette_.hudFill);
        hudPanel.setOutlineThickness(1.5f);
        hudPanel.setOutlineColor(palette_.hudOutline);
        window_.draw(hudPanel);

        std::ostringstream stream;
        stream << "Score: " << score_;
        drawText(stream.str(), 22, {WindowWidth - 164.0f, 22.0f}, palette_.textPrimary);

        std::ostringstream speedStream;
        speedStream << "Speed Lv. " << (score_ / 3 + 1);
        drawText(speedStream.str(), 15, {WindowWidth - 164.0f, 46.0f}, palette_.textSecondary);
    }

    void renderOverlay()
    {
        float overlayAlpha = 0.0f;
        std::string title;
        std::string subtitle;
        std::string kicker;

        if (state_ == GameState::Start)
        {
            overlayAlpha = 0.78f;
            title = "Snake";
            subtitle = "Press Enter to Start";
            kicker = "Arcade Grid Runner";
        }
        else if (state_ == GameState::GameOver)
        {
            overlayAlpha = 0.72f;
            title = "Game Over";
            subtitle = "Press Enter or R to Restart";
            kicker = "Run Ended";
        }

        if (overlayAlpha <= 0.0f)
        {
            return;
        }

        const float fadeIn = std::min(1.0f, stateTime_ * 2.5f);
        fadeOverlay_.setFillColor(withAlpha(sf::Color(8, 12, 16), overlayAlpha * fadeIn));
        window_.draw(fadeOverlay_);

        sf::RectangleShape modal({460.0f, state_ == GameState::GameOver ? 220.0f : 370.0f});
        modal.setOrigin(modal.getSize() * 0.5f);
        modal.setPosition({WindowWidth * 0.5f, WindowHeight * 0.5f});
        modal.setFillColor(withAlpha(sf::Color(10, 15, 20), 0.86f * fadeIn));
        modal.setOutlineThickness(2.0f);
        modal.setOutlineColor(withAlpha(palette_.hudOutline, fadeIn));
        window_.draw(modal);

        const float centerX = WindowWidth * 0.5f;
        const float panelCenterY = WindowHeight * 0.5f;

        drawText(kicker, 18, {centerX, panelCenterY - 105.0f}, withAlpha(palette_.accent, fadeIn), true);
        drawText(title, 58, {centerX, panelCenterY - 50.0f}, withAlpha(palette_.textPrimary, fadeIn), true);
        drawText(subtitle, 24, {centerX, panelCenterY + 8.0f}, withAlpha(palette_.textSecondary, fadeIn), true);

        if (state_ == GameState::GameOver)
        {
            std::ostringstream finalScore;
            finalScore << "Final Score: " << score_;
            drawText(finalScore.str(), 24, {centerX, panelCenterY + 72.0f}, withAlpha(sf::Color(244, 91, 105), fadeIn), true);
        }
        else if (state_ == GameState::Start)
        {
            drawText("Use arrow keys to survive the grid.", 20, {centerX, panelCenterY + 56.0f},
                     withAlpha(palette_.textSecondary, fadeIn), true);

            std::ostringstream themeText;
            themeText << "Theme: " << themeLabel(theme_) << "   [A/D]";
            drawText(themeText.str(), 19, {centerX, panelCenterY + 102.0f},
                     withAlpha(palette_.textPrimary, fadeIn), true);

            std::ostringstream snakeText;
            snakeText << "Snake: " << snakeStyleLabel(snakeStyle_) << "   [Q/E]";
            drawText(snakeText.str(), 19, {centerX, panelCenterY + 134.0f},
                     withAlpha(palette_.textPrimary, fadeIn), true);

            std::ostringstream difficultyText;
            difficultyText << "Difficulty: " << difficultyLabel(difficulty_) << "   [W/S]";
            drawText(difficultyText.str(), 19, {centerX, panelCenterY + 166.0f},
                     withAlpha(palette_.textPrimary, fadeIn), true);
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
    Theme theme_ = Theme::Ember;
    SnakeStyle snakeStyle_ = SnakeStyle::Lime;
    Difficulty difficulty_ = Difficulty::Classic;
    Palette palette_ = makePalette(Theme::Ember);
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
