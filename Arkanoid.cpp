#define _USE_MATH_DEFINES
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <map>
#include <ctime>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <tuple>
#include <functional>
#include <math.h>

// Window dimensions
const GLint WIDTH = 800, HEIGHT = 600;

// Paddle
struct Paddle {
    float x, y;
    float width, height;
    float speed;
} paddle;

// Ball
struct Ball {
    float x, y;
    float radius;
    float velocityX, velocityY;

    bool operator==(const Ball& other) const {
        return x == other.x && y == other.y;
    }
} ball;

// Типы блоков в игре Арканоид
enum BlockType {
    DESTRUCTIBLE,
    SPEED_UP,
    INDESTRUCTIBLE,
};

enum BonusType {
    BONUS_SIZE_UP,
    BONUS_SIZE_DOWN,
    BONUS_SPEED_UP,
    BONUS_SPEED_DOWN,
    BONUS_STICKY,
    BONUS_EXTRA_LIFE,
    BONUS_EXTRA_BALL,
    BONUS_ONE_TIME_BOTTOM
};

std::map<BlockType, std::tuple<float, float, float>> blockColorMap = {
    {INDESTRUCTIBLE, {0.8f, 0.8f, 0.8f}}, // FFFFFF Неразрушаемые
    {DESTRUCTIBLE, {1.0f, 0.843f, 0.0f}}, // C492B1 Блоки имеют уровень здоровья
    {SPEED_UP, {0.886f, 0.286f, 0.427f}}  // E34A6F скорость
};

std::map<BonusType, std::tuple<float, float, float>> bonusColorMap = {
    {BONUS_SIZE_UP, {0.329f, 1.0f, 0.267f}}, // 53FF45 зеленый
    {BONUS_SIZE_DOWN, {0.329f, 1.0f, 0.267f}}, // 53FF45
    {BONUS_STICKY, {0.329f, 1.0f, 0.267f}}, // 53FF45
    {BONUS_EXTRA_LIFE, {0.329f, 1.0f, 0.267f}}, // 53FF45
    {BONUS_EXTRA_BALL, {0.0f, 0.663f, 0.910f}}, // 00A9E8
    {BONUS_SPEED_UP, {0.886f, 0.286f, 0.427f}}, // E34A6F скорость
    {BONUS_SPEED_DOWN, {0.886f, 0.286f, 0.427f}}, // E34A6F скорость
    {BONUS_ONE_TIME_BOTTOM, {1.0f, 0.843f, 0.0f}}  // C492B1
};

std::map<BlockType, int> blockHealth = {
    {INDESTRUCTIBLE, -1},
    {DESTRUCTIBLE, 2},
    {SPEED_UP, 1}
};

struct Block {
    float x, y;
    float width, height;
    BlockType type;
    int health;
    bool destroyed;
};

struct Bonus {
    float x, y;
    float width, height;
    BonusType type;
    bool active;
};

// Game state
int score = 0;
int lives = 3;
bool stickyBall;
bool oneTimeBottom = false;
bool startFlag;
std::vector<Block> blocks;
std::vector<Ball> balls;
std::vector<Bonus> bonuses;

bool checkCollision(Ball& ball, Paddle& paddle) {
    return ball.x + ball.radius >= paddle.x && ball.x - ball.radius <= paddle.x + paddle.width &&
        ball.y + ball.radius >= paddle.y && ball.y - ball.radius <= paddle.y + paddle.height;
}

bool checkCollision(Ball& ball, Block& block) {
    return !block.destroyed &&
        ball.x + ball.radius >= block.x && ball.x - ball.radius <= block.x + block.width &&
        ball.y + ball.radius >= block.y && ball.y - ball.radius <= block.y + block.height;
}

bool checkCollision(Paddle& paddle, Bonus& bonus) {
    return bonus.active &&
        paddle.x < bonus.x + bonus.width && paddle.x + paddle.width > bonus.x &&
        paddle.y < bonus.y + bonus.height && paddle.y + paddle.height > bonus.y;
}

void applyBonus(BonusType type) {
    switch (type) {
    case BONUS_SIZE_UP:
        paddle.width *= 1.2f;
        break;
    case BONUS_SIZE_DOWN:
        paddle.width *= 0.8f;
        break;
    case BONUS_SPEED_UP:
        for (auto& ball : balls) {
            ball.velocityX *= 1.2f;
            ball.velocityY *= 1.2f;
        }
        break;
    case BONUS_SPEED_DOWN:
        for (auto& ball : balls) {
            ball.velocityX *= 0.8f;
            ball.velocityY *= 0.8f;
        }
        break;
    case BONUS_STICKY:
        stickyBall = true;
        break;
    case BONUS_EXTRA_LIFE:
        lives++;
        break;
    case BONUS_EXTRA_BALL: {
        stickyBall = false;
        Ball newBall = { paddle.x + paddle.width / 2, paddle.y - 10.0f, 10.0f, 200.0f, -200.0f };
        balls.push_back(newBall);
        break;
    }
    case BONUS_ONE_TIME_BOTTOM:
        oneTimeBottom = true;
        break;
    default:
        break;
    }
}

void initGame() {

    startFlag = true;
    stickyBall = true;
    // Инициализация весла
    paddle.x = WIDTH / 2.0f - 50.0f;
    paddle.y = HEIGHT - 30.0f;
    paddle.width = 100.0f;
    paddle.height = 20.0f;
    paddle.speed = 500.0f;

    // Инициализация мячей
    balls.clear();
    Ball initialBall = { paddle.x + paddle.width / 2, paddle.y - 10.0f, 10.0f, 0.0f, 0.0f };
    balls.push_back(initialBall);

    // Инициализация блоков
    blocks.clear();
    bonuses.clear();

    int numRows = 4 + std::rand() % 5; // Генерация случайного числа от 4 до 8

    // Создание блоков
    for (int i = 0; i < numRows; ++i) {
        for (int j = 0; j < 10; ++j) {
            if (i >= 5 && std::rand() % 3 == 0) { // Генерация пропуска с вероятностью 1/3
                continue;
            }

            Block block;
            block.x = j * 80.0f;
            block.y = i * 30.0f;
            block.width = 78.0f;
            block.height = 28.0f;
            block.destroyed = false;

            // Генерация случайного типа блока и присвоение здоровья
            int randomTypeIndex = std::rand() % 2;
            auto it = blockHealth.begin();
            std::advance(it, randomTypeIndex);
            block.type = it->first;
            block.health = it->second;

            blocks.push_back(block);
        }
    }
}

void processInput(GLFWwindow* window, float deltaTime) {
    float deltaX = paddle.x;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        paddle.x -= paddle.speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        paddle.x += paddle.speed * deltaTime;

    // Mouse control
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    paddle.x = static_cast<float>(mouseX) - paddle.width / 2.0f;

    // Ensure the paddle stays within bounds
    if (paddle.x < 0.0f) paddle.x = 0.0f;
    if (paddle.x + paddle.width > WIDTH) paddle.x = WIDTH - paddle.width;

    deltaX -= paddle.x;
    if (stickyBall) {
        for (auto& ball : balls) {
            if (startFlag) {
                ball.x -= deltaX;
            }
        }
    }

    // Launch the ball
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && stickyBall) {
        for (auto& ball : balls) {
            if (ball.velocityX == 0.0f && ball.velocityY == 0.0f) {
                ball.velocityX = 200.0f;
                ball.velocityY = -200.0f;
            }
            stickyBall = false;
            startFlag = false;
        }

    }
}

void destroy(Block& block) {
    // Уничтожение разрушаемого блока
    if (block.type != INDESTRUCTIBLE) {
        block.health--;
        if (block.health <= 0) {
            block.destroyed = true;
            score += 1;
        }
        if (block.type == SPEED_UP) {
            for (auto& ball : balls) {
                ball.velocityX *= 1.2f;
                ball.velocityY *= 1.2f;
            }
            return;
        }
        // Создание бонуса
        Bonus bonus;
        bonus.x = block.x + block.width / 2 - 10.0f;
        bonus.y = block.y + block.height / 2 - 10.0f;
        bonus.width = 20.0f;
        bonus.height = 20.0f;
        bonus.active = true;
        bonus.type = static_cast<BonusType>(std::rand() % 8);
        bonuses.push_back(bonus);
    }
}

void updateGame(float deltaTime) {
    for (auto& ball : balls) {
        // Обновление позиции шарика
        ball.x += ball.velocityX * deltaTime;
        ball.y += ball.velocityY * deltaTime;

        // Обработка столкновений со стенами
        if (ball.x <= 0.0f || ball.x + ball.radius >= WIDTH) {
            ball.velocityX = -ball.velocityX;
        }
        if (ball.y <= 0.0f) {
            ball.velocityY = -ball.velocityY;
        }

        // Обработка столкновения с платформой
        if (checkCollision(ball, paddle)) {
            ball.velocityY = -ball.velocityY;
            ball.y = paddle.y - ball.radius;
        }

        // Обработка столкновений с блоками
        for (auto& block : blocks) {
            if (checkCollision(ball, block)) {
                double xDist = std::abs(ball.x - (block.x + block.width / 2)) - block.width / 2;
                double yDist = std::abs(ball.y - (block.y + block.height / 2)) - block.height / 2;

                if (xDist < ball.radius && yDist < ball.radius) {
                    if (xDist < yDist) {
                        ball.velocityY = -ball.velocityY;
                    }
                    else if (xDist > yDist) {
                        ball.velocityX = -ball.velocityX;
                    }
                    destroy(block);
                }
            }
        }

        if (ball.y >= HEIGHT) {
            if (oneTimeBottom) {
                oneTimeBottom = false;
                ball.velocityY = -ball.velocityY;
            }
            else if (balls.size() > 1) {
                auto it = std::find(balls.begin(), balls.end(), ball);
                if (it != balls.end()) {
                    balls.erase(it);
                }
            }
            else {
                lives--;
                if (lives <= 0) {
                    std::cout << "Game Over! Your score: " << score << std::endl;
                    initGame();
                }
                else {
                    startFlag = true;
                    stickyBall = true;
                    ball.x = paddle.x + paddle.width / 2;
                    ball.y = paddle.y - 10.0f;
                    ball.velocityX = 0.0f;
                    ball.velocityY = 0.0f;
                }
            }
        }
    }

    // Обновление бонусов
    for (auto& bonus : bonuses) {
        if (bonus.active) {
            bonus.y += 100.0f * deltaTime;

            if (checkCollision(paddle, bonus)) {
                applyBonus(bonus.type);
                bonus.active = false;
            }

            if (bonus.y > HEIGHT) {
                bonus.active = false;
            }
        }
    }
}

void renderBlocks() {
    for (const auto& block : blocks) {
        if (!block.destroyed) {
            auto it = blockColorMap.find(block.type);
            if (it != blockColorMap.end()) {
                glColor3f(std::get<0>(it->second), std::get<1>(it->second), std::get<2>(it->second));
            }

            glBegin(GL_QUADS);
            glVertex2f(block.x, block.y);
            glVertex2f(block.x + block.width, block.y);
            glVertex2f(block.x + block.width, block.y + block.height);
            glVertex2f(block.x, block.y + block.height);
            glEnd();

            if (block.health > 0) {
                glColor3f(0.0f, 0.0f, 0.0f);
                glBegin(GL_LINES);
                for (int i = 0; i < block.health; i++) {
                    glVertex2f(block.x + (i * (block.width / (block.health + 1))), block.y);
                    glVertex2f(block.x + (i * (block.width / (block.health + 1))), block.y + block.height);
                }
                glEnd();
            }
        }
    }
}

// Объявляем функции для рисования объектов
void drawPlus(float x, float y, float size);
void drawMinus(float x, float y, float size);
void drawHeart(float x, float y, float size);
void drawCircle(float x, float y, float radius);
void drawSquare(float x, float y, float size);

// Создаем карту, которая сопоставляет типы бонусов с функциями рисования
std::map<BonusType, std::function<void(float, float, float)>> bonusDrawFuncMap = {
    {BONUS_SIZE_UP, drawPlus},
    {BONUS_SIZE_DOWN, drawMinus},
    {BONUS_SPEED_UP, drawPlus},
    {BONUS_SPEED_DOWN, drawMinus},
    {BONUS_STICKY, drawSquare},
    {BONUS_EXTRA_LIFE, drawHeart},
    {BONUS_EXTRA_BALL, drawCircle},
    {BONUS_ONE_TIME_BOTTOM, drawHeart}
};

void drawPlus(float x, float y, float size) {
    glLineWidth(10);
    const float halfSize = size / 2;
    glBegin(GL_LINES);
    glVertex2f(x - halfSize, y);
    glVertex2f(x + halfSize, y);
    glVertex2f(x, y - halfSize);
    glVertex2f(x, y + halfSize);
    glEnd();
}

void drawMinus(float x, float y, float size) {
    glLineWidth(10);
    const float halfSize = size / 2;
    glBegin(GL_LINES);
    glVertex2f(x - halfSize, y);
    glVertex2f(x + halfSize, y);
    glEnd();
}

void drawHeart(float x, float y, float size) {
    glLineWidth(3);
    const float halfSize = size / 2.0f;
    const float leftX = x - halfSize;
    const float rightX = x + halfSize;
    const float topY = y + halfSize;

    // Рисуем левую половину круга
    glBegin(GL_POLYGON);
    const float leftCircleX = x - halfSize / 2.0f;
    const float leftCircleY = y;
    const float leftCircleRadius = halfSize / 2.0f;
    const int numSegments = 20;
    for (int i = 0; i <= numSegments; ++i) {
        const float angle = M_PI * i / numSegments;
        const float circleX = leftCircleX + leftCircleRadius * std::cos(angle);
        const float circleY = leftCircleY - leftCircleRadius * std::sin(angle);
        if (circleY <= y) {
            glVertex2f(circleX, circleY);
        }
    }
    glEnd();

    // Рисуем правую половину круга
    glBegin(GL_POLYGON);
    const float rightCircleX = x + halfSize / 2.0f;
    const float rightCircleY = y;
    const float rightCircleRadius = halfSize / 2.0f;
    for (int i = 0; i <= numSegments; ++i) {
        const float angle = M_PI * i / numSegments;
        const float circleX = rightCircleX - rightCircleRadius * std::cos(angle);
        const float circleY = rightCircleY - rightCircleRadius * std::sin(angle);
        if (circleY <= y) {
            glVertex2f(circleX, circleY);
        }
    }
    glEnd();

    // Рисуем основание треугольника
    glBegin(GL_TRIANGLES);
    glVertex2f(leftX, y);
    glVertex2f(rightX, y);
    glVertex2f(x, topY);
    glEnd();
}

void drawCircle(float x, float y, float size) {
    glLineWidth(3);
    glBegin(GL_POLYGON);
    const int numSegments = 30;
    for (int i = 0; i <= numSegments; ++i) {
        const float angle = 2.0f * M_PI * i / numSegments;
        const float x1 = x + size / 2 * std::cos(angle);
        const float y1 = y + size / 2 * std::sin(angle);
        glVertex2f(x1, y1);
    }
    glEnd();
}

void drawSquare(float x, float y, float size) {
    glLineWidth(3);
    const float halfSize = size / 2;
    glBegin(GL_QUADS);
    glVertex2f(x - halfSize, y - halfSize);
    glVertex2f(x + halfSize, y - halfSize);
    glVertex2f(x + halfSize, y + halfSize);
    glVertex2f(x - halfSize, y + halfSize);
    glEnd();
}

void renderBonuses() {
    for (const auto& bonus : bonuses) {
        if (bonus.active) {
            auto it = bonusColorMap.find(bonus.type);
            if (it != bonusColorMap.end()) {
                glColor3f(std::get<0>(it->second), std::get<1>(it->second), std::get<2>(it->second));
            }

            auto drawFunc = bonusDrawFuncMap[bonus.type];
            drawFunc(bonus.x, bonus.y, std::max(bonus.width, bonus.height));
        }
    }
}

void renderLives() {
    glColor3f(1.0f, 1.0f, 1.0f);
    for (int i = 0; i < lives; i++) {
        drawHeart(WIDTH - 25.0f * i - 15.0f, 10.0f, 20.0f);
    }
}
// Render game objects
void renderGame() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Render paddle
    glBegin(GL_QUADS);
    glVertex2f(paddle.x, paddle.y);
    glVertex2f(paddle.x + paddle.width, paddle.y);
    glVertex2f(paddle.x + paddle.width, paddle.y + paddle.height);
    glVertex2f(paddle.x, paddle.y + paddle.height);
    glEnd();

    // Render balls
    for (const auto& ball : balls) {
        glBegin(GL_TRIANGLE_FAN);
        for (int i = 0; i < 360; i++) {
            float theta = i * 3.14159f / 180;
            glVertex2f(ball.x + ball.radius * cos(theta), ball.y + ball.radius * sin(theta));
        }
        glEnd();
    }

    renderBlocks();
    renderBonuses();
    renderLives();

    glColor3f(1.0f, 1.0f, 1.0f); // Reset color to white for next frame
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Arkanoid", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WIDTH, HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    initGame();

    float lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        processInput(window, deltaTime);
        updateGame(deltaTime);
        renderGame();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}