#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <filesystem>
#include <iostream>
#include <vector>

#include "camera.h"
#include "model.h"
#include "shader.h"

// Window dimensions
static const int SCR_W = 1280;
static const int SCR_H = 720;

// Global camera
Camera g_camera({ 0.f, 5.f, 15.f });

// Mouse tracking
static float g_lastX      = SCR_W / 2.f;
static float g_lastY      = SCR_H / 2.f;
static bool  g_rmb        = false;
static bool  g_firstMouse = true;

// Frame timing
static float g_deltaTime = 0.f;
static float g_lastFrame = 0.f;

// Fog toggle
static bool g_fogEnabled = true;

// ── Callbacks ────────────────────────────────────────────────────────────────

static void framebufferSizeCallback(GLFWwindow*, int w, int h)
{
    glViewport(0, 0, w, h);
}

static void mouseButtonCallback(GLFWwindow*, int button, int action, int)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        g_rmb = (action == GLFW_PRESS);
        if (g_rmb) g_firstMouse = true;
    }
}

static void cursorPosCallback(GLFWwindow*, double xpos, double ypos)
{
    if (!g_rmb) return;

    if (g_firstMouse) {
        g_lastX = (float)xpos;
        g_lastY = (float)ypos;
        g_firstMouse = false;
        return;
    }

    float dx = (float)xpos - g_lastX;
    float dy = g_lastY - (float)ypos; // inverted Y
    g_lastX = (float)xpos;
    g_lastY = (float)ypos;

    g_camera.processMouseMovement(dx, dy);
}

static void scrollCallback(GLFWwindow*, double, double yo)
{
    g_camera.processScroll((float)yo);
}

static void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        g_camera.reset();

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        g_camera.processKeyboard(CameraDir::FORWARD, g_deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        g_camera.processKeyboard(CameraDir::BACKWARD, g_deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        g_camera.processKeyboard(CameraDir::LEFT, g_deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        g_camera.processKeyboard(CameraDir::RIGHT, g_deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        g_camera.processKeyboard(CameraDir::DOWN, g_deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        g_camera.processKeyboard(CameraDir::UP, g_deltaTime);
}

// ── Terrain ──────────────────────────────────────────────────────────────────

static GLuint g_terrainVAO  = 0;
static GLuint g_terrainVBO  = 0;
static GLuint g_terrainEBO  = 0;
static int    g_terrainIndexCount = 0;
static GLuint g_terrainTex  = 0;

static GLuint loadTerrainTexture(const char* path)
{
    int w, h, ch;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &w, &h, &ch, 0);
    if (!data) {
        std::cerr << "[Terrain] Failed to load texture: " << path << "\n";
        // Return a green fallback
        GLuint id;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        unsigned char green[3] = { 80, 140, 50 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, green);
        glGenerateMipmap(GL_TEXTURE_2D);
        return id;
    }

    GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return id;
}

static void buildTerrain()
{
    const float Y    = -0.001f;
    const float HALF = 250.f;
    const float TILE = 100.f;

    float verts[] = {
        -HALF, Y, -HALF,  0.f, 1.f, 0.f,  0.f,  TILE,
         HALF, Y, -HALF,  0.f, 1.f, 0.f,  TILE, TILE,
         HALF, Y,  HALF,  0.f, 1.f, 0.f,  TILE, 0.f,
        -HALF, Y,  HALF,  0.f, 1.f, 0.f,  0.f,  0.f,
    };
    unsigned idx[] = { 0, 1, 2,  2, 3, 0 };
    g_terrainIndexCount = 6;

    glGenVertexArrays(1, &g_terrainVAO);
    glGenBuffers(1, &g_terrainVBO);
    glGenBuffers(1, &g_terrainEBO);
    glBindVertexArray(g_terrainVAO);

    glBindBuffer(GL_ARRAY_BUFFER, g_terrainVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_terrainEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

    const int stride = 8 * sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

    glBindVertexArray(0);

    g_terrainTex = loadTerrainTexture("assets/models/pinetree/Texture/grass.jpg");
}

// ── Lighting ─────────────────────────────────────────────────────────────────


static void setLightUniforms(Shader& sh)
{
    // 1. Dim the sun to make it a true dusk/night scene so lamps are visible
    sh.setVec3("dirLight.direction", -0.5f, -1.0f, -0.5f);
    sh.setVec3("dirLight.ambient",   0.05f, 0.05f, 0.08f); // Very dark ambient
    sh.setVec3("dirLight.diffuse",   0.15f, 0.15f, 0.25f); // Faint moonlight
    sh.setVec3("dirLight.specular",  0.1f,  0.1f,  0.1f);

    // 2. Define a warm, bright yellow/orange color for the lamps
    glm::vec3 lampAmb(0.05f, 0.05f, 0.02f);
    glm::vec3 lampDiff(1.0f, 0.8f, 0.4f);
    glm::vec3 lampSpec(1.0f, 0.9f, 1.0f);

    // 3. Array of lamp positions (matching the X and Z from the scene array)
    // The Y is set to 3.8f to simulate the light emitting from the top bulb area
    // 3. Array of lamp positions (matching the X and Z from the scene array)
    // The Y is set to 3.8f to simulate the light emitting from the top bulb area
    glm::vec3 lightPositions[] = {
        {  14.f, 3.8f,  15.f }, // Matches Lamp 1
        {   2.f, 3.8f,  15.f }, // Matches Lamp 2
        {  14.f, 3.8f,  23.f }, // Matches Lamp 3
        {   2.f, 3.8f,  23.f }  // Matches Lamp 4
    };

    // 4. Send all 4 lights to the shader using a loop
    for (int i = 0; i < 4; ++i) {
        std::string base = "pointLights[" + std::to_string(i) + "].";
        sh.setVec3 (base + "position",  lightPositions[i]);
        sh.setVec3 (base + "ambient",   lampAmb);
        sh.setVec3 (base + "diffuse",   lampDiff);
        sh.setVec3 (base + "specular",  lampSpec);
        
        // Attenuation settings (covers about a distance of 50 units)
        sh.setFloat(base + "constant",  1.0f);
        sh.setFloat(base + "linear",    0.09f);
        sh.setFloat(base + "quadratic", 0.032f);
    }
}

// ── Draw helpers ─────────────────────────────────────────────────────────────

static void drawModel(Model& model, Shader& sh,
                      const glm::mat4& modelMat,
                      const glm::mat4& view,
                      const glm::mat4& proj,
                      float shininess = 32.f)
{
    sh.setMat4("model", modelMat);
    sh.setMat4("view", view);
    sh.setMat4("projection", proj);

    // Normal matrix packed into mat4
    glm::mat3 nm = glm::transpose(glm::inverse(glm::mat3(modelMat)));
    sh.setMat4("normalMatrix", glm::mat4(nm));

    sh.setFloat("material.shininess", shininess);
    model.draw(sh);
}

// Scene object descriptor
struct SceneObject {
    Model*    model;
    glm::vec3 position;
    float     rotationDeg;
    glm::vec3 rotationAxis;
    glm::vec3 scale;
    float     shininess;
};

// ── Main ─────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[])
{
    // Change working directory to where the binary lives so relative paths work
    {
        std::filesystem::path binPath = std::filesystem::canonical(argv[0]);
        std::filesystem::current_path(binPath.parent_path());
    }
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(SCR_W, SCR_H,
                                          "HW3 - 3D Rural Farmstead Scene", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: "
                  << glewGetErrorString(glewErr) << "\n";
        glfwTerminate();
        return -1;
    }
    glGetError();

    std::cout << "OpenGL " << glGetString(GL_VERSION)
              << "  Renderer: " << glGetString(GL_RENDERER) << "\n";

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    // Load shader
    Shader objShader("shaders/object.vert", "shaders/object.frag");

    // Load all models (5+ distinct OBJ files)
    std::cout << "\nLoading models...\n";
    Model mFarmhouse("assets/models/farmhouse/Farm_house.obj");
    Model mBarrel   ("assets/models/barrel/Barrel_OBJ.obj");
    Model mTree     ("assets/models/pinetree/Pine_Tree.obj");
    Model mBench    ("assets/models/bench/wooden_bench.obj");
    Model mLamp     ("assets/models/lamp/objLamp.obj");
    Model mRobot    ("assets/models/robot/Robot.obj");
    Model mFarmer   ("assets/models/farmer/farmer.obj");
    std::cout << "Model loading complete.\n\n";

    // Build terrain
    buildTerrain();

    // Scene objects with varied position, rotation, scale
    // Scene objects with varied position, rotation, scale
    std::vector<SceneObject> scene = {
        // Farmhouse pushed further back to create a front yard
        { &mFarmhouse, {  0.f, 25.f, -15.f },   90.f, {0,1,0}, { 0.5f, 0.5f, 0.5f }, 16.f },

        // Barrel Stack (left side of the house)
        { &mBarrel,    { -8.3f, 0.0f, 7.f },  15.f, {0,1,0}, { 3.0f, 3.0f, 3.0f }, 32.f }, // Base left
        { &mBarrel,    { -5.8f, 0.0f, 7.f },  0.f, {0,1,0}, { 3.0f, 3.0f, 3.0f }, 32.f }, // Base right
        { &mBarrel,    { -6.f, 4.f, 7.f },   90.f, {0,0,1}, { 3.0f, 3.0f, 3.0f }, 32.f }, // Stacked on top (Adjust Y: 1.3f if it floats/clips)

        // Pine trees spread around the outer property
        { &mTree,      { 25.f, -1.f, -20.f },   10.f, {0,1,0}, { 4.0f, 4.0f, 4.0f },  8.f },
        { &mTree,      {-30.f, -1.f, -15.f },   35.f, {0,1,0}, { 3.2f, 3.2f, 3.2f },  8.f },
        { &mTree,      { 25.f, -1.f,  15.f },   85.f, {0,1,0}, { 4.f, 10.f, 4.f },  8.f },
        { &mTree,      {-22.f, -1.f,  18.f },  -20.f, {0,1,0}, { 3.1f, 8.1f, 3.1f },  8.f },
        { &mTree,      { -8.f, -1.f, -30.f },   45.f, {0,1,0}, { 3.0f, 3.0f, 3.0f },  8.f }, // Behind the house

        // Park bench placed under one of the front-right trees, facing the path
        { &mBench,     { 22.f, 0.f,  13.f },  -60.f, {0,1,0}, { .05f, .05f, .05f }, 16.f },

        // Street lamps lining a "pathway" leading to the farmhouse
        { &mLamp,      {  14.f, 0.f,   15.f },    0.f, {0,1,0}, { 1.0f, 1.0f, 1.0f }, 64.f },
        { &mLamp,      { 2.f, 0.f,   15.f },    0.f, {0,1,0}, { 1.0f, 1.0f, 1.0f }, 64.f },
        { &mLamp,      {  14.f, 0.f,  23.f },    0.f, {0,1,0}, { 1.0f, 0.8f, 1.0f }, 64.f },
        { &mLamp,      { 2.f, 0.f,  23.f },    0.f, {0,1,0}, { 1.0f, 0.8f, 1.0f }, 64.f },

        // Farmer standing near the farmhouse entrance
        { &mFarmer,    { -15.f, 7.f, 14.f },  140.f, {0,1,0}, { 5.0f, 5.0f, 5.0f }, 16.f },

        // Robot exploring the yard
        { &mRobot,     { -15.f, 0.f,   10.f },  110.f, {0,1,0}, { 1.3f, 1.3f, 1.3f }, 128.f },
        { &mRobot,     { 1.f, 0.f,   16.f },  110.f, {0,1,0}, { 0.5f, 0.5f, 0.5f }, 128.f }
    };

    // Fog color matches sky
    glm::vec3 skyColor(0.55f, 0.62f, 0.75f);

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        float now   = (float)glfwGetTime();
        g_deltaTime = now - g_lastFrame;
        g_lastFrame = now;

        processInput(window);

        // Toggle fog with F key (debounced)
        static bool fKeyWasDown = false;
        bool fKeyDown = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
        if (fKeyDown && !fKeyWasDown) g_fogEnabled = !g_fogEnabled;
        fKeyWasDown = fKeyDown;

        // Clear with dusk sky color
        glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // View and projection matrices
        glm::mat4 view = g_camera.getViewMatrix();
        glm::mat4 proj = glm::perspective(
            glm::radians(g_camera.fov),
            (float)SCR_W / (float)SCR_H,
            0.1f, 500.f
        );

        objShader.use();
        objShader.setVec3("viewPos", g_camera.position);
        setLightUniforms(objShader);

        // Fog uniforms (bonus)
        objShader.setBool("fogEnabled", g_fogEnabled);
        objShader.setVec3("fogColor", skyColor);
        objShader.setFloat("fogDensity", 0.012f);

        // Draw terrain with grass texture
        {
            glm::mat4 M(1.f);
            objShader.setMat4("model", M);
            objShader.setMat4("view", view);
            objShader.setMat4("projection", proj);
            objShader.setMat4("normalMatrix", glm::mat4(1.f)); // identity for flat ground
            objShader.setFloat("material.shininess", 8.f);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, g_terrainTex);
            objShader.setInt("material.diffuse0", 0);
            objShader.setInt("material.hasDiffuse",  1);
            objShader.setInt("material.hasSpecular", 0);
            objShader.setInt("material.hasEmissive", 0);

            glBindVertexArray(g_terrainVAO);
            glDrawElements(GL_TRIANGLES, g_terrainIndexCount, GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        }

        // Draw all scene objects
        for (auto& obj : scene) {
            glm::mat4 M = glm::translate(glm::mat4(1.f), obj.position);
            M = glm::rotate(M, glm::radians(obj.rotationDeg), obj.rotationAxis);
            M = glm::scale(M, obj.scale);
            drawModel(*obj.model, objShader, M, view, proj, obj.shininess);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &g_terrainVAO);
    glDeleteBuffers(1, &g_terrainVBO);
    glDeleteBuffers(1, &g_terrainEBO);
    glDeleteTextures(1, &g_terrainTex);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
