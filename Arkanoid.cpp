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

// Block types
enum BlockType {
    INDESTRUCTIBLE,
    BONUS,
    SPEED_UP,
    DESTRUCTIBLE
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
std::vector<Block> blocks;

// Initialize the game objects
void initGame() {
    paddle.x = WIDTH / 2.0f - 50.0f;
    paddle.y = HEIGHT - 30.0f;
    paddle.width = 100.0f;
    paddle.height = 20.0f;
    paddle.speed = 500.0f;

    ball.x = WIDTH / 2.0f;
    ball.y = HEIGHT / 2.0f;
    ball.radius = 10.0f;
    ball.velocityX = 200.0f;
    ball.velocityY = -200.0f;

    blocks.clear();
    // Initialize blocks with different types and properties
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 10; ++j) {
            Block block;
            block.x = j * 80.0f;
            block.y = i * 30.0f;
            block.width = 78.0f;
            block.height = 28.0f;
            block.destroyed = false;
            if (i == 0) {
                block.type = INDESTRUCTIBLE;
                block.health = -1;
            }
            else if (i == 1) {
                block.type = BONUS;
                block.health = 1;
            }
            else if (i == 2) {
                block.type = SPEED_UP;
                block.health = 1;
            }
            else {
                block.type = DESTRUCTIBLE;
                block.health = 2;
            }
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

// Collision detection with blocks
bool checkCollision(Ball& ball, Block& block) {
    return !block.destroyed &&
        ball.x + ball.radius > block.x && ball.x - ball.radius < block.x + block.width &&
        ball.y + ball.radius > block.y && ball.y - ball.radius < block.y + block.height;
}

// Update game state
void updateGame(float deltaTime) {
    ball.x += ball.velocityX * deltaTime;
    ball.y += ball.velocityY * deltaTime;

    if (ball.x <= 0.0f || ball.x + ball.radius >= WIDTH)
        ball.velocityX = -ball.velocityX;
    if (ball.y <= 0.0f)
        ball.velocityY = -ball.velocityY;

    if (checkCollision(ball, paddle)) {
        ball.velocityY = -ball.velocityY;
        ball.y = paddle.y - ball.radius;
    }

    for (auto& block : blocks) {
        if (checkCollision(ball, block)) {
            ball.velocityY = -ball.velocityY;
            if (block.type == SPEED_UP) {
                ball.velocityX *= 1.1f;
                ball.velocityY *= 1.1f;
            }
            else if (block.type == BONUS) {
                // Handle bonus drop (not implemented here)
            }
            if (block.type != INDESTRUCTIBLE) {
                block.health--;
                if (block.health <= 0) {
                    block.destroyed = true;
                    score += 1;
                }
            }
        }
    }

    if (ball.y >= HEIGHT) {
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

    // Render ball
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i < 360; i++) {
        float theta = i * 3.14159f / 180;
        glVertex2f(ball.x + ball.radius * cos(theta), ball.y + ball.radius * sin(theta));
    }
    glEnd();

    // Render blocks
    for (const auto& block : blocks) {
        if (!block.destroyed) {
            if (block.type == INDESTRUCTIBLE) glColor3f(0.8f, 0.8f, 0.8f); // Grey
            else if (block.type == BONUS) glColor3f(0.0f, 1.0f, 0.0f); // Green
            else if (block.type == SPEED_UP) glColor3f(1.0f, 0.0f, 0.0f); // Red
            else glColor3f(1.0f, 1.0f, 0.0f); // Yellow

            glBegin(GL_QUADS);
            glVertex2f(block.x, block.y);
            glVertex2f(block.x + block.width, block.y);
            glVertex2f(block.x + block.width, block.y + block.height);
            glVertex2f(block.x, block.y + block.height);
            glEnd();
        }
    }

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