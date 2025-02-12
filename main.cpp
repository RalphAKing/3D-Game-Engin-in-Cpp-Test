#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "src/include/stb_image.h"
#include "src/include/json.hpp"
#include <fstream>
using json = nlohmann::json;

const int GRID_WIDTH = 10;
const int GRID_HEIGHT = 10;
GLuint gridVBO, gridVAO;
std::vector<float> gridVertices;

float camX = 0.0f, camY = 2.2f, camZ = 5.0f; 
float camYaw = 0.0f, camPitch = 0.0f;
float playerY = 0.0f;
bool isHoldingObject;
int heldObjectIndex;
float holdDistance = 5.0f;

float frameCount = 0;
float lastFPSUpdate = 0.0f;
float savedCamX = 0.0f, savedCamY = 0.0f, savedCamZ = 0.0f;
float savedCamYaw = 0.0f, savedCamPitch = 0.0f;

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
bool nojump = false;
float collisionThreshold = 0.5f;

bool menue = false;
bool wasMenuClosed = true;
bool wireframs = false;
int Vsync = 0;
bool FPScount = false; 
bool isFullscreen = true;
std::string lastmap;
std::string currentmap;


void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void drawGrid(int lines, float spacing, float r, float g, float b);
void drawWalls(int lines, float spacing);
void drawGrids(int lines, float spacing);
void drawCube(float size, GLuint textureID);
void updateMovement();
void drawPlatforms();
void drawPlatformWireframes();
void display();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void loadFont(const char* filePath);
void renderText(const std::string& text, float x, float y, float scale, GLuint fontTexture);
void loadLevelTextures();

GLuint fontTexture;
int fontWidth, fontHeight;
GLuint platformTexture;

void saveSettings() {
    json settingsData;
    settingsData["vsync"] = Vsync;
    settingsData["fpscount"] = FPScount; 
    settingsData["wireframs"] = wireframs;
    settingsData["lastmap"] = currentmap;

    std::ofstream file("assets/settings.json");
    file << settingsData.dump(4);
}

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

void toggleFullscreen(GLFWwindow* window) {
    isFullscreen = !isFullscreen;
    if (isFullscreen) {
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
    } else {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        height -= 100;
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window, nullptr, (mode->width - width) / 2, (mode->height - height) / 2, width, height, 0);
    }
}



struct Frustum {
    float nearD, farD, aspect, fov;
    float nearH, nearW, farH, farW;
    
    void update(float fovDegrees, float aspectRatio, float nearDist, float farDist) {
        fov = fovDegrees * 3.14159f / 180.0f;
        nearD = nearDist;
        farD = farDist;
        aspect = aspectRatio;
        
        nearH = 2 * tan(fov / 2) * nearD;
        nearW = nearH * aspect;
        farH = 2 * tan(fov / 2) * farD;
        farW = farH * aspect;
    }
};

struct menuedata {
    std::string text;
}; 

menuedata menueData[] = {
    {"Quit"},
    {"Toggle FPS"},
    {"Toggle VSync"},
    {"Wireframs"},
    {"Fullscreen"},
    {"Resume"}
};

struct Platform {
    float x, z;     
    float width, depth; 
    float height;  
    float heightdelta;
    std::string changemap;
    std::string texturePath;
    GLuint textureID;
};

std::vector<Platform> platforms = {
};

struct PhysicsObject {
    float x, y, z;
    float size; 
    std::string texturePath;
    GLuint textureID;

    float velocityX, velocityY, velocityZ;

    float rotationX, rotationY, rotationZ;
    float angularVelocityX, angularVelocityY, angularVelocityZ;
  
    float mass;
    float friction;
    float restitution; 

    float wobbleX, wobbleZ;
    float wobblePhase;
    float randomSpin;

    PhysicsObject() : x(0.0f), y(0.0f), z(0.0f), size(1.0f) {
        velocityX = velocityY = velocityZ = 0.0f;
        rotationX = rotationY = rotationZ = 0.01f;
        angularVelocityX = angularVelocityY = angularVelocityZ = 0.02f;
        mass = 5.0f;
        friction = 2.5f; 
        restitution = 0.3f;  

        wobbleX = wobbleZ = 0.05f;
        wobblePhase = 0.0f;
        randomSpin = 0.0f;
        
    }
    
    void update(float deltaTime) {
        const float GRAVITY = -9.81f * 2;
        const float VELOCITY_THRESHOLD = 0.01f;
        const float ANGULAR_DAMPING = 0.8f;
        velocityY += GRAVITY * deltaTime;
        float prevX = x;
        float prevY = y;
        float prevZ = z;

        x += velocityX * deltaTime;
        y += velocityY * deltaTime;
        z += velocityZ * deltaTime;
        for (const auto& platform : platforms) {
            bool isWithinXBounds = x >= platform.x - platform.width/2 - size/2 && 
                                x <= platform.x + platform.width/2 + size/2;
            bool isWithinZBounds = z >= platform.z - platform.depth/2 - size/2 && 
                                z <= platform.z + platform.depth/2 + size/2;
            float platformTop = platform.height + platform.heightdelta;
            float platformBottom = platform.heightdelta;
            
            if (isWithinXBounds && isWithinZBounds) {
                if (prevY - size/2 >= platformTop && y - size/2 <= platformTop) {
                    y = platformTop + size/2;
                    velocityY = -velocityY * restitution;
                    velocityX *= (1.0f - friction * deltaTime);
                    velocityZ *= (1.0f - friction * deltaTime);
                    if (prevY - size/2 >= platformTop && y - size/2 <= platformTop) {
                        velocityY=0;
                        y=platformTop + size/2 + 0.01;
                    }

                    if (abs(velocityY) > 1.0f) {
                        angularVelocityX += (rand() % 100 - 50) / 50.0f;
                        angularVelocityZ += (rand() % 100 - 50) / 50.0f;
                    }
                }
                else if (prevY + size/2 <= platformBottom && y + size/2 > platformBottom) {
                    y = platformBottom - size/2;
                    velocityY = -velocityY * restitution;
                }
                else if (y + size/2 > platformBottom && y - size/2 < platformTop) {
                    if (prevX + size/2 <= platform.x - platform.width/2 || 
                        prevX - size/2 >= platform.x + platform.width/2) {
                        x = prevX;
                        velocityX = -velocityX * restitution;
                    }
                    if (prevZ + size/2 <= platform.z - platform.depth/2 || 
                        prevZ - size/2 >= platform.z + platform.depth/2) {
                        z = prevZ;
                        velocityZ = -velocityZ * restitution;
                    }
                }
            }

        }


        if (y - size/2 <= groundY) {
            y = groundY + size/2;
            if (velocityY < 0) {
                velocityY = -velocityY * restitution;
                if (abs(velocityY) > 1.0f) {
                    angularVelocityX += (rand() % 100 - 50) / 50.0f;
                    angularVelocityZ += (rand() % 100 - 50) / 50.0f;
                }
            }
            velocityX *= (1.0f - friction * deltaTime);
            velocityZ *= (1.0f - friction * deltaTime);
            
            if (abs(velocityX) < VELOCITY_THRESHOLD) velocityX = 0.0f;
            if (abs(velocityZ) < VELOCITY_THRESHOLD) velocityZ = 0.0f;
        }

        rotationX += angularVelocityX * deltaTime;
        rotationY += angularVelocityY * deltaTime;
        rotationZ += angularVelocityZ * deltaTime;

        angularVelocityX *= ANGULAR_DAMPING;
        angularVelocityY *= ANGULAR_DAMPING;
        angularVelocityZ *= ANGULAR_DAMPING;

        while (rotationX > 360.0f) rotationX -= 360.0f;
        while (rotationX < -360.0f) rotationX += 360.0f;
        while (rotationY > 360.0f) rotationY -= 360.0f;
        while (rotationY < -360.0f) rotationY += 360.0f;
        while (rotationZ > 360.0f) rotationZ -= 360.0f;
        while (rotationZ < -360.0f) rotationZ += 360.0f;
    }

    
    bool checkPlatformCollision(const Platform& platform) {
        bool withinX = x + size/2 >= platform.x - platform.width/2 &&
                    x - size/2 <= platform.x + platform.width/2;
        bool withinZ = z + size/2 >= platform.z - platform.depth/2 &&
                    z - size/2 <= platform.z + platform.depth/2;
        bool withinY = y + size/2 >= platform.heightdelta &&
                    y - size/2 <= platform.height + platform.heightdelta;
        
        return withinX && withinZ && withinY;
    }

    
    void handlePlatformCollision(const Platform& platform) {
        float penetrationY = (platform.height + platform.heightdelta) - (y - size/2);
        if (penetrationY > 0 && velocityY < 0) {
            y += penetrationY;
            velocityY = -velocityY * restitution;
        }
    }
    
    void applyForce(float forceX, float forceZ) {
        const float FORCE_MULTIPLIER = 2.0f;  
        velocityX += (forceX * FORCE_MULTIPLIER) / mass;
        velocityZ += (forceZ * FORCE_MULTIPLIER) / mass;
    }

    
    void draw() {
        glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef(rotationX, 1.0f, 0.0f, 0.0f);
        glRotatef(rotationY, 0.0f, 1.0f, 0.0f);
        glRotatef(rotationZ, 0.0f, 0.0f, 1.0f);
        drawCube(size, textureID);
        glPopMatrix();
    }
};

std::vector<PhysicsObject> physicsObjects;


void checkPlayerPhysicsObjectCollision() {
    float playerRadius = 0.7f;
    float playerHeight = 2.2f; 
    float stepHeight = 0.5f;  
    
    for (auto& obj : physicsObjects) {
        float dx = obj.x - camX; 
        float dz = obj.z - camZ;
        float dy = (obj.y + obj.size/2) - (camY - playerHeight/2);
        float horizontalDistance = sqrt(dx*dx + dz*dz);

        if (horizontalDistance < (playerRadius + obj.size/2)) {
            float length = sqrt(dx*dx + dz*dz);
            if (length > 0) {
                dx /= length;
                dz /= length;
            }

            if (dy > 0 && dy < stepHeight) {
                camY = obj.y + obj.size/2 + playerHeight/2;
            }
            else if (dy < 0 && dy > -playerHeight) {
                float overlap = (playerRadius + obj.size/2) - horizontalDistance;
                float pushStrength = 5.0f * overlap;
                
                obj.applyForce(dx * pushStrength, dz * pushStrength);
                
                float playerPushback = 0.06f;
                camX -= dx * playerPushback;
                camZ -= dz * playerPushback;
                
                obj.angularVelocityY += (rand() % 100 - 50) / 10.0f;
            }

            if (abs(dy) < 0.1f && velocityY <= 0) {
                obj.velocityY -= 9.81f * 0.016f;
                velocityY = 0;
            }
        }
    }
}

void checkObjectCollision(PhysicsObject& obj1, PhysicsObject& obj2) {
    float dx = obj2.x - obj1.x;
    float dy = obj2.y - obj1.y;
    float dz = obj2.z - obj1.z;
    float distance = sqrt(dx*dx + dy*dy + dz*dz);

    float minDist = (obj1.size + obj2.size) / 2.0f;
    if (distance < minDist) {
        if (distance > 0) {
            dx /= distance;
            dy /= distance;
            dz /= distance;
        }

        float overlap = minDist - distance;

        float relativeVelX = obj1.velocityX - obj2.velocityX;
        float relativeVelY = obj1.velocityY - obj2.velocityY;
        float relativeVelZ = obj1.velocityZ - obj2.velocityZ;

        float impulse = -(1.0f + obj1.restitution * obj2.restitution) * 
                      (relativeVelX * dx + relativeVelY * dy + relativeVelZ * dz) /
                      (1.0f/obj1.mass + 1.0f/obj2.mass);

        obj1.velocityX += (impulse / obj1.mass) * dx;
        obj1.velocityY += (impulse / obj1.mass) * dy;
        obj1.velocityZ += (impulse / obj1.mass) * dz;

        obj2.velocityX -= (impulse / obj2.mass) * dx;
        obj2.velocityY -= (impulse / obj2.mass) * dy;
        obj2.velocityZ -= (impulse / obj2.mass) * dz;

        float moveAmount1 = overlap * (obj2.mass / (obj1.mass + obj2.mass));
        float moveAmount2 = overlap * (obj1.mass / (obj1.mass + obj2.mass));

        obj1.x -= dx * moveAmount1;
        obj1.y -= dy * moveAmount1;
        obj1.z -= dz * moveAmount1;

        obj2.x += dx * moveAmount2;
        obj2.y += dy * moveAmount2;
        obj2.z += dz * moveAmount2;

        float spinForce = overlap * 10.0f;
        obj1.angularVelocityY += (rand() % 100 - 50) / 50.0f * spinForce;
        obj2.angularVelocityY += (rand() % 100 - 50) / 50.0f * spinForce;
    }
}

void updatePhysicsObjects(float deltaTime) {
    const int objectCount = physicsObjects.size();
    
    #pragma omp parallel for
    for (int i = 0; i < objectCount; i++) {
        physicsObjects[i].update(deltaTime);
    }
    
    for (int i = 0; i < objectCount; i++) {
        for (int j = i + 1; j < objectCount; j++) {
            const float dx = std::abs(physicsObjects[i].x - physicsObjects[j].x);
            const float dy = std::abs(physicsObjects[i].y - physicsObjects[j].y);
            const float dz = std::abs(physicsObjects[i].z - physicsObjects[j].z);
            const float maxDist = physicsObjects[i].size + physicsObjects[j].size;
            
            if (dx < maxDist && dy < maxDist && dz < maxDist) {
                checkObjectCollision(physicsObjects[i], physicsObjects[j]);
            }
        }
    }
    
    checkPlayerPhysicsObjectCollision();
}



int IsObjectInfront() {
    float playerRadius = 5.0f;
    float playerHeight = 2.2f;
    
    for (size_t i = 0; i < physicsObjects.size(); i++) {
        const auto& obj = physicsObjects[i];
        float dx = obj.x - camX;
        float dz = obj.z - camZ;
        float dy = (obj.y + obj.size/2) - (camY - playerHeight/2);
        float horizontalDistance = sqrt(dx*dx + dz*dz);

        float radYaw = camYaw * 3.14159f / 180.0f;
        float angle = atan2(dx, -dz) - radYaw;
        while (angle > M_PI) angle -= 2 * M_PI;
        while (angle < -M_PI) angle += 2 * M_PI;

        if (horizontalDistance < (playerRadius + obj.size/2) &&
            fabs(angle) < (M_PI / 6.0f)) {
            return i;
        }
    }
    return -1;
}

bool Objectfrustum(const PhysicsObject& obj) {
    float playerRadius = 100.0f;
    float playerHeight = 2.2f;
    float FOV_RADIANS = M_PI / 4.8f; 
    
    float dx = obj.x - camX;
    float dz = obj.z - camZ;
    float dy = (obj.y + obj.size/2) - (camY - playerHeight/2);
    float horizontalDistance = sqrt(dx*dx + dz*dz);

    float radYaw = camYaw * 3.14159f / 180.0f;
    float angle = atan2(dx, -dz) - radYaw;
    while (angle > M_PI) angle -= 2 * M_PI;
    while (angle < -M_PI) angle += 2 * M_PI;

    if (horizontalDistance < (playerRadius + obj.size/2) &&
        fabs(angle) < FOV_RADIANS) {
        return true;
    }

    return false;
}

bool isInFrustum(const Platform& platform, const Frustum& frustum) {
    static constexpr float PI = 3.14159265359f;
    static constexpr float DEG_TO_RAD = PI / 180.0f;
    static constexpr float BUFFER_ANGLE = 20.0f * DEG_TO_RAD;

    const float radYaw = camYaw * DEG_TO_RAD;
    const float totalAngle = frustum.fov * 0.5f + BUFFER_ANGLE;
    
    const float centerX = platform.x;
    const float centerZ = platform.z;
    const float halfWidth = platform.width * 0.5f;
    const float halfDepth = platform.depth * 0.5f;
    
    const float dx = centerX - camX;
    const float dz = centerZ - camZ;
    
    const float distSquared = dx * dx + dz * dz;
    if (distSquared < 0.0001f) return false; 
    
    const float centerAngle = atan2(dx, -dz) - radYaw;
    
    const float maxRadius = std::sqrt(halfWidth * halfWidth + halfDepth * halfDepth);
    const float maxAngularDeviation = std::asin(maxRadius / std::sqrt(distSquared));
    
    float normalizedAngle = centerAngle;
    while (normalizedAngle > PI) normalizedAngle -= 2 * PI;
    while (normalizedAngle < -PI) normalizedAngle += 2 * PI;
    
    const float minPossibleAngle = normalizedAngle - maxAngularDeviation;
    const float maxPossibleAngle = normalizedAngle + maxAngularDeviation;
    return (maxPossibleAngle < -totalAngle || minPossibleAngle > totalAngle);
}


void loadmap(const std::string& mapName) {
    for (auto& platform : platforms) {
        if (platform.textureID != 0) {
            glDeleteTextures(1, &platform.textureID);
        }
    }
    platforms.clear();
    physicsObjects.clear();

    std::string mapPath = "assets/maps/" + mapName + ".json";
    std::ifstream file(mapPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open map file: " << mapPath << std::endl;
        return;
    }

    try {
        json mapData = json::parse(file);

        for (const auto& platformData : mapData["platforms"]) {
            Platform platform;
            platform.x = platformData["x"];
            platform.z = platformData["z"];
            platform.width = platformData["width"];
            platform.depth = platformData["depth"];
            platform.height = platformData["height"];
            platform.heightdelta = platformData["heightdelta"];
            platform.changemap = platformData["changemap"];
            platform.texturePath = platformData["texture"];
            platform.textureID = 0;
            platforms.push_back(platform);
        }
        for (const auto& physicsobjectData : mapData["physicsobjects"]) {
            PhysicsObject physicsobject;
            physicsobject.x = physicsobjectData["x"];
            physicsobject.y = physicsobjectData["y"];
            physicsobject.z = physicsobjectData["z"];
            physicsobject.size = physicsobjectData["size"];
            physicsobject.texturePath = physicsobjectData["texture"];
            physicsObjects.push_back(physicsobject);
        }
    } catch (json::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        return;
    }
    camX = 0.0f, camY = 2.2f, camZ = 5.0f; 
    playerY = 0.0f;
    isHoldingObject=false;
    heldObjectIndex=-1;
    loadLevelTextures();
}

void loadsettings() {
    lastmap = "empty"; // Add default map name
    std::string settingspath = "assets/settings.json";
    std::ifstream file(settingspath);
    if (!file.is_open()) {
        std::cerr << "Failed to open settings file: " << settingspath << std::endl;
        return;
    }

    try {
        json settingsData = json::parse(file);
        Vsync = settingsData["vsync"];
        FPScount = settingsData["fpscount"];
        wireframs = settingsData["wireframs"];
        if (!settingsData["lastmap"].empty()) {
            lastmap = settingsData["lastmap"];
        }
        
    } catch (json::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
    }
}



void loadLevelTextures() {
    for (auto& platform : platforms) {
        platform.textureID = loadTexture(platform.texturePath.c_str());
    }
    for (auto& physicsobject : physicsObjects) {
        physicsobject.textureID = loadTexture(physicsobject.texturePath.c_str());
    }
}

void drawCube(float size, GLuint textureID) {
    float halfSize = size / 2.0f;
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);  
    glCullFace(GL_BACK);    
    glFrontFace(GL_CCW);    
    glBindTexture(GL_TEXTURE_2D, textureID);
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
    static Frustum frustum;
    static float lastUpdateTime = 0.0f;
    const float updateInterval = 0.1f; 
    
    float currentTime = glfwGetTime();
    if (currentTime - lastUpdateTime > updateInterval) {
        int width, height;
        glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);
        frustum.update(45.0f, (float)width/height, 0.1f, 100.0f);
        lastUpdateTime = currentTime;
    }
    
    std::vector<const Platform*> visiblePlatforms;
    visiblePlatforms.reserve(platforms.size());
    
    for (const auto& platform : platforms) {
        if (!isInFrustum(platform, frustum)) {
            visiblePlatforms.push_back(&platform);
        }
    }

    std::sort(visiblePlatforms.begin(), visiblePlatforms.end(),
        [](const Platform* a, const Platform* b) {
            return a->textureID < b->textureID;
        });
    
    GLuint currentTexture = 0;
    for (const Platform* platform : visiblePlatforms) {
        if (platform->textureID != currentTexture) {
            currentTexture = platform->textureID;
            glBindTexture(GL_TEXTURE_2D, currentTexture);
        }
        
        glPushMatrix();
        glTranslatef(platform->x, (platform->height / 2.0f) + platform->heightdelta, platform->z);
        glScalef(platform->width, platform->height, platform->depth);
        drawCube(1.0f, currentTexture);
        glPopMatrix();
    }
}

void drawPhysicsObjects() {
    float distance;
    for (auto& obj : physicsObjects) {
        if (Objectfrustum(obj)) {
            obj.draw();
        }
    }
}


void drawPlatformWireframes() {
    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0f, 0.0f, 0.0f); 
    glLineWidth(2.0f);  

    for (const auto& platform : platforms) {
        float halfWidth = (platform.width / 2.0f) + collisionThreshold;
        float halfDepth = (platform.depth / 2.0f) + collisionThreshold;
        float halfHeight = platform.height / 2.0f;  

        glPushMatrix();
        glTranslatef(platform.x, platform.heightdelta + halfHeight, platform.z);

        glBegin(GL_LINE_LOOP);
        glVertex3f(-halfWidth, -halfHeight - collisionThreshold, -halfDepth);
        glVertex3f(halfWidth, -halfHeight - collisionThreshold, -halfDepth);
        glVertex3f(halfWidth, -halfHeight - collisionThreshold, halfDepth);
        glVertex3f(-halfWidth, -halfHeight - collisionThreshold, halfDepth);
        glEnd();

        glBegin(GL_LINE_LOOP);
        glVertex3f(-halfWidth, halfHeight, -halfDepth);
        glVertex3f(halfWidth, halfHeight, -halfDepth);
        glVertex3f(halfWidth, halfHeight, halfDepth);
        glVertex3f(-halfWidth, halfHeight, halfDepth);
        glEnd();

        glBegin(GL_LINES);
        glVertex3f(-halfWidth, -halfHeight - collisionThreshold, -halfDepth);
        glVertex3f(-halfWidth, halfHeight, -halfDepth);
        glVertex3f(halfWidth, -halfHeight - collisionThreshold, -halfDepth);
        glVertex3f(halfWidth, halfHeight, -halfDepth);
        glVertex3f(halfWidth, -halfHeight - collisionThreshold, halfDepth);
        glVertex3f(halfWidth, halfHeight, halfDepth);
        glVertex3f(-halfWidth, -halfHeight - collisionThreshold, halfDepth);
        glVertex3f(-halfWidth, halfHeight, halfDepth);
        glEnd();

        glPopMatrix();
    }


    glColor3f(0.0f, 0.5f, 0.5f); 
    for (const auto& platform : platforms) {
        float halfWidth = (platform.width / 2.0f)+0.001f;
        float halfDepth = (platform.depth / 2.0f)+0.001f;
        float halfHeight = (platform.height / 2.0f)+0.001f;  

        glPushMatrix();
        glTranslatef(platform.x, platform.heightdelta + halfHeight, platform.z);

        glBegin(GL_LINE_LOOP);
        glVertex3f(-halfWidth, -halfHeight, -halfDepth);
        glVertex3f(halfWidth, -halfHeight, -halfDepth);
        glVertex3f(halfWidth, -halfHeight, halfDepth);
        glVertex3f(-halfWidth, -halfHeight, halfDepth);
        glEnd();

        glBegin(GL_LINE_LOOP);
        glVertex3f(-halfWidth, halfHeight, -halfDepth);
        glVertex3f(halfWidth, halfHeight, -halfDepth);
        glVertex3f(halfWidth, halfHeight, halfDepth);
        glVertex3f(-halfWidth, halfHeight, halfDepth);
        glEnd();

        glBegin(GL_LINES);
        glVertex3f(-halfWidth, -halfHeight, -halfDepth);
        glVertex3f(-halfWidth, halfHeight, -halfDepth);
        glVertex3f(halfWidth, -halfHeight, -halfDepth);
        glVertex3f(halfWidth, halfHeight, -halfDepth);
        glVertex3f(halfWidth, -halfHeight, halfDepth);
        glVertex3f(halfWidth, halfHeight, halfDepth);
        glVertex3f(-halfWidth, -halfHeight, halfDepth);
        glVertex3f(-halfWidth, halfHeight, halfDepth);
        glEnd();

        glPopMatrix();
    }

    glEnable(GL_TEXTURE_2D);
}

float getPlatformHeight(float x, float z, float currentY) {
    float closestPlatformHeight = groundY;
    for (const auto& platform : platforms) {
        bool isWithinXBounds = x >= platform.x - platform.width / 2 - collisionThreshold && x <= platform.x + platform.width / 2 + collisionThreshold;
        bool isWithinZBounds = z >= platform.z - platform.depth / 2 - collisionThreshold && z <= platform.z + platform.depth / 2 + collisionThreshold;

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
            if ((camY-y)<=platform.height+platform.heightdelta - 0.1f && !(platform.heightdelta-collisionThreshold>(camY-y)) && canwalkbeneeth) {
                lastmap = platform.changemap;
                return true;  
            }
            
            if (!canwalkbeneeth && isBelowPlatform ) {
                lastmap = platform.changemap;
                return true;  
            }
        }
    }
    lastmap = "";
    return false;
}

bool checkJump() {
    float nextplatformheight = std::numeric_limits<float>::max();
    float curentplatform;
    std::string nextplafrom = "";

    for (const auto& platform : platforms) {
        bool isWithinXBounds = camX >= platform.x - platform.width / 2 - collisionThreshold && camX <= platform.x + platform.width / 2 + collisionThreshold;
        bool isWithinZBounds = camZ >= platform.z - platform.depth / 2 - collisionThreshold && camZ <= platform.z + platform.depth / 2 + collisionThreshold;

        if (isWithinXBounds && isWithinZBounds) {
            curentplatform = platform.height + platform.heightdelta;
            if (curentplatform > camY && curentplatform < nextplatformheight) {
                nextplatformheight = curentplatform;
            }
        }
    }
    if (nextplatformheight-collisionThreshold <= camY+collisionThreshold) {
        return false;
    }
    return true;
}

void updateHeldObject() {
    if (!isHoldingObject || heldObjectIndex < 0) return;
    
    float radYaw = camYaw * 3.14159f / 180.0f;
    float radPitch = camPitch * 3.14159f / 180.0f;
    
    float targetX = camX + sin(radYaw) * cos(radPitch) * holdDistance;
    float targetY = camY + -sin(radPitch) * holdDistance;
    float targetZ = camZ + -cos(radYaw) * cos(radPitch) * holdDistance;
    
    PhysicsObject& obj = physicsObjects[heldObjectIndex];
    
    float springStrength = 15.0f;
    float dampingFactor = 0.95f;

    float dx = targetX - obj.x;
    float dy = targetY - obj.y;
    float dz = targetZ - obj.z;
    

    obj.velocityX += dx * springStrength * 0.016f;
    obj.velocityY += dy * springStrength * 0.016f;
    obj.velocityZ += dz * springStrength * 0.016f;

    obj.velocityX *= dampingFactor;
    obj.velocityY *= dampingFactor;
    obj.velocityZ *= dampingFactor;
    
    float maxVelocity = 20.0f;
    float velocityMagnitude = sqrt(obj.velocityX * obj.velocityX + 
                                 obj.velocityY * obj.velocityY + 
                                 obj.velocityZ * obj.velocityZ);
    
    if (velocityMagnitude > maxVelocity) {
        float scale = maxVelocity / velocityMagnitude;
        obj.velocityX *= scale;
        obj.velocityY *= scale;
        obj.velocityZ *= scale;
    }
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
                saveSettings();
            }
        }

        if (key == GLFW_KEY_SPACE && !isJumping && playerY == groundY && !crouch && stamina >= 5 && !nojump && checkJump()) {
            
            isJumping = true;
            velocityY = jumpSpeed;
            stamina -= 5;
        }

        if (key == GLFW_KEY_E && action == GLFW_PRESS) {
            if (!isHoldingObject) {
                int objectIndex = IsObjectInfront();
                if (objectIndex >= 0) {
                    isHoldingObject = true;
                    heldObjectIndex = objectIndex;
                }
            } else {
                isHoldingObject = false;
                heldObjectIndex = -1;
            }
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
        float nextplatformheight = std::numeric_limits<float>::max();
        float curentplatform;
        std::string nextplafrom = "";

        for (const auto& platform : platforms) {
            bool isWithinXBounds = camX >= platform.x - platform.width / 2 - collisionThreshold && camX <= platform.x + platform.width / 2 + collisionThreshold;
            bool isWithinZBounds = camZ >= platform.z - platform.depth / 2 - collisionThreshold && camZ <= platform.z + platform.depth / 2 + collisionThreshold;

            if (isWithinXBounds && isWithinZBounds) {
                curentplatform = platform.height + platform.heightdelta;
                if (curentplatform > camY && curentplatform < nextplatformheight) {
                    nextplatformheight = curentplatform;
                    nextplafrom = std::to_string(platform.x) + "," + std::to_string(platform.z);
                }
            }
        }

        for (const auto& platform : platforms) {
            if (camX >= platform.x - platform.width / 2 - collisionThreshold && camX <= platform.x + platform.width / 2 + collisionThreshold &&
                camZ >= platform.z - platform.depth / 2 - collisionThreshold && camZ <= platform.z + platform.depth / 2 + collisionThreshold &&
                camY + 1.0f > platform.heightdelta-(platform.height/2) && platform.heightdelta-(platform.height/2) > 0.0f && nextplafrom == (std::to_string(platform.x) + "," + std::to_string(platform.z))) {
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
    float platformHeight;

    float checkplatformHeight = getPlatformHeight(nextCamX, nextCamZ, playerY+0.5f);
    if (isNearPlatform(nextCamX, nextCamZ, playerY, camY) && !(checkplatformHeight - playerY <= 0.5f && checkplatformHeight - playerY > 0.0f)) {
        moveX = 0.0f;
        moveZ = 0.0f;
        velocityY=0.0f;
        nojump = true;
        if (lastmap != "") {
            currentmap=lastmap;
            saveSettings();
            loadmap(lastmap);
            lastmap = "";
        }
    } else if (checkplatformHeight - playerY <= 0.5f && checkplatformHeight - playerY > 0.0f) {
        playerY = checkplatformHeight;
        camX += moveX * moveSpeed;
        camZ += moveZ * moveSpeed;
        nojump = false;
    } else {
        nojump = false;
    }

    if (!(checkplatformHeight - playerY <= 0.5f && checkplatformHeight - playerY > 0.0f)) {
        camX += moveX * moveSpeed;
        camZ += moveZ * moveSpeed;
        platformHeight = getPlatformHeight(camX, camZ, playerY);
    }

    


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

            if (keys[GLFW_KEY_SPACE]&& !crouch && stamina >= 5 && !nojump && checkJump()) {
                isJumping = true;
                velocityY = jumpSpeed; 
                stamina -= 5;
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
        float scaleFactor = std::min(static_cast<float>(width) / 1920.0f, static_cast<float>(height) / 1080.0f);

        float buttonWidth = 300.0f * scaleFactor;
        float buttonHeight = 100.0f * scaleFactor;
        float buttonSpacing = 50.0f * scaleFactor;
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
                    saveSettings();
                } else if (menueData[i].text == "Toggle FPS") {
                    FPScount = !FPScount;
                } else if (menueData[i].text == "Toggle VSync") {
                    Vsync = !Vsync;  
                    glfwSwapInterval(Vsync);  
                } else if (menueData[i].text == "Fullscreen") {
                    toggleFullscreen(window);
                } else if (menueData[i].text == "Wireframs") {
                    wireframs = !wireframs;
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
        int width, height;
        glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);
        float scaleFactor = std::min(static_cast<float>(width) / 640.0f, static_cast<float>(height) / 480.0f);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, width, height, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        if (FPScount) {
            std::string fpsText = "FPS: " + std::to_string(static_cast<int>(frameCount / (glfwGetTime() - lastFPSUpdate)));
            renderText(fpsText, 650.0f * scaleFactor, 10.0f * scaleFactor, 15.0f * scaleFactor, fontTexture); 
        }
        drawStaminaBar(10.0f * scaleFactor, 10.0f  * scaleFactor, 100.0f * scaleFactor, 10.0f * scaleFactor, stamina); 

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }
};


class MENU {
public:
    static void drawRectangle(float x, float y, float width, float height, float r, float g, float b) {
        glColor3f(r, g, b);
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
        float scaleFactor = std::min(static_cast<float>(width) / 1920.0f, static_cast<float>(height) / 1080.0f);

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
        float buttonWidth = 400.0f * scaleFactor;
        float buttonHeight = 100.0f * scaleFactor;
        float buttonSpacing = 50.0f * scaleFactor;
        float totalHeight = numButtons * buttonHeight + (numButtons - 1) * buttonSpacing;
        float startY = (height - totalHeight) / 2.0f;

        if (charIndices.empty()) {
            charIndices.resize(numButtons, 0);
        }

        for (int i = 0; i < numButtons; ++i) {
            float buttonX = (width - buttonWidth) / 2.0f;
            float buttonY = startY + i * (buttonHeight + buttonSpacing);

            if (menueData[i].text == "Toggle FPS") {
                drawRectangle(buttonX, buttonY, buttonWidth, buttonHeight, 
                            FPScount ? 0.2f : 1.0f,   
                            FPScount ? 1.0f : 0.2f,    
                            FPScount ? 0.2f : 0.2f);  
            }
            else if (menueData[i].text == "Toggle VSync") {
                drawRectangle(buttonX, buttonY, buttonWidth, buttonHeight, 
                            Vsync ? 0.2f : 1.0f,
                            Vsync ? 1.0f : 0.2f,
                            Vsync ? 0.2f : 0.2f);
            }
            else if (menueData[i].text == "Wireframs") {
                drawRectangle(buttonX, buttonY, buttonWidth, buttonHeight, 
                            wireframs ? 0.2f : 1.0f,
                            wireframs ? 1.0f : 0.2f,
                            wireframs ? 0.2f : 0.2f);
            }
            else if (menueData[i].text == "Fullscreen") {
                drawRectangle(buttonX, buttonY, buttonWidth, buttonHeight, 
                            isFullscreen ? 0.2f : 1.0f,
                            isFullscreen ? 1.0f : 0.2f,
                            isFullscreen ? 0.2f : 0.2f);
            }
            else if (menueData[i].text == "Resume") {
                drawRectangle(buttonX, buttonY, buttonWidth, buttonHeight, 
                            0.2f, 0.6f, 1.0f);
            }
            else if (menueData[i].text == "Quit") {
                drawRectangle(buttonX, buttonY, buttonWidth, buttonHeight, 
                            1.0f, 0.3f, 0.3f);
            }
            else {
                drawRectangle(buttonX, buttonY, buttonWidth, buttonHeight, 
                            0.35f, 0.35f, 0.4f);
            }

            float textwidth = getTextWidth(menueData[i].text, 55.0f * scaleFactor);
            float textX = buttonX + (buttonWidth - textwidth) / 2.0f;  
            float textY = buttonY + (buttonHeight + 55.0f * scaleFactor) / 2.0f;

            if (i < currentButton) {
                renderText(menueData[i].text, textX, textY, 55.0f * scaleFactor, fontTexture);
            } else if (i == currentButton) {
                std::string displayText = menueData[i].text.substr(0, charIndices[i]);
                if (charIndices[i] < menueData[i].text.length()) {
                    displayText += "_";
                }
                renderText(displayText, textX, textY, 55.0f * scaleFactor, fontTexture);
                
                static float lastPrintTime = 0.0f;
                float currentTime = glfwGetTime();
                if (currentTime - lastPrintTime >= 0.03f) {
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
            updatePhysicsObjects(fixedTimeStep);
            accumulator -= fixedTimeStep;  
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        glLoadIdentity();
        glRotatef(camPitch, 1.0f, 0.0f, 0.0f);
        glRotatef(camYaw, 0.0f, 1.0f, 0.0f);
        glTranslatef(-camX, -camY, -camZ);
        if (wireframs) {
            drawPlatformWireframes();
        }
        drawGrids(GRID_WIDTH, 1.0f);
        drawPlatforms();
        updateHeldObject();
        drawPhysicsObjects();
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
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    if (!primaryMonitor) {
        glfwTerminate();
        return -1;
    }

    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Fullscreen Window", primaryMonitor, NULL);

    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }
    loadFont("assets/fonts/font.png");
    if (fontTexture == 0) {
        std::cerr << "Font texture failed to load" << std::endl;
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

    loadsettings();
    currentmap=lastmap;
    loadmap(lastmap);
    
    while (!glfwWindowShouldClose(window)) {

        display(window);

        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}