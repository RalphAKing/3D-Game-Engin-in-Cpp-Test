#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const int GRID_WIDTH = 10;
const int GRID_HEIGHT = 10;

float camX = 0.0f, camY = 2.2f, camZ = 5.0f; 
float camYaw = 0.0f, camPitch = 0.0f;
float playerY = 0.0f;

float frameCount = 0;
float lastFPSUpdate = 0.0f;
int Vsync = 0; //if 0 then disabled if 1 then enabled
bool FPScount = false; //turns on a fps count which is displayed in the top right of screen 

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
float collisionThreshold = 0.0f;

bool menue = false;
float savedCamX = 0.0f, savedCamY = 0.0f, savedCamZ = 0.0f;
float savedCamYaw = 0.0f, savedCamPitch = 0.0f;
bool wasMenuClosed = true;

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void drawGrid(int lines, float spacing, float r, float g, float b);
void drawWalls(int lines, float spacing);
void drawGrids(int lines, float spacing);
void display();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

GLuint fontTexture;
int fontWidth, fontHeight;
GLuint platformTexture;

void loadFont(const char* filePath) {
    int channels;
    unsigned char* image = stbi_load(filePath, &fontWidth, &fontHeight, &channels, 4);
    if (image == nullptr) {
        std::cerr << "Failed to load font texture! Path: " << filePath << std::endl;
        std::cerr << "Error: " << stbi_failure_reason() << std::endl;
        return;
    }

    std::cout << "Font texture loaded: " << fontWidth << "x" << fontHeight << " pixels" << std::endl;

    glGenTextures(1, &fontTexture);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
  
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fontWidth, fontHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    stbi_image_free(image);
}

GLuint loadTexture(const char* filename) {
    int width, height, nrChannels;
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << filename << std::endl;
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    return textureID;
}

void renderText(const std::string& text, float x, float y, float scale, GLuint fontTexture) {
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, fontTexture);

    glPushMatrix();
    glLoadIdentity();
    glTranslatef(x, y, 0.0f);
    if (menue) {
        glScalef(scale, -scale, 1.0f); 
    } else {
        glScalef(scale, scale, 1.0f);
    }

    for (char c : text) {
        if (c == ' ') {
            glTranslatef(0.6f, 0.0f, 0.0f);
            continue;
        }

        int charIndex = (int)c;
        float u = (charIndex % 16) / 16.0f;
        float v = (charIndex / 16) / 16.0f;
        float charWidth = 1.0f / 16.0f;
        float charHeight = 1.0f / 16.0f;

        glBegin(GL_QUADS);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glTexCoord2f(u, v + charHeight);
        glVertex2f(0.0f, 1.0f);
        glTexCoord2f(u + charWidth, v + charHeight);
        glVertex2f(1.0f, 1.0f);
        glTexCoord2f(u + charWidth, v);
        glVertex2f(1.0f, 0.0f);
        glTexCoord2f(u, v);
        glVertex2f(0.0f, 0.0f);

        glEnd();

        glTranslatef(0.6f, 0.0f, 0.0f);
    }

    glPopMatrix();
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

float getTextWidth(const std::string& text, float scale) {
    float width = 0.0f;
    for (size_t i = 0; i < text.length(); i++) {
        char c = text[i];
        if (c == ' ') {
            width += 0.6f;
            continue;
        }
        if (i == 0 || i == text.length() - 1) {
            width += 0.8f;
        } else {
            width += 0.6f;
        }
    }
    return (width * scale);
}

std::string typingText(const std::string& text, float x, float y, float scale, GLuint fontTexture) {
    static size_t charIndex = 0;
    static float lastPrintTime = 0.0f;
    float currentTime = glfwGetTime();

    std::string output = text.substr(0, charIndex);
    if (charIndex < text.length()) {
        output += "_";
    }

    if (currentTime - lastPrintTime >= 0.1f && charIndex <= text.length()) {
        charIndex++;
        lastPrintTime = currentTime;
    }
    
    if (charIndex > text.length()) {
        charIndex = 0;
    }

    return output;
}





struct menuedata {
    std::string text;
}; 

menuedata menueData[] = {
    {"Quit"},
    {"Toggle FPS"},
    {"Toggle VSync"},
    {"Resume"}
};

struct Platform {
    float x, z;     
    float width, depth; 
    float height;  
    float heightdelta;  
};

// platforms
std::vector<Platform> platforms = {
    {2.0f, 2.0f, 2.0f, 2.0f, 1.0f, 1.5f},
    {-3.0f, -3.0f, 2.0f, 2.0f, 2.0f, 0.0f}
};

void drawCube(float size) {
    float halfSize = size / 2.0f;
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);  
    glCullFace(GL_BACK);    
    glFrontFace(GL_CCW);    
    glBindTexture(GL_TEXTURE_2D, platformTexture);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-halfSize, -halfSize, halfSize);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(halfSize, -halfSize, halfSize);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(halfSize, halfSize, halfSize);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-halfSize, halfSize, halfSize);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-halfSize, -halfSize, -halfSize);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-halfSize, halfSize, -halfSize);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(halfSize, halfSize, -halfSize);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(halfSize, -halfSize, -halfSize);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-halfSize, halfSize, -halfSize);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-halfSize, halfSize, halfSize);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(halfSize, halfSize, halfSize);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(halfSize, halfSize, -halfSize);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-halfSize, -halfSize, -halfSize);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(halfSize, -halfSize, -halfSize);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(halfSize, -halfSize, halfSize);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-halfSize, -halfSize, halfSize);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(halfSize, -halfSize, -halfSize);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(halfSize, halfSize, -halfSize);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(halfSize, halfSize, halfSize);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(halfSize, -halfSize, halfSize);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-halfSize, -halfSize, -halfSize);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-halfSize, -halfSize, halfSize);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-halfSize, halfSize, halfSize);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-halfSize, halfSize, -halfSize);
    glEnd();
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
}

void drawPlatforms() {
    for (const auto& platform : platforms) {
        glPushMatrix();
        glTranslatef(platform.x, (platform.height / 2.0f) + platform.heightdelta, platform.z);
        glScalef(platform.width, platform.height, platform.depth);
        drawCube(1.0f);
        glPopMatrix();
    }
}

float getPlatformHeight(float x, float z, float currentY) {
    float closestPlatformHeight = groundY;
    for (const auto& platform : platforms) {
        bool isWithinXBounds = x >= platform.x - platform.width / 2 && x <= platform.x + platform.width / 2;
        bool isWithinZBounds = z >= platform.z - platform.depth / 2 && z <= platform.z + platform.depth / 2;

        if (isWithinXBounds && isWithinZBounds) {
            if (platform.height + platform.heightdelta <= currentY) {
                if (platform.height + platform.heightdelta > closestPlatformHeight) {
                    closestPlatformHeight = platform.height + platform.heightdelta;
                }
            }
        }
    }

    return closestPlatformHeight;
}

bool isNearPlatform(float x, float z, float y, float camY) {
    for (const auto& platform : platforms) {
        bool isNearXBounds = (x >= platform.x - platform.width / 2 - collisionThreshold && x <= platform.x + platform.width / 2 + collisionThreshold);
        bool isNearZBounds = (z >= platform.z - platform.depth / 2 - collisionThreshold && z <= platform.z + platform.depth / 2 + collisionThreshold);
        

        bool isBelowPlatform = (y <= platform.height + platform.heightdelta - 0.1f);
        bool canwalkbeneeth = (camY <= platform.height + platform.heightdelta - 0.1f ); 


        if (isNearXBounds && isNearZBounds) {
            if ((camY-y)<=platform.height+platform.heightdelta - 0.1f && !(platform.heightdelta>(camY-y)) && canwalkbeneeth) {
                return true;  
            }
            
            if (!canwalkbeneeth && isBelowPlatform ) {
                return true;  
            }
        }
    }
    return false;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        keys[key] = true;
        if (key == GLFW_KEY_ESCAPE) {
            menue = !menue;
            if (menue) {
                savedCamX = camX;
                savedCamY = camY;
                savedCamZ = camZ;
                savedCamYaw = camYaw;
                savedCamPitch = camPitch;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            } else {
                camX = savedCamX;
                camY = savedCamY;
                camZ = savedCamZ;
                camYaw = savedCamYaw;
                camPitch = savedCamPitch;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                wasMenuClosed=true;
            }
        }

        if (key == GLFW_KEY_SPACE && !isJumping && playerY == groundY && !crouch && stamina >= 5) {

            isJumping = true;
            velocityY = jumpSpeed;
            stamina -= 3;
        }
    } else if (action == GLFW_RELEASE) {
        keys[key] = false;
    }
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
        if (!crouch ) {
            crouch = true;
            camY -= 1.0f; 
            moveSpeed -= 0.03f;
        }
    } else if ((!keys[GLFW_KEY_LEFT_CONTROL] && !keys[GLFW_KEY_RIGHT_CONTROL]) && crouch ) {
        bool canUncrouch = true;
        for (const auto& platform : platforms) {
            if (camX >= platform.x - platform.width / 2 && camX <= platform.x + platform.width / 2 &&
                camZ >= platform.z - platform.depth / 2 && camZ <= platform.z + platform.depth / 2 &&
                camY + 1.0f >= platform.heightdelta) {
                canUncrouch = false;
                if (playerY + 0.1 >= platform.heightdelta) {
                    canUncrouch = true;
                }
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

    if (isNearPlatform(nextCamX, nextCamZ, playerY, camY)) {
        moveX = 0.0f;
        moveZ = 0.0f;
        velocityY=0.0f;
    }
    camX += moveX * moveSpeed;
    camZ += moveZ * moveSpeed;


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
                camY += 1.0f;
                moveSpeed += 0.03f;
            }
        } else {
            if (platformHeight < playerY) {
                playerY = platformHeight;
            }
            if (!crouch) {
                camY = playerY + 2.2f;
            }

            if (keys[GLFW_KEY_SPACE]&& !crouch && stamina >= 5) {
                isJumping = true;
                velocityY = jumpSpeed; 
            }

        }
    }

}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    static bool firstMouse = true;
    
    if (firstMouse) {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastMouseX;
    float yoffset = ypos - lastMouseY; 
    lastMouseX = xpos;
    lastMouseY = ypos;

    xoffset *= turnSpeed;
    yoffset *= turnSpeed;

    camYaw += xoffset;
    camPitch += yoffset;

    if (camPitch > 89.0f) camPitch = 89.0f;
    if (camPitch < -89.0f) camPitch = -89.0f;
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && menue) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        float buttonWidth = 300.0f;
        float buttonHeight = 100.0f;
        float buttonSpacing = 50.0f;
        int numButtons = std::size(menueData);
        float totalHeight = numButtons * buttonHeight + (numButtons - 1) * buttonSpacing;
        float startY = (height - totalHeight) / 2.0f;
        ypos = height - ypos;

        for (int i = 0; i < numButtons; i++) {
            float buttonX = (width - buttonWidth) / 2.0f;
            float buttonY = startY + i * (buttonHeight + buttonSpacing);

            if (xpos >= buttonX && xpos <= buttonX + buttonWidth &&
                ypos >= buttonY && ypos <= buttonY + buttonHeight) {
                if (menueData[i].text == "Quit") {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                } else if (menueData[i].text == "Resume") {
                    menue = false;
                    camX = savedCamX;
                    camY = savedCamY;
                    camZ = savedCamZ;
                    camYaw = savedCamYaw;
                    camPitch = savedCamPitch;
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    wasMenuClosed = true;
                } else if (menueData[i].text == "Toggle FPS") {
                    FPScount = !FPScount;
                } if (menueData[i].text == "Toggle VSync") {
                    Vsync = !Vsync;  
                    glfwSwapInterval(Vsync);  
                }
            }
        }
    }
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
        if (FPScount) {
            std::string fpsText = "FPS: " + std::to_string(static_cast<int>(frameCount / (glfwGetTime() - lastFPSUpdate)));
            renderText(fpsText, 540.0f, 10.0f, 15.0f, fontTexture); 
        }
        drawStaminaBar(10.0f, 10.0f, 100.0f, 10.0f, stamina); 

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }
};

class MENU {
public:
    static void drawRectangle(float x, float y, float width, float height) {
        glColor3f(0.2, 0.2, 0.2);
        glBegin(GL_QUADS);
        glVertex2f(x, y);               
        glVertex2f(x + width, y);      
        glVertex2f(x + width, y + height); 
        glVertex2f(x, y + height);     
        glEnd();
    }
    static void drawMENU(GLFWwindow* window) {
        static int currentButton = 0;
        static std::vector<size_t> charIndices;
        int width, height;
        if (wasMenuClosed) {
            currentButton = 0;
            charIndices.clear();
            wasMenuClosed = false;
        }
        glfwGetWindowSize(window, &width, &height);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, width, 0, height, -1, 1); 
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        int numButtons = std::size(menueData);
        float buttonWidth = 400.0f;
        float buttonHeight = 100.0f;
        float buttonSpacing = 50.0f;
        float totalHeight = numButtons * buttonHeight + (numButtons - 1) * buttonSpacing;
        float startY = (height - totalHeight) / 2.0f;

        
        if (charIndices.empty()) {
            charIndices.resize(numButtons, 0);
        }

        for (int i = 0; i < numButtons; ++i) {
            float buttonX = (width - buttonWidth) / 2.0f;
            float buttonY = startY + i * (buttonHeight + buttonSpacing);
            drawRectangle(buttonX, buttonY, buttonWidth, buttonHeight);

            float textwidth = getTextWidth(menueData[i].text, 55.0f);
            float textX = buttonX + (buttonWidth - textwidth) / 2.0f;  
            float textY = buttonY + (buttonHeight + 55.0f) / 2.0f;

            if (i < currentButton) {
                renderText(menueData[i].text, textX, textY, 55.0f, fontTexture);
            } else if (i == currentButton) {
                std::string displayText = menueData[i].text.substr(0, charIndices[i]);
                if (charIndices[i] < menueData[i].text.length()) {
                    displayText += "_";
                }
                renderText(displayText, textX, textY, 55.0f, fontTexture);
                
                static float lastPrintTime = 0.0f;
                float currentTime = glfwGetTime();
                if (currentTime - lastPrintTime >= 0.2f) {
                    charIndices[i]++;
                    lastPrintTime = currentTime;
                    if (charIndices[i] > menueData[i].text.length()) {
                        currentButton++;
                        charIndices[i] = 0;
                    }
                }
            }
        }


        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }
};

void display(GLFWwindow* window) {
    static float accumulator = 0.0f;
    static const float fixedTimeStep = 1.0f / 60.0f; 
    float currentTime = glfwGetTime();
    float deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    if (menue) {
        MENU::drawMENU(window);
    } else {

        accumulator += deltaTime;
        while (accumulator >= fixedTimeStep) {
            updateMovement();  
            accumulator -= fixedTimeStep;  
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        glLoadIdentity();
        glRotatef(camPitch, 1.0f, 0.0f, 0.0f);
        glRotatef(camYaw, 0.0f, 1.0f, 0.0f);
        glTranslatef(-camX, -camY, -camZ);

        drawGrids(GRID_WIDTH, 1.0f);
        drawPlatforms();
        GUI::drawHUD(); 
    }

    if (FPScount) {
        frameCount++;
        if (currentTime - lastFPSUpdate >= 1.0f) {
        lastFPSUpdate = currentTime;
        frameCount = 0; 
        }
        frameCount++;
    }
    


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
    loadFont("assets/font.png");
    if (fontTexture == 0) {
        std::cerr << "Font texture failed to load" << std::endl;
        return -1;
    }

    platformTexture = loadTexture("assets/platform.jpeg");
    if (platformTexture == 0) {
        std::cerr << "Platform texture failed to load" << std::endl;
        return -1;
    }


    glfwSwapInterval(Vsync);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    if (menue) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
    glEnable(GL_DEPTH_TEST);
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    framebuffer_size_callback(window, width, height);
    while (!glfwWindowShouldClose(window)) {

        display(window);

        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}