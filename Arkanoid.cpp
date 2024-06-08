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
} ball;

// Типы блоков в игре Арканоид
enum BlockType {
    INDESTRUCTIBLE,  // 1) Неразрушаемые блоки
    BONUS_SIZE_UP,    // 5) Бонус, увеличивающий размер каретки
    BONUS_SIZE_DOWN,  // 5) Бонус, уменьшающий размер каретки
    BONUS_SPEED_UP,   // 5) Бонус, увеличивающий скорость шарика
    BONUS_SPEED_DOWN, // 5) Бонус, уменьшающий скорость шарика
    BONUS_STICKY,     // 5) Бонус, увеличивающий прилипание шарика к каретке
    BONUS_EXTRA_LIFE,// дающий дополнительную жизнь
    BONUS_EXTRA_BALL,// 5, 9) Бонус, создающий второй шарик
    BONUS_ONE_TIME_BOTTOM, // 6) Бонус, создающий одноразовое дно для шарика
    SPEED_UP,        // 3) Блоки, увеличивающие скорость шарика при столкновении
    DESTRUCTIBLE     // 4) Разрушаемые блоки с уровнем здоровья
};


// Block
struct Block {
    float x, y;
    float width, height;
    BlockType type;
    int health;
    bool destroyed;
};

// Game state
int score = 0;
int lives = 3;
bool stickyBall = false;
bool oneTimeBottom = false;
std::vector<Block> blocks;
std::vector<Ball> balls;

void initGame() {
    // Инициализация весла
    paddle.x = WIDTH / 2.0f - 50.0f;
    paddle.y = HEIGHT - 30.0f;
    paddle.width = 100.0f;
    paddle.height = 20.0f;
    paddle.speed = 500.0f;

    // Инициализация мячей
    balls.clear();
    Ball initialBall = { WIDTH / 2.0f, HEIGHT / 2.0f, 10.0f, 200.0f, -200.0f };
    balls.push_back(initialBall);

    // Инициализация блоков
    blocks.clear();

    // Создание map для связи типа блока с его здоровьем
    std::map<BlockType, int> blockHealth = {
        {INDESTRUCTIBLE, -1},
        {BONUS_SIZE_UP, 1},
        {BONUS_SIZE_DOWN, 1},
        {BONUS_SPEED_UP, 1},
        {BONUS_SPEED_DOWN, 1},
        {BONUS_STICKY, 1},
        {BONUS_EXTRA_LIFE, 1},
        {BONUS_EXTRA_BALL, 1},
        {BONUS_ONE_TIME_BOTTOM, 1},
        {SPEED_UP, 1},
        {DESTRUCTIBLE, 2}
    };

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
            int randomTypeIndex = std::rand() % blockHealth.size();
            auto it = blockHealth.begin();
            std::advance(it, randomTypeIndex);
            block.type = it->first;
            block.health = it->second;

            blocks.push_back(block);
        }
    }
}
// Input handling
void processInput(GLFWwindow* window, float deltaTime) {
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
}

// Collision detection with paddle
bool checkCollision(Ball& ball, Paddle& paddle) {
    return ball.x + ball.radius > paddle.x && ball.x - ball.radius < paddle.x + paddle.width &&
        ball.y + ball.radius > paddle.y && ball.y - ball.radius < paddle.y + paddle.height;
}

//Cтолкновений с блоками
bool checkCollision(Ball& ball, Block& block) {
    return !block.destroyed &&
        ball.x + ball.radius > block.x && ball.x - ball.radius < block.x + block.width &&
        ball.y + ball.radius > block.y && ball.y - ball.radius < block.y + block.height;
}




// Apply bonus effect
void applyBonus(BlockType type) {
    switch (type) {
    case BONUS_SIZE_UP:
        paddle.width *= 1.2f;  // Меньше увеличение размера
        break;
    case BONUS_SIZE_DOWN:
        paddle.width *= 0.8f;
        break;
    case BONUS_SPEED_UP:
        ball.velocityX *= 1.2f;
        ball.velocityY *= 1.2f;
        break;
    case BONUS_SPEED_DOWN:
        ball.velocityX *= 0.8f;
        ball.velocityY *= 0.8f;
        break;
    case BONUS_STICKY:
        stickyBall = true;
        break;
    case BONUS_EXTRA_LIFE:
        lives++;
        break;
    case BONUS_EXTRA_BALL: {
        Ball newBall = ball;
        newBall.velocityX = -newBall.velocityX;
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

void destroy(Block& block) {
    // Уничтожение разрушаемого блока
    if (block.type != INDESTRUCTIBLE) {
        block.health--;
        if (block.health <= 0) {
            block.destroyed = true;
            score += 1;
            applyBonus(block.type);
        }
    }
}

// Update game state
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
                        // Отражение шарика вверх при столкновении с блоком
                        ball.velocityY = -ball.velocityY;
                    }
                    else if (xDist > yDist) {
                        // Отражение шарика вверх при столкновении с блоком
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
            else {
                lives--;
                if (lives <= 0) {
                    std::cout << "Game Over! Your score: " << score << std::endl;
                    initGame();
                }
                else {
                    ball.x = WIDTH / 2.0f;
                    ball.y = HEIGHT / 2.0f;
                    ball.velocityX = 200.0f;
                    ball.velocityY = -200.0f;
                }
            }
        }
    }
}


void renderBlocks() {
    for (const auto& block : blocks) {
        if (!block.destroyed) {
            if (block.type == INDESTRUCTIBLE) glColor3f(0.8f, 0.8f, 0.8f); // Grey
            else if (block.type == BONUS_SIZE_UP || block.type == BONUS_EXTRA_LIFE) glColor3f(0.0f, 1.0f, 0.0f); // Green
            else if (block.type == BONUS_SIZE_DOWN || block.type == BONUS_SPEED_DOWN) glColor3f(1.0f, 0.5f, 0.0f); // Orange
            else if (block.type == BONUS_SPEED_UP) glColor3f(1.0f, 0.0f, 0.0f); // Red
            else if (block.type == BONUS_STICKY) glColor3f(0.0f, 0.0f, 1.0f); // Blue
            else if (block.type == BONUS_EXTRA_BALL || block.type == BONUS_ONE_TIME_BOTTOM) glColor3f(1.0f, 1.0f, 0.0f); // Yellow
            else glColor3f(1.0f, 1.0f, 0.0f); // Yellow

            glBegin(GL_QUADS);
            glVertex2f(block.x, block.y);
            glVertex2f(block.x + block.width, block.y);
            glVertex2f(block.x + block.width, block.y + block.height);
            glVertex2f(block.x, block.y + block.height);
            glEnd();

            // Draw cracks proportional to health
            if (block.health > 0) {
                glColor3f(0.0f, 0.0f, 0.0f); // Black for cracks
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

    // Render score and lives will be leter

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