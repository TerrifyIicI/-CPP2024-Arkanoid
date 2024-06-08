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
}

// Input handling
void processInput(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        paddle.x -= paddle.speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        paddle.x += paddle.speed * deltaTime;
}

// Collision detection
bool checkCollision(Ball& ball, Paddle& paddle) {
    if (ball.x + ball.radius > paddle.x && ball.x - ball.radius < paddle.x + paddle.width &&
        ball.y + ball.radius > paddle.y && ball.y - ball.radius < paddle.y + paddle.height) {
        return true;
    }
    return false;
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

    if (ball.y >= HEIGHT) {
        initGame();
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

// Function prototypes
void initGLFW();
void initGLEW();
void initOpenGL(GLFWwindow*& window);
//void gameLoop(GLFWwindow* window, GameRenderer& ren, Shader& ourShader);
void cleanup(GLuint& VAO, GLuint& VBO);



void initGLFW() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
}

void initGLEW() {
    glewExperimental = GL_TRUE;
    glewInit();
}

// void initOpenGL(GLFWwindow*& window) {
//     window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", nullptr, nullptr);
//     glfwMakeContextCurrent(window);

//     initGLEW();

//     glViewport(0, 0, WIDTH, HEIGHT);
// }

// void gameLoop(GLFWwindow* window, GameRenderer& ren, Shader& ourShader) {
//     while (!glfwWindowShouldClose(window))
//     {
//         glfwPollEvents();

//         glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
//         glClear(GL_COLOR_BUFFER_BIT);

//         ourShader.Use();
//         ren.GEMS();

//         glfwSwapBuffers(window);
//     }
// }

void cleanup(GLuint& VAO, GLuint& VBO) {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
}

// int main()
// {
//     initGLFW();

//     GLFWwindow* window;
//     initOpenGL(window);

//     // Build and compile our shader program
//     Shader ourShader("../shaders/default.vs", "../shaders/default.frag");
//     GLuint VBO, VAO;
//     GLint vertexColorLocation = glGetUniformLocation(ourShader.Program, "ourColor");

//     GameRenderer ren = GameRenderer(VBO, VAO, (GLfloat)x_parts, (GLfloat)y_parts, SQUARE, vertexColorLocation);

//     glfwSetMouseButtonCallback(window, ren.mouse_button_callback);

//     gameLoop(window, ren, ourShader);

//     cleanup(VAO, VBO);

//     return 0;
// }