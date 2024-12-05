#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <vector>

const int GRID_WIDTH = 10;
const int GRID_HEIGHT = 10;

float camX = 0.0f, camY = 2.2f, camZ = 5.0f; 
float camYaw = 0.0f, camPitch = 0.0f;
float playerY = 0.0f;

// Movement 
bool isJumping = false;
float velocityY = 0.0f;
const float gravity = -0.01f; 
const float jumpSpeed = 0.22f; 
const float groundY = 0.0f;   
bool crouch = false;
float lastTime = 0.0f;
bool walking = false;
bool sprinting = false;
float cooldown = 0.0f;
float stamina = 100;
float moveSpeed = 0.1f;
const float turnSpeed = 0.1f;
double lastMouseX, lastMouseY;
bool keys[1024] = {false};
float collisionThreshold = 0.1f;

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void drawGrid(int lines, float spacing, float r, float g, float b);
void drawWalls(int lines, float spacing);
void drawGrids(int lines, float spacing);
void display();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        keys[key] = true;
        if (key == GLFW_KEY_SPACE && !isJumping && playerY == groundY && !crouch && stamina >= 5) {
            isJumping = true;
            velocityY = jumpSpeed;
            stamina -= 3; 
        }
    } else if (action == GLFW_RELEASE) {
        keys[key] = false;
    }
}

struct Platform {
    float x, z;     
    float width, depth; 
    float height;  
    float heightdelta;  
};

// platforms
std::vector<Platform> platforms = {
    {2.0f, 2.0f, 2.0f, 2.0f, 1.0f, 2.0f},
    {-3.0f, -3.0f, 2.0f, 2.0f, 2.0f, 0.0f}
};

void drawCube(float size) {
    float halfSize = size / 2.0f;
    glBegin(GL_QUADS);
    glVertex3f(-halfSize, -halfSize, halfSize);
    glVertex3f(halfSize, -halfSize, halfSize);
    glVertex3f(halfSize, halfSize, halfSize);
    glVertex3f(-halfSize, halfSize, halfSize);

    glVertex3f(-halfSize, -halfSize, -halfSize);
    glVertex3f(halfSize, -halfSize, -halfSize);
    glVertex3f(halfSize, halfSize, -halfSize);
    glVertex3f(-halfSize, halfSize, -halfSize);

    glVertex3f(-halfSize, -halfSize, -halfSize);
    glVertex3f(-halfSize, -halfSize, halfSize);
    glVertex3f(-halfSize, halfSize, halfSize);
    glVertex3f(-halfSize, halfSize, -halfSize);

    glVertex3f(halfSize, -halfSize, -halfSize);
    glVertex3f(halfSize, -halfSize, halfSize);
    glVertex3f(halfSize, halfSize, halfSize);
    glVertex3f(halfSize, halfSize, -halfSize);

    glVertex3f(-halfSize, halfSize, -halfSize);
    glVertex3f(-halfSize, halfSize, halfSize);
    glVertex3f(halfSize, halfSize, halfSize);
    glVertex3f(halfSize, halfSize, -halfSize);

    glVertex3f(-halfSize, -halfSize, -halfSize);
    glVertex3f(-halfSize, -halfSize, halfSize);
    glVertex3f(halfSize, -halfSize, halfSize);
    glVertex3f(halfSize, -halfSize, -halfSize);
    glEnd();
}



void drawPlatforms() {
    glColor3f(0.5f, 0.5f, 0.5f);
    for (const auto& platform : platforms) {
        glPushMatrix();
        glTranslatef(platform.x, (platform.height / 2.0f) + platform.heightdelta, platform.z);
        glScalef(platform.width, platform.height, platform.depth);
        drawCube(1.0f);
        glPopMatrix();
    }
}

float getPlatformHeight(float x, float z, float currentY) {
    for (const auto& platform : platforms) {
        bool isWithinXBounds = x >= platform.x - platform.width / 2 && x <= platform.x + platform.width / 2;
        bool isWithinZBounds = z >= platform.z - platform.depth / 2 && z <= platform.z + platform.depth / 2;
        if (isWithinXBounds && isWithinZBounds) {
            return platform.height + platform.heightdelta;
        }
    }
    return groundY; 
}

bool isNearPlatform(float x, float z, float y, float threshold, float camY) {
    for (const auto& platform : platforms) {
        bool isNearXBounds = (x >= platform.x - platform.width / 2 - threshold && x <= platform.x + platform.width / 2 + threshold);
        bool isNearZBounds = (z >= platform.z - platform.depth / 2 - threshold && z <= platform.z + platform.depth / 2 + threshold);
        bool isBelowPlatformHeight = (y < platform.height);
        bool cancrouchbeneeth = (camY > platform.heightdelta);
        if (isNearXBounds && isNearZBounds && isBelowPlatformHeight && cancrouchbeneeth) {
            return true;
        }
    }
    return false;
}

void updateMovement() {
    float radYaw = camYaw * 3.14159265 / 180.0f;

    float moveX = 0.0f, moveZ = 0.0f;
    if (keys[GLFW_KEY_W]) {
        moveX += sin(radYaw);
        moveZ -= cos(radYaw);
    }
    if (keys[GLFW_KEY_S]) {
        moveX -= sin(radYaw);
        moveZ += cos(radYaw);
    }
    if (keys[GLFW_KEY_A]) {
        moveX -= cos(radYaw);
        moveZ -= sin(radYaw);
    }
    if (keys[GLFW_KEY_D]) {
        moveX += cos(radYaw);
        moveZ += sin(radYaw);
    }
    if ((keys[GLFW_KEY_LEFT_CONTROL] || keys[GLFW_KEY_RIGHT_CONTROL]) && !isJumping && !sprinting) {
        bool canUncrouch = true;
        for (const auto& platform : platforms) {
            if (camX >= platform.x - platform.width / 2 && camX <= platform.x + platform.width / 2 &&
                camZ >= platform.z - platform.depth / 2 && camZ <= platform.z + platform.depth / 2 &&
                camY + 1.0f <= platform.height + platform.heightdelta) {
                canUncrouch = false;
                break;
            }
        }
        if (!crouch && canUncrouch) {
            crouch = true;
            camY -= 1.0f; 
            moveSpeed -= 0.03f;
        }
    } else if ((!keys[GLFW_KEY_LEFT_CONTROL] && !keys[GLFW_KEY_RIGHT_CONTROL]) && crouch ) {
        bool canUncrouch = true;
        for (const auto& platform : platforms) {
            if (camX >= platform.x - platform.width / 2 && camX <= platform.x + platform.width / 2 &&
                camZ >= platform.z - platform.depth / 2 && camZ <= platform.z + platform.depth / 2 &&
                camY + 1.0f <= platform.height + platform.heightdelta) {
                canUncrouch = false;
                break;
            }
        }

        if (canUncrouch) {
            crouch = false;
            camY += 1.0f;
            moveSpeed += 0.03f;
        }
    }


    if ((keys[GLFW_KEY_W]) || (keys[GLFW_KEY_S]) || (keys[GLFW_KEY_A]) || (keys[GLFW_KEY_D])) {
        walking = true;
        if (!sprinting && cooldown == 0 && stamina < 100) {
            stamina += 0.1f;
        }
    } else {
        walking = false;
    }

    if (((keys[GLFW_KEY_LEFT_SHIFT] || keys[GLFW_KEY_RIGHT_SHIFT]) && (not(crouch)) && (stamina > 0)) &&
        (keys[GLFW_KEY_W] || keys[GLFW_KEY_S] || keys[GLFW_KEY_A] || keys[GLFW_KEY_D])) {
        if (!sprinting) {
            moveSpeed += 0.05f;
        }
        stamina -= 0.2f;
        sprinting = true;
    } else if (((keys[GLFW_KEY_LEFT_SHIFT] || keys[GLFW_KEY_RIGHT_SHIFT]) && (not(crouch))  && stamina <= 0) &&
        (keys[GLFW_KEY_W] || keys[GLFW_KEY_S] || keys[GLFW_KEY_A] || keys[GLFW_KEY_D])) {
            if (sprinting) {
                moveSpeed-=0.05f;
            }
            sprinting=false;
    } else if (!(keys[GLFW_KEY_LEFT_SHIFT] || keys[GLFW_KEY_RIGHT_SHIFT])) {
        if (sprinting && stamina <= 25) {
            cooldown = 5.0f;
        }
        if (sprinting) {
            moveSpeed-=0.05f;
        }
        sprinting = false;
    }

    float currentTime = glfwGetTime();
    float deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    if (cooldown > 0.0f) {
        cooldown -= deltaTime;
        if (cooldown < 0.0f) {
            cooldown = 0.0f;
        }
    }

    float length = sqrt(moveX * moveX + moveZ * moveZ);
    if (length > 0.0f) {
        moveX /= length;
        moveZ /= length;
    }
    if (!walking && cooldown == 0 && stamina < 100) {
        stamina += 0.2f;
    }

    float nextCamX = camX + moveX * moveSpeed;
    float nextCamZ = camZ + moveZ * moveSpeed;

    float platformHeight = getPlatformHeight(camX, camZ, playerY); 
    if (isJumping) {
        velocityY += gravity;
        playerY += velocityY;
        if (!crouch) {
            camY = playerY + 2.2f;
        }
        if (playerY <= platformHeight && velocityY <= 0) {
            playerY = platformHeight;
            if (!crouch) {
                camY = playerY + 2.2f;
            }
            isJumping = false;
            velocityY = 0.0f;
        }
    } else {
        if (playerY > platformHeight) {
            isJumping = true;
            velocityY = 0.0f;
            if (crouch) {
                crouch=false;
            }
        } else {
            playerY = platformHeight;
            if (!crouch) {
                camY = playerY + 2.2f;
            }

            if (keys[GLFW_KEY_SPACE]&& !crouch && stamina >= 5) {
                isJumping = true;
                velocityY = jumpSpeed; 
            }

        }
    }
    if (!isJumping && isNearPlatform(nextCamX, nextCamZ, playerY, collisionThreshold, camY)) {
        moveX = 0.0f;
        moveZ = 0.0f;
    }
    camX += moveX * moveSpeed;
    camZ += moveZ * moveSpeed;
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    double xoffset = xpos - lastMouseX;
    double yoffset = lastMouseY - ypos; 

    lastMouseX = xpos;
    lastMouseY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    camYaw += xoffset;
    camPitch -= yoffset;

    if (camYaw > 180.0f) {
        camYaw -= 360.0f;
    }
    if (camYaw < -180.0f) {
        camYaw += 360.0f;
    }

    if (camPitch > 89.0f) camPitch = 89.0f;
    if (camPitch < -89.0f) camPitch = -89.0f;
}

void drawGrid(int lines, float spacing, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_LINES);
    for (int i = -lines; i <= lines; ++i) {
        glVertex3f(i * spacing, 0.0f, -lines * spacing);
        glVertex3f(i * spacing, 0.0f, lines * spacing);
        glVertex3f(-lines * spacing, 0.0f, i * spacing);
        glVertex3f(lines * spacing, 0.0f, i * spacing);
    }
    glEnd();
}

void drawGrids(int lines, float spacing) {

    glPushMatrix();
    glTranslatef(0.0f, -0.01f, 0.0f); 
    drawGrid(lines, spacing, 0.0f, 1.0f, 0.0f); 
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, lines * spacing * 2.0f, 0.0f);
    drawGrid(lines, spacing, 0.0f, 0.0f, 1.0f); 
    glPopMatrix();

    drawWalls(lines, spacing);
}

void drawWalls(int lines, float spacing) {

    glPushMatrix();
    glTranslatef(0.0f, lines * spacing, lines * spacing);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    drawGrid(lines, spacing, 1.0f, 0.0f, 0.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, lines * spacing, -lines * spacing);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    drawGrid(lines, spacing, 1.0f, 1.0f, 0.0f); 
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-lines * spacing, lines * spacing, 0.0f);
    glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
    drawGrid(lines, spacing, 0.0f, 1.0f, 1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(lines * spacing, lines * spacing, 0.0f);
    glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
    drawGrid(lines, spacing, 1.0f, 0.0f, 1.0f); 
    glPopMatrix();
}



class GUI {
public:
    static void drawRect(float x, float y, float width, float height, float r, float g, float b) {
        glColor3f(r, g, b);
        glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + width, y);
        glVertex2f(x + width, y + height);
        glVertex2f(x, y + height);
        glEnd();
    }
    static void drawStaminaBar(float x, float y, float width, float height, float stamina) {
        if (stamina < 0) stamina = 0;
        if (stamina > 100) stamina = 100;
        drawRect(x, y, width, height, 0.2f, 0.2f, 0.2f); 
        float barWidth = width * (stamina / 100.0f); 
        drawRect(x, y, barWidth, height, 0.0f, 0.0f, 1.0f); 
    }
    static void drawHUD() {

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, 640, 480, 0, -1, 1);  
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        drawStaminaBar(10.0f, 10.0f, 100.0f, 10.0f, stamina); 
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }
};

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    updateMovement();
    glLoadIdentity();
    glRotatef(camPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(camYaw, 0.0f, 1.0f, 0.0f);
    glTranslatef(-camX, -camY, -camZ);
    drawGrids(GRID_WIDTH, 1.0f);
    drawPlatforms();
    GUI::drawHUD(); 
    glfwSwapBuffers(glfwGetCurrentContext());
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)width / height, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(1920, 1080, "3D Grid Engine", NULL, NULL);
    if (!window) {
        glfwTerminate();
        std::cerr << "Failed to create GLFW window" << std::endl;
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    glfwSetKeyCallback(window, keyCallback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
    glEnable(GL_DEPTH_TEST);
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    framebuffer_size_callback(window, width, height);
    while (!glfwWindowShouldClose(window)) {

        display();

        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}