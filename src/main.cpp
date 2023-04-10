#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>
#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadCubemap(std::vector<std::string> faces);
void drawModel(Model obj_model, Shader shader, const std::vector<glm::vec3>& translations, glm::vec3 rotation, glm::vec3 scale, glm::mat4 projection, glm::mat4 view);
void loadFaces(std::vector<std::string> &faces, const std::string& dirName);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
unsigned int loadTexture(char const * path);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(.1f, .1f, .1f);
    bool ImGui1Enable = false;
    bool ImGui2Enable = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    float diamondScale = 2.0f;
    float diamondTransparent = 0.4f;

    Model diamond, pink_diamond, mars, venus, sun;

    ProgramState() : camera(glm::vec3(0.0f, 0.0f, 7.0f)),
                    diamond(FileSystem::getPath("resources/objects/diamond/Diamond.obj")),
                    pink_diamond(FileSystem::getPath("resources/objects/pink_diamond/Diamond.obj")),
                    mars(FileSystem::getPath("resources/objects/mars/planet.obj")),
                    venus(FileSystem::getPath("resources/objects/venus/planet.obj")),
                    sun(FileSystem::getPath("resources/objects/sun/planet.obj")) {}

    std::vector<std::string> faces, inner_faces;
    unsigned int cubemapTexture{}, inner_cubemapTexture{};

    std::string color;

    void SaveToFile(const std::string filename);
    void LoadFromFile(const std::string filename);
};

void ProgramState::SaveToFile(const std::string filename) {
    std::ofstream out(filename);

    out << clearColor.r << "\n"
        << clearColor.g << "\n"
        << clearColor.b << "\n"
        << ImGui1Enable << "\n"
        << ImGui2Enable << "\n"
        << camera.Position.x << "\n"
        << camera.Position.y << "\n"
        << camera.Position.z << "\n"
        << camera.Front.x << "\n"
        << camera.Front.y << "\n"
        << camera.Front.z << "\n"
        << camera.Pitch << "\n"
        << camera.Yaw << "\n"
        << diamondScale << "\n"
        << diamondTransparent;
}

void ProgramState::LoadFromFile(const std::string filename) {
    std::ifstream in(filename);

    if(in) {
        in  >> clearColor.r
            >> clearColor.g
            >> clearColor.b
            >> ImGui1Enable
            >> ImGui2Enable
            >> camera.Position.x
            >> camera.Position.y
            >> camera.Position.z
            >> camera.Front.x
            >> camera.Front.y
            >> camera.Front.z
            >> camera.Pitch
            >> camera.Yaw
            >> diamondScale
            >> diamondTransparent;
    }
}

struct ProgramShader {

    Shader cube, skybox, diamond, window, planet;

    ProgramShader() : cube("resources/shaders/cube/cube.vs", "resources/shaders/cube/cube.fs"),
                    skybox("resources/shaders/skybox/skybox.vs", "resources/shaders/skybox/skybox.fs"),
                    diamond("resources/shaders/diamond/diamond.vs", "resources/shaders/diamond/diamond.fs"),
                    window("resources/shaders/window/transparent.vs", "resources/shaders/window/transparent.fs"),
                    planet("resources/shaders/planet/planet.vs", "resources/shaders/planet/planet.fs") {}
};

ProgramState *programState;
ProgramShader *shader;

void drawImGui(ProgramState *programState);

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", nullptr, nullptr);

    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        return -1;
    }

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");

    shader = new ProgramShader;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (programState->ImGui1Enable) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    // ImGui Init
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float cube_vertices[] = {
            -0.5f, -0.5, -0.5,  .0f, 0.0f, -1.0f,
            0.5f, -0.5, -0.5,  .0f, 0.0f, -1.0f,
            0.5f,  0.5, -0.5,  .0f, .0f, -1.0f,
            0.5f,  0.5, -0.5,  .0f, .0f, -1.0f,
            -0.5f,  0.5, -0.5,  0.0f, .0f, -1.0f,
            -0.5f, -0.5, -0.5,  0.0f, 0.0f, -1.0f,

            -0.5, -0.5,  0.5,  0.0f, 0.0f, 1.0f,
            0.5, -0.5,  0.5,  .0f, 0.0f, 1.0f,
            0.5,  0.5,  0.5,  .0f, .0f, 1.0f,
            0.5,  0.5,  0.5,  .0f, .0f, 1.0f,
            -0.5,  0.5,  0.5,  0.0f, .0f,1.0f,
            -0.5, -0.5,  0.5,  0.0f, 0.0f,1.0f,

            -0.5,  0.5,  0.5,  -1.0f, 0.0f,0.0f,
            -0.5,  0.5, -0.5,  -1.0f, .0f,0.0f,
            -0.5, -0.5, -0.5,  -1.0f, .0f,0.0f,
            -0.5, -0.5, -0.5,  -1.0f, .0f,0.0f,
            -0.5, -0.5,  0.5,  -1.0f, 0.0f,0.0f,
            -0.5,  0.5,  0.5,  -1.0f, 0.0f,0.0f,

            0.5,  0.5,  0.5,  1.0f, 0.0f,0.0f,
            0.5,  0.5, -0.5,  1.0f, .0f,0.0f,
            0.5, -0.5, -0.5,  1.0f, .0f,0.0f,
            0.5, -0.5, -0.5,  1.0f, .0f,0.0f,
            0.5, -0.5,  0.5,  1.0f, 0.0f,0.0f,
            0.5,  0.5,  0.5,  1.0f, 0.0f,0.0f,

            -0.5, -0.5, -0.5,  0.0f, -1.0f,0.0f,
            0.5, -0.5, -0.5,  .0f, -1.0f,0.0f,
            0.5, -0.5,  0.5,  .0f, -1.0f,0.0f,
            0.5, -0.5,  0.5,  .0f, -1.0f,0.0f,
            -0.5, -0.5,  0.5,  0.0f, -1.0f,0.0f,
            -0.5, -0.5, -0.5,  0.0f, -1.0f,0.0f,

            -0.5,  0.5, -0.5,  0.0f, 1.0f,0.0f,
            0.5,  0.5, -0.5,  .0f, 1.0f,0.0f,
            0.5,  0.5,  0.5,  .0f, 1.0f,0.0f,
            0.5,  0.5,  0.5,  .0f, 1.0f,0.0f,
            -0.5,  0.5,  0.5,  0.0f, 1.0f,0.0f,
            -0.5,  0.5, -0.5f,  0.0f, 1.0f, 0.0f
    };

    float skyBox_vertices[] = {
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    float transparentVertices[] = {
            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };


    // VBOs, VAOs

    unsigned int cubeVBO, cubeVAO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    unsigned int inner_skyboxVBO, inner_skyboxVAO;
    glGenVertexArrays(1, &inner_skyboxVAO);
    glGenBuffers(1, &inner_skyboxVBO);
    glBindVertexArray(inner_skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, inner_skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyBox_vertices), skyBox_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)nullptr);
    glEnableVertexAttribArray(0);

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyBox_vertices), &skyBox_vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)nullptr);

    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    loadFaces(programState->inner_faces, "skybox");
    loadFaces(programState->faces, "inner_skybox");

    programState->inner_cubemapTexture = loadCubemap(programState->inner_faces);
    programState->cubemapTexture = loadCubemap(programState->faces);

    shader->cube.use();
    shader->cube.setInt("skybox", 0);

    shader->skybox.use();
    shader->skybox.setInt("skybox", 0);

    unsigned int transparentTexture = loadTexture(FileSystem::getPath("resources/textures/window.png").c_str());

    shader->window.use();
    shader->window.setInt("texture1", 0);

    // render loop
    while (!glfwWindowShouldClose(window)) {

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        // render
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f , 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();

        // cube
        glm::mat4 cubeModel = glm::mat4(1.0f);
        cubeModel = glm::scale(cubeModel, glm::vec3(17.0f, 17.0f, 17.0f));

        shader->cube.use();
        shader->cube.setMat4("projection", projection);
        shader->cube.setMat4("view", view);
        shader->cube.setMat4("model", cubeModel);
        shader->cube.setVec3("cameraPos", programState->camera.Position);

        glBindVertexArray(cubeVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, programState->cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        // MODELS
        // diamonds models

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glm::vec3 translation = glm::vec3(0.0f, -1.0f, -2.0f);
        glm::vec3 rotation = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 scale = glm::vec3(programState->diamondScale);

        shader->diamond.use();
        shader->diamond.setFloat("diamondTransparent", programState->diamondTransparent);

        if (programState->color == "clear") {
            drawModel(programState->diamond, shader->diamond, {translation}, rotation, scale, projection, view);
        } else if (programState->color == "pink") {
            drawModel(programState->pink_diamond, shader->diamond, {translation}, rotation, scale, projection, view);
        } else {
            drawModel(programState->diamond, shader->diamond, {translation}, rotation, scale, projection, view);
        }

        glDisable(GL_CULL_FACE);

        // mars model
        double time = glfwGetTime();

        translation = glm::vec3(-2.0f * cos(time), -2.0f * cos(time), -4.5f * sin(time) / 2);
        glm::vec3 translation2 = glm::vec3(0.0f, 0.0f, -2.0f);
        scale = glm::vec3(0.08f, 0.08f, 0.08f);

        drawModel(programState->mars, shader->planet, {translation, translation2}, rotation, scale, projection, view);

        // venus model
        translation = glm::vec3(2.0f * cos(time), -2.0f * cos(time), 4.5f * sin(time) / 2);

        drawModel(programState->venus, shader->planet, {translation, translation2}, rotation, scale, projection, view);

        // sun model
        translation = glm::vec3(2.5f * cos(time), .0f , -4.0f * sin(time) / 2);

        drawModel(programState->sun, shader->planet, {translation, translation2}, rotation, scale, projection, view);

        // transparent window
        shader->window.use();
        shader->window.setMat4("projection", projection);
        shader->window.setMat4("view", view);

        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);
        glm::mat4 windowModel = glm::mat4(1.0f);
        windowModel = glm::translate(windowModel, glm::vec3(-8.45f, 0.0f, 8.6f));
        windowModel = glm::scale(windowModel, glm::vec3(16.9f, 16.9f, 0.0f));
        shader->window.setMat4("model", windowModel);
        glDrawArrays(GL_TRIANGLES, 0, 6);


        // SKY_BOXES
        // main skybox
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);
        shader->skybox.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix()));
        shader->skybox.setMat4("view", view);
        shader->skybox.setMat4("projection", projection);

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, programState->cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);

        // inner skybox
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);
        shader->skybox.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix()));
        shader->skybox.setMat4("projection", projection);
        shader->skybox.setMat4("view", view);

        glBindVertexArray(inner_skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, programState->inner_cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);

        // skybox end

        //ImGui

        if (programState->ImGui1Enable || programState->ImGui2Enable) {
            drawImGui(programState);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteVertexArrays(1, &inner_skyboxVAO);
    glDeleteVertexArrays(1, &transparentVAO);

    glDeleteBuffers(1, &transparentVBO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &skyboxVBO);
    glDeleteBuffers(1, &inner_skyboxVBO);

    programState->SaveToFile("resources/program_state.txt");

    delete programState;
    delete shader;

    // ImGui CleanUp
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        programState->color = "";
        programState->color = "clear";
    }

    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        programState->color = "";
        programState->color = "pink";
    }
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if(!programState->ImGui1Enable && !programState->ImGui2Enable) {
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void drawModel(Model obj_model, Shader m_shader, const std::vector<glm::vec3>& translations, glm::vec3 rotation, glm::vec3 scale, glm::mat4 projection, glm::mat4 view) {
    glm::mat4 model = glm::mat4(1.0f);

    for (auto &translation : translations) {
        model = glm::translate(model, translation);
    }

    model = glm::rotate(model, (float)glfwGetTime(), rotation);
    model = glm::scale(model, scale);

    m_shader.use();
    m_shader.setMat4("projection", projection);
    m_shader.setMat4("view", view);
    m_shader.setMat4("model", model);

    stbi_set_flip_vertically_on_load(true);
    obj_model.Draw(m_shader);
    stbi_set_flip_vertically_on_load(false);
}

void loadFaces(std::vector<std::string> &faces, const std::string& dirName) {
    faces = {
            FileSystem::getPath("resources/textures/" + dirName + "/right.jpg"),
            FileSystem::getPath("resources/textures/" + dirName + "/left.jpg"),
            FileSystem::getPath("resources/textures/" + dirName + "/top.jpg"),
            FileSystem::getPath("resources/textures/" + dirName + "/bottom.jpg"),
            FileSystem::getPath("resources/textures/" + dirName + "/front.jpg"),
            FileSystem::getPath("resources/textures/" + dirName + "/back.jpg")
    };
}

void drawImGui(ProgramState *programState) {
    // ImGui Frame Init
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (programState->ImGui1Enable) {
        {
            ImGui::Begin("Diamons's Universe");
            ImGui::Text(
                    "Dobro dosli na moj projekat iz predmeta Racunarska grafika!\nOvaj projekat ilustruje zalazak sunca u kanjonu sa portalom koji vodi u drugu dimenziju.\n"
                    "Kad udjete u drugu dimenziju, videcete dijamant, koji moze da menja boje.\n"
                    "Pritiskom na dugme P dijaman ce postati roze, pritiskom na dugme C vratice se u prvobitnu, default, boju.\n"
                    "Ukoliko zelite da iskucite ovaj prozor pritisnite F1. \n(Naravno, ukoliko zelite ponovo da ga ukljucite isto pritisnite F1)\n"
                    "\nUkoliko zelite da promenite neke karakteristike dijamanta kliknite F2.\n");
            ImGui::End();
        }
    }

    if (programState->ImGui2Enable) {
        {
            ImGui::Begin("Settings");
            ImGui::Text("Ovde mozete da promenite velicinu dijamanta.\n\n");
            ImGui::DragFloat("<- Diamond scale", &programState->diamondScale, 0.01f, 0.0, 2.5);
            ImGui::Text("\n\nUkoliko zelite mozete da menjate i transparentnost dijamanta:\n\n");
            ImGui::DragFloat("<- Diamond transparent", &programState->diamondTransparent, 0.005f, 0.0, 1.0);
            ImGui::End();
        }
    }

    // ImGui render
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGui1Enable = !programState->ImGui1Enable;
        if (programState->ImGui1Enable) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    if (key == GLFW_KEY_F2 && action == GLFW_PRESS) {
        programState->ImGui2Enable = !programState->ImGui2Enable;
        if (programState->ImGui2Enable) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}