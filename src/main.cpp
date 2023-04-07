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
//void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

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
    Camera camera;
    Model diamond, pink_diamond, mars, venus, sun;

    ProgramState() : camera(glm::vec3(0.0f, 0.0f, 30.0f)),
                    diamond(FileSystem::getPath("resources/objects/diamond/Diamond.obj")),
                    pink_diamond(FileSystem::getPath("resources/objects/pink_diamond/Diamond.obj")),
                    mars(FileSystem::getPath("resources/objects/mars/planet.obj")),
                    venus(FileSystem::getPath("resources/objects/venus/planet.obj")),
                    sun(FileSystem::getPath("resources/objects/sun/planet.obj")) {}

    std::vector<std::string> faces, inner_faces;
    unsigned int cubemapTexture, inner_cubemapTexture;

    std::string color;
};

ProgramState *programState;

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

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    programState = new ProgramState;

    // build and compile shaders
    Shader cubeShader("resources/shaders/cube/cube.vs", "resources/shaders/cube/cube.fs");
    Shader skyboxShader("resources/shaders/skybox/skybox.vs", "resources/shaders/skybox/skybox.fs");
    Shader diamondShader("resources/shaders/diamond/diamond.vs", "resources/shaders/diamond/diamond.fs");
    Shader transparentShader("resources/shaders/window/transparent.vs", "resources/shaders/window/transparent.fs");
    Shader planetShader("resources/shaders/planet/planet.vs", "resources/shaders/planet/planet.fs");

    // cube
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

    // cube VBO and VAO
    unsigned int cubeVBO, cubeVAO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // inner skybox VBO and VAO
    unsigned int inner_skyboxVBO, inner_skyboxVAO;
    glGenVertexArrays(1, &inner_skyboxVAO);
    glGenBuffers(1, &inner_skyboxVBO);
    glBindVertexArray(inner_skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, inner_skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyBox_vertices), skyBox_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // skybox
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyBox_vertices), &skyBox_vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // transparent VAO
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    programState->inner_faces =
            {
                    FileSystem::getPath("resources/textures/skybox/right.jpg"),
                    FileSystem::getPath("resources/textures/skybox/left.jpg"),
                    FileSystem::getPath("resources/textures/skybox/top.jpg"),
                    FileSystem::getPath("resources/textures/skybox/bottom.jpg"),
                    FileSystem::getPath("resources/textures/skybox/front.jpg"),
                    FileSystem::getPath("resources/textures/skybox/back.jpg")
            };

    programState->inner_cubemapTexture = loadCubemap(programState->inner_faces);

    cubeShader.use();
    cubeShader.setInt("skybox", 0);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    programState->faces =
            {
                    FileSystem::getPath("resources/textures/inner_skybox/right.jpg"),
                    FileSystem::getPath("resources/textures/inner_skybox/left.jpg"),
                    FileSystem::getPath("resources/textures/inner_skybox/top.jpg"),
                    FileSystem::getPath("resources/textures/inner_skybox/bottom.jpg"),
                    FileSystem::getPath("resources/textures/inner_skybox/front.jpg"),
                    FileSystem::getPath("resources/textures/inner_skybox/back.jpg")
            };

    programState->cubemapTexture = loadCubemap(programState->faces);

    vector<glm::vec3> windows
            {
                    glm::vec3(-5.5f, 0.0f, -0.48f)
            };

    unsigned int transparentTexture = loadTexture(FileSystem::getPath("resources/textures/window.png").c_str());

    transparentShader.use();
    transparentShader.setInt("texture1", 0);

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

        cubeShader.use();
        cubeShader.setMat4("projection", projection);
        cubeShader.setMat4("view", view);
        cubeShader.setMat4("model", cubeModel);
        cubeShader.setVec3("cameraPos", programState->camera.Position);

        glBindVertexArray(cubeVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, programState->cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        // diamonds models
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -1.0f, -2.0f));
        model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(2.f, 2.0f, 2.0f));
        diamondShader.use();
        diamondShader.setMat4("projection", projection);
        diamondShader.setMat4("view", view);
        diamondShader.setMat4("model", model);

        if (programState->color == "clear") {
            stbi_set_flip_vertically_on_load(true);
            programState->diamond.Draw(diamondShader);
            stbi_set_flip_vertically_on_load(false);
        } else if (programState->color == "pink") {
            stbi_set_flip_vertically_on_load(true);
            programState->pink_diamond.Draw(diamondShader);
            stbi_set_flip_vertically_on_load(false);
        } else {
            stbi_set_flip_vertically_on_load(true);
            programState->diamond.Draw(diamondShader);
            stbi_set_flip_vertically_on_load(false);
        }

        // mars model
        float time = glfwGetTime();

        glm::mat4 marsModel = glm::mat4(1.0f);
        marsModel = glm::translate(marsModel, glm::vec3(-2.0f * cos(time), -2.0f * cos(time), -4.5f * sin(time) / 2));
        marsModel = glm::translate(marsModel, glm::vec3(0.0f, 0.0f, -2.0f));
        marsModel = glm::rotate(marsModel, (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
        marsModel = glm::scale(marsModel, glm::vec3(0.08f, 0.08f, 0.08f));

        planetShader.use();
        planetShader.setMat4("projection", projection);
        planetShader.setMat4("view", view);
        planetShader.setMat4("model", marsModel);

        stbi_set_flip_vertically_on_load(true);
        programState->mars.Draw(planetShader);
        stbi_set_flip_vertically_on_load(false);

        // venus model
        glm::mat4 venusModel = glm::mat4(1.0f);
        venusModel = glm::translate(venusModel, glm::vec3(2.0f * cos(time), -2.0f * cos(time), 4.5f * sin(time) / 2));
        venusModel = glm::translate(venusModel, glm::vec3(.0f, .0f, -2.0f));
        venusModel = glm::rotate(venusModel, (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
        venusModel = glm::scale(venusModel, glm::vec3(0.08f, 0.08f, 0.08f));

        planetShader.use();
        planetShader.setMat4("projection", projection);
        planetShader.setMat4("view", view);
        planetShader.setMat4("model", venusModel);

        stbi_set_flip_vertically_on_load(true);
        programState->venus.Draw(planetShader);
        stbi_set_flip_vertically_on_load(false);


        // sun model
        glm::mat4 sunModel = glm::mat4(1.0f);
        sunModel = glm::translate(sunModel, glm::vec3(2.5f * cos(time + 10), .0f , -4.0f * sin(time) / 2));
        sunModel = glm::translate(sunModel, glm::vec3(.0f, .0f, -2.0f));
        sunModel = glm::rotate(sunModel, (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
        sunModel = glm::scale(sunModel, glm::vec3(0.08f, 0.08f, 0.08f));

        planetShader.use();
        planetShader.setMat4("projection", projection);
        planetShader.setMat4("view", view);
        planetShader.setMat4("model", sunModel);

        stbi_set_flip_vertically_on_load(true);
        programState->sun.Draw(planetShader);
        stbi_set_flip_vertically_on_load(false);

        // transparent window
        transparentShader.use();
        transparentShader.setMat4("projection", projection);
        transparentShader.setMat4("view", view);
        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-8.45f, 0.0f, 8.6f));
        model = glm::scale(model, glm::vec3(16.9f, 16.9f, 0.0f));
        transparentShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        //draw skybox in the end
        // main skybox
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix()));
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

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
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix()));
        skyboxShader.setMat4("projection", projection);
        skyboxShader.setMat4("view", view);

        glBindVertexArray(inner_skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, programState->inner_cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);

        // skybox end

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

//    ImGui_ImplOpenGL3_Shutdown();
//    ImGui_ImplGlfw_Shutdown();
//    ImGui::DestroyContext();

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteVertexArrays(1, &inner_skyboxVAO);
    glDeleteVertexArrays(1, &transparentVAO);

    glDeleteBuffers(1, &transparentVBO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &skyboxVBO);
    glDeleteBuffers(1, &inner_skyboxVBO);


    delete programState;

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

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
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

    programState->camera.ProcessMouseMovement(xoffset, yoffset);
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

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

//void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
//
//}