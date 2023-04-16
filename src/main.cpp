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
void drawSkyBox(Shader objShader, unsigned int objVAO, unsigned int texture);
void drawImGui();
void renderQuad();

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

bool bloom = false;
bool bloomKeyPressed = false;
float exposure = 1.0f;

// camera
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

void setDiamondLightsShader(Shader objShader, PointLight pointLight, DirLight dirLight, SpotLight spotLight);
void setLightsShader(Shader objShader, PointLight pointLight, DirLight dirLight, SpotLight spotLight, glm::vec3 dirLightDiffuse, glm::vec3 dirLightAmbient);

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(.1f, .1f, .1f);
    bool ImGui1Enable = false;
    bool ImGui2Enable = false;
    Camera camera;
    float diamondScale = 2.0f;
    float diamondTransparent = 0.4f;

    bool bling = false;
    bool inSpace = false;

    Model diamond, pink_diamond, mars, venus, sun;

    PointLight pointLight;
    DirLight dirLight;
    SpotLight spotLight, cubeSpotLight;

    ProgramState() : camera(glm::vec3(0.0f, 0.0f, 7.0f)),
                    diamond(FileSystem::getPath("resources/objects/diamond/Diamond.obj")),
                    pink_diamond(FileSystem::getPath("resources/objects/pink_diamond/Diamond.obj")),
                    mars(FileSystem::getPath("resources/objects/mars/planet.obj")),
                    venus(FileSystem::getPath("resources/objects/venus/planet.obj")),
                    sun(FileSystem::getPath("resources/objects/sun/planet.obj")) {}

    std::vector<std::string> faces, inner_faces;
    unsigned int cubemapTexture{}, inner_cubemapTexture{};

    std::string color;

    std::vector<glm::vec3> pointLightsPositions = {
            glm::vec3( 0.7f,  0.2f,  2.0f),
            glm::vec3( 2.3f, -3.3f, -4.0f),
            glm::vec3(-4.0f,  2.0f, -12.0f),
            glm::vec3( 0.0f,  0.0f, -3.0f)
    };

    void SaveToFile(const std::string& filename) const;
    void LoadFromFile(const std::string& filename);
};

void ProgramState::SaveToFile(const std::string& filename) const {
    std::ofstream out(filename);

    out << clearColor.r << "\n"
        << clearColor.g << "\n"
        << clearColor.b << "\n"
        << ImGui1Enable << "\n"
        << ImGui2Enable << "\n"
        << inSpace << "\n"
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

void ProgramState::LoadFromFile(const std::string& filename) {
    std::ifstream in(filename);

    if(in) {
        in  >> clearColor.r
            >> clearColor.g
            >> clearColor.b
            >> ImGui1Enable
            >> ImGui2Enable
            >> inSpace
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

    Shader cube, skybox, diamond, window, planet, hdr, bloom, blur;

    ProgramShader() : cube("resources/shaders/cube/cube.vs", "resources/shaders/cube/cube.fs"),
                    skybox("resources/shaders/skybox/skybox.vs", "resources/shaders/skybox/skybox.fs"),
                    diamond("resources/shaders/diamond/diamond.vs", "resources/shaders/diamond/diamond.fs"),
                    window("resources/shaders/window/transparent.vs", "resources/shaders/window/transparent.fs"),
                    planet("resources/shaders/planet/planet.vs", "resources/shaders/planet/planet.fs"),
                    hdr("resources/shaders/hdr/hdr.vs", "resources/shaders/hdr/hdr.fs"),
                    bloom("resources/shaders/bloom/bloom.vs", "resources/shaders/bloom/bloom.fs"),
                    blur("resources/shaders/blur/blur.vs", "resources/shaders/blur/blur.fs") {}

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

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        return -1;
    }

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");

    shader = new ProgramShader;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (programState->ImGui2Enable) {
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

    ////////////// HDR and BLOOM //////////////

    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }

    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong-framebuffer for blurring
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);

    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }

    /////////////// end HDR and BLOOM  ///////////////

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

    shader->hdr.use();
    shader->hdr.setInt("hdrBuffer", 0);

    shader->blur.use();
    shader->blur.setInt("image", 0);

    shader->bloom.use();
    shader->bloom.setInt("scene", 0);
    shader->bloom.setInt("bloomBlur", 1);

    // hint: rotation -> glm::vec3(angle, axis, nothing)
    std::vector<std::map<std::string, glm::vec3>> windows = {
            {{"translation", glm::vec3(-8.45f, 0.0f, 8.6f)}, {"rotation", glm::vec3(0.0f, 0.0f, 0.0f)}}, // 1st window
            {{"translation", glm::vec3(-8.45f, 0.0f, 8.6f)}, {"rotation", glm::vec3(90.0f, 2.0f, .0f)}}, // 2nd window
            {{"translation", glm::vec3(-8.45f, 0.0f, 8.6f)}, {"rotation", glm::vec3(-90.f, 2.0f, .0f)}}, // 3rd window
            {{"translation", glm::vec3(-8.45f, 0.0f, -8.6f)}, {"rotation", glm::vec3(.0f, 0.0f, .0f)}}, // 4th window
            {{"translation", glm::vec3(-8.45f, .0f, 8.6f)}, {"rotation", glm::vec3(-90.0f, 1.0f, .0f)}}, // 5th window
            {{"translation", glm::vec3(-8.45f, .0f, 8.6f)}, {"rotation", glm::vec3(90.0f, 1.0f, .0f)}} // 6th  window
    };

    programState->diamond.SetShaderTextureNamePrefix("material.");
    programState->mars.SetShaderTextureNamePrefix("material.");
    programState->venus.SetShaderTextureNamePrefix("material.");
    programState->sun.SetShaderTextureNamePrefix("material.");

    // lights

    programState->pointLight.ambient = glm::vec3(0.25, 0.20725, 0.40725);
    programState->pointLight.constant = 1.0f;
    programState->pointLight.linear = 0.09f;
    programState->pointLight.quadratic = 0.032f;

    programState->dirLight.direction = glm::vec3(-.8f, -.5f, -0.3f);
    programState->dirLight.ambient = glm::vec3(0.05f, 0.05f, 0.05f);

    programState->spotLight.position = programState->camera.Position;
    programState->spotLight.direction = programState->camera.Front;
    programState->spotLight.ambient = glm::vec3(.0f);
    programState->spotLight.constant = 1.0f;
    programState->spotLight.linear = 0.09f;
    programState->spotLight.quadratic = 0.032f;
    programState->spotLight.cutOff = glm::cos(glm::radians(12.5f));
    programState->spotLight.outerCutOff = glm::cos(glm::radians(15.0f));


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

        // HDR
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
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

        double a = 17.02 * 0.5;

        if (programState->camera.Position.x >= -a && programState->camera.Position.x <= a
        && programState->camera.Position.y >= -a && programState->camera.Position.y <= a
        && programState->camera.Position.z >= -a && programState->camera.Position.z <= a) {
            drawImGui();
            programState->inSpace = true;
            programState->ImGui1Enable = false;
        } else {
            programState->inSpace = false;
//            programState->ImGui1Enable = true;
        }

        // MODELS
        // diamonds models

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        double time = glfwGetTime();

        glm::vec3 translation = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 rotation = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 scale = glm::vec3(programState->diamondScale);

        // turn on diamond

        if (programState->bling) {
            programState->dirLight.diffuse = glm::vec3(1.05f);
            programState->dirLight.specular = glm::vec3(1.05f);
            programState->spotLight.diffuse = glm::vec3(1.05f);
            programState->spotLight.specular = glm::vec3(1.05f);
            programState->pointLight.diffuse = glm::vec3(1.0, 0.823, 0.829);
            programState->pointLight.specular = glm::vec3(0.296648, 0.296648, 0.296648);
        } else {
            programState->dirLight.diffuse = glm::vec3(0.0f);
            programState->dirLight.specular = glm::vec3(0.0f);
            programState->spotLight.diffuse = glm::vec3(0.0f);
            programState->spotLight.specular = glm::vec3(0.0f);
            programState->pointLight.diffuse = glm::vec3(0.0, 0.0, 0.0);
            programState->pointLight.specular = glm::vec3(0.0, 0.0, 0.0);
        }

        setDiamondLightsShader(shader->diamond, programState->pointLight, programState->dirLight, programState->spotLight);

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
        translation = glm::vec3(-2.f * cos(time), -2.0f * cos(time), -5.f * sin(time) / 2);
        glm::vec3 translation2 = glm::vec3(-0.4f, 1.0f, 0.0f);
        scale = glm::vec3(0.08f, 0.08f, 0.08f);

        setLightsShader(shader->planet, programState->pointLight, programState->dirLight, programState->spotLight, glm::vec3(0.4f, 0.4f, 0.4f), glm::vec3(0.05f, 0.05f, 0.05f));

        drawModel(programState->mars, shader->planet, {translation, translation2}, rotation, scale, projection, view);

        // venus model
        translation = glm::vec3(2.0f * cos(time), -2.0f * cos(time), 5.0f * sin(time) / 2);

        setLightsShader(shader->planet, programState->pointLight, programState->dirLight, programState->spotLight, glm::vec3(2.4f, 0.4f, 0.4f), glm::vec3(0.6f, 0.05f, 0.05f));

        drawModel(programState->venus, shader->planet, {translation, translation2}, rotation, scale, projection, view);

        // sun model
        translation = glm::vec3(0.f, 2.5f * cos(time) , -4.0f * sin(time) / 2);
        translation2 = glm::vec3(0.1f, 0.5f, .0f);

        setLightsShader(shader->planet, programState->pointLight, programState->dirLight, programState->spotLight, glm::vec3(0.4f, 0.4f, 0.4f), glm::vec3(0.05f, 0.05f, 0.05f));

        drawModel(programState->sun, shader->planet, {translation, translation2}, rotation, scale, projection, view);

        // transparent windows

        shader->window.use();
        shader->window.setMat4("projection", projection);
        shader->window.setMat4("view", view);

        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);

        glm::mat4 windowModel;
        float angle;
        float axis;
        glm::vec3 rotateAxis;

        for (auto &m : windows) {
            windowModel = glm::mat4(1.0f);
            for (auto &p: m) {
                if(p.first == "translation") {
                    windowModel = glm::translate(windowModel, p.second);
                } else {
                    angle = p.second.x;
                    axis = p.second.y;

                    if (axis == 1.0) {
                        rotateAxis = glm::vec3(1.0f, .0f, .0f);
                    } else if (axis == 2.0) {
                        rotateAxis = glm::vec3(.0f, 1.0f, .0f);
                    } else {
                        rotateAxis = glm::vec3(.0f, .0f, 1.0f);
                    }

                    windowModel = glm::rotate(windowModel, glm::radians(angle), rotateAxis);
                }
            }

            windowModel = glm::scale(windowModel, glm::vec3(16.9f, 16.9f, 0.0f));
            shader->window.setMat4("model", windowModel);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // SKY_BOXES
        // sunset skybox
        drawSkyBox(shader->skybox, skyboxVAO, programState->cubemapTexture);

        // Universe skybox
        drawSkyBox(shader->skybox, skyboxVAO, programState->inner_cubemapTexture);

        // HDR & BLOOM

        bool horizontal = true, first_iteration = true;
        unsigned int amount = 10;
        shader->blur.use();

        for (unsigned int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            shader->blur.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
            renderQuad();
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader->bloom.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        shader->bloom.setInt("bloom", bloom);
        shader->bloom.setFloat("exposure", exposure);
        renderQuad();

        //ImGui

        if (programState->ImGui1Enable || programState->ImGui2Enable) {
            drawImGui();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteVertexArrays(1, &transparentVAO);

    glDeleteBuffers(1, &transparentVBO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &skyboxVBO);

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
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

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

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !bloomKeyPressed) {
        bloom = !bloom;
        bloomKeyPressed = true;
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
        bloomKeyPressed = false;
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
    float yoffset = lastY - ypos;

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

void drawImGui() {
    // ImGui Frame Init
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (programState->ImGui1Enable) {
        {
            ImGui::Begin("Diamons's Universe");
            ImGui::Text(
                    "Dobro dosli na moj projekat iz predmeta Racunarska grafika!\nOvaj projekat ilustruje zalazak sunca u kanjonu sa portalom koji vodi u drugu dimenziju, tj. Univerzum.\n"
                    "Za kretanje napred, nazad, levo, desno koristite dugmice W, S, A, D. Za zoom koristite skrol misa.\n\n"
                    "Ukoliko zelite da iskucite ovaj prozor pritisnite F1. \n(Naravno, ukoliko zelite ponovo da ga ukljucite isto pritisnite F1)\n");
            ImGui::End();
        }
    }

    if (programState->ImGui2Enable) {
        {
            ImGui::Begin("Settings");
            ImGui::Text("Ovde mozete da promenite velicinu dijamanta.\n\n");
            ImGui::DragFloat("<- Diamond scale", &programState->diamondScale, 0.01f, 0.0, 2.2);
            ImGui::Text("\n\nUkoliko zelite mozete da menjate i transparentnost dijamanta:\n\n");
            ImGui::DragFloat("<- Diamond transparent", &programState->diamondTransparent, 0.005f, 0.0, 1.0);
            ImGui::End();
        }
    }

    if (programState->inSpace) {
        {
            ImGui::Begin("Hello from Universe!");
            ImGui::Text("Dobro dosli u Univerzum! Pritisnite dugme B za magiju!\n"
                        "Ukoliko zelite, mozete da menjate boju dijamantu.\n"
                        "P -> pink\n"
                        "C -> default\n"
                        "Ukoliko zelite da blurujete pozadinu pritisnite ENTER.\n"
                        "Ukoliko zelite mozete da menjate i neke karakteristike dijamanta pritiskom na F2.");
            ImGui::End();
        }
    }

    // ImGui render
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void drawSkyBox(Shader objShader, unsigned int objVAO, unsigned int texture) {
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    objShader.use();

    objShader.setMat4("view", glm::mat4(glm::mat3(programState->camera.GetViewMatrix())));
    objShader.setMat4("projection", glm::perspective(glm::radians(programState->camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f , 100.0f));

    glBindVertexArray(objVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGui1Enable = !programState->ImGui1Enable;
    }

    if (key == GLFW_KEY_F2 && action == GLFW_PRESS) {
        programState->ImGui2Enable = !programState->ImGui2Enable;
        if (programState->ImGui2Enable) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    if (key == GLFW_KEY_B && action == GLFW_PRESS) {
        if (programState->bling) {
            programState->bling = false;
        } else {
            programState->bling = true;
        }
    }
}

void setDiamondLightsShader(Shader objShader, PointLight pointLight, DirLight dirLight, SpotLight spotLight) {
    double time = glfwGetTime();

    objShader.use();
    objShader.setVec3("pointLights[0].position",  glm::vec3(2.0f * cos(time), 4.0f, 4*sin(time)));
    objShader.setVec3("pointLights[0].ambient",  pointLight.ambient);
    objShader.setVec3("pointLights[0].diffuse",  pointLight.diffuse);
    objShader.setVec3("pointLights[0].specular",  pointLight.specular);
    objShader.setFloat("pointLights[0].constant",  pointLight.constant);
    objShader.setFloat("pointLights[0].linear",  pointLight.linear);
    objShader.setFloat("pointLights[0].quadratic",  pointLight.quadratic);

    objShader.setVec3("pointLights[1].position",  glm::vec3(1.0f * cos(time), -4.0f, 3*sin(time)));
    objShader.setVec3("pointLights[1].ambient",  pointLight.ambient);
    objShader.setVec3("pointLights[1].diffuse",  pointLight.diffuse);
    objShader.setVec3("pointLights[1].specular",  pointLight.specular);
    objShader.setFloat("pointLights[1].constant",  pointLight.constant);
    objShader.setFloat("pointLights[1].linear",  pointLight.linear);
    objShader.setFloat("pointLights[1].quadratic",  pointLight.quadratic);

    objShader.setVec3("pointLights[2].position",  programState->pointLightsPositions[2]);
    objShader.setVec3("pointLights[2].ambient",  pointLight.ambient);
    objShader.setVec3("pointLights[2].diffuse",  pointLight.diffuse);
    objShader.setVec3("pointLights[2].specular",  pointLight.specular);
    objShader.setFloat("pointLights[2].constant",  pointLight.constant);
    objShader.setFloat("pointLights[2].linear",  pointLight.linear);
    objShader.setFloat("pointLights[2].quadratic",  pointLight.quadratic);

    objShader.setVec3("pointLights[3].position",  programState->pointLightsPositions[3]);
    objShader.setVec3("pointLights[3].ambient",  pointLight.ambient);
    objShader.setVec3("pointLights[3].diffuse",  pointLight.diffuse);
    objShader.setVec3("pointLights[3].specular",  pointLight.specular);
    objShader.setFloat("pointLights[3].constant",  pointLight.constant);
    objShader.setFloat("pointLights[3].linear",  pointLight.linear);
    objShader.setFloat("pointLights[3].quadratic",  pointLight.quadratic);

    objShader.setVec3("dirLight.direction", dirLight.direction);
    objShader.setVec3("dirLight.ambient", dirLight.ambient);
    objShader.setVec3("dirLight.diffuse", dirLight.diffuse);
    objShader.setVec3("dirLight.specular", dirLight.specular);

    objShader.setVec3("spotLight.position", spotLight.position);
    objShader.setVec3("spotLight.direction", spotLight.direction);
    objShader.setVec3("spotLight.ambient", spotLight.ambient);
    objShader.setVec3("spotLight.diffuse", spotLight.diffuse);
    objShader.setVec3("spotLight.specular", spotLight.specular);
    objShader.setFloat("spotLight.constant", spotLight.constant);
    objShader.setFloat("spotLight.linear", spotLight.linear);
    objShader.setFloat("spotLight.quadratic", spotLight.quadratic);
    objShader.setFloat("spotLight.cutOff", spotLight.cutOff);
    objShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);

    objShader.setVec3("viewPos",  programState->camera.Position);
    objShader.setFloat("material.shininess", 64.0f);
}

void setLightsShader(Shader objShader, PointLight pointLight, DirLight dirLight, SpotLight spotLight, glm::vec3 dirLightDiffuse, glm::vec3 dirLightAmbient) {
    double time = glfwGetTime();

    objShader.use();
    objShader.setVec3("pointLights[0].position", glm::vec3(-2.f * cos(time), -2.0f * cos(time), -5.f * sin(time) / 2));
    objShader.setVec3("pointLights[0].ambient",  glm::vec3(0.05f));
    objShader.setVec3("pointLights[0].diffuse",  pointLight.diffuse);
    objShader.setVec3("pointLights[0].specular",  pointLight.specular);
    objShader.setFloat("pointLights[0].constant",  pointLight.constant);
    objShader.setFloat("pointLights[0].linear",  pointLight.linear);
    objShader.setFloat("pointLights[0].quadratic",  pointLight.quadratic);

    objShader.setVec3("pointLights[1].position",  glm::vec3(1.0f * cos(time), -4.0f, 3*sin(time)));
    objShader.setVec3("pointLights[1].ambient",  glm::vec3(0.05f));
    objShader.setVec3("pointLights[1].diffuse",  pointLight.diffuse);
    objShader.setVec3("pointLights[1].specular",  pointLight.specular);
    objShader.setFloat("pointLights[1].constant",  pointLight.constant);
    objShader.setFloat("pointLights[1].linear",  pointLight.linear);
    objShader.setFloat("pointLights[1].quadratic",  pointLight.quadratic);

    objShader.setVec3("pointLights[2].position",  glm::vec3(0.f, 3.5f * cos(time) , -2.0f * sin(time) / 2));
    objShader.setVec3("pointLights[2].ambient",  pointLight.ambient);
    objShader.setVec3("pointLights[2].diffuse",  pointLight.diffuse);
    objShader.setVec3("pointLights[2].specular",  pointLight.specular);
    objShader.setFloat("pointLights[2].constant",  pointLight.constant);
    objShader.setFloat("pointLights[2].linear",  pointLight.linear);
    objShader.setFloat("pointLights[2].quadratic",  pointLight.quadratic);

    objShader.setVec3("pointLights[3].position",  programState->pointLightsPositions[3]);
    objShader.setVec3("pointLights[3].ambient",  pointLight.ambient);
    objShader.setVec3("pointLights[3].diffuse",  pointLight.diffuse);
    objShader.setVec3("pointLights[3].specular",  pointLight.specular);
    objShader.setFloat("pointLights[3].constant",  pointLight.constant);
    objShader.setFloat("pointLights[3].linear",  pointLight.linear);
    objShader.setFloat("pointLights[3].quadratic",  pointLight.quadratic);

    objShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
    objShader.setVec3("dirLight.ambient", dirLightAmbient);
    objShader.setVec3("dirLight.diffuse", dirLightDiffuse);
    objShader.setVec3("dirLight.specular", 0.25f, 0.25f, 0.25f);

    objShader.setVec3("spotLight.position", programState->camera.Position);
    objShader.setVec3("spotLight.direction", programState->camera.Front);
    objShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
    objShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
    objShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
    objShader.setFloat("spotLight.constant", 1.0f);
    objShader.setFloat("spotLight.linear", 0.09);
    objShader.setFloat("spotLight.quadratic", 0.032);
    objShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
    objShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

    objShader.setVec3("viewPos",  programState->camera.Position);
    objShader.setFloat("material.shininess", 32.0f);
}

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}