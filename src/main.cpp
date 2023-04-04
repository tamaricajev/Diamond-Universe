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
    ProgramState() : camera(glm::vec3(0.0f, 0.0f, 10.0f)) {}

    std::vector<std::string> faces;
    unsigned int cubemapTexture;

    std::vector<std::string> inner_faces;
    unsigned int inner_cubemapTexture;
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

    programState = new ProgramState;

    // build and compile shaders
    Shader cubeShader("resources/shaders/cube.vs", "resources/shaders/cube.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader inner_skyboxShader("resources/shaders/inner_skybox.vs", "resources/shaders/inner_skybox.fs");

    // coordinate system
    float cube_vertices[] = {
            // positions                    // normals
            -5.0f, -5.0, -5.0,  .0f, 0.0f, -1.0f,
            5.0f, -5.0, -5.0,  .0f, 0.0f, -1.0f,
            5.0f,  5.0, -5.0,  .0f, .0f, -1.0f,
            5.0f,  5.0, -5.0,  .0f, .0f, -1.0f,
            -5.0f,  5.0, -5.0,  0.0f, .0f, -1.0f,
            -5.0f, -5.0, -5.0,  0.0f, 0.0f, -1.0f,

            -5.0, -5.0,  5.0,  0.0f, 0.0f, 1.0f,
            5.0, -5.0,  5.0,  .0f, 0.0f, 1.0f,
            5.0,  5.0,  5.0,  .0f, .0f, 1.0f,
            5.0,  5.0,  5.0,  .0f, .0f, 1.0f,
            -5.0,  5.0,  5.0,  0.0f, .0f,1.0f,
            -5.0, -5.0,  5.0,  0.0f, 0.0f,1.0f,

            -5.0,  5.0,  5.0,  -1.0f, 0.0f,0.0f,
            -5.0,  5.0, -5.0,  -1.0f, .0f,0.0f,
            -5.0, -5.0, -5.0,  -1.0f, .0f,0.0f,
            -5.0, -5.0, -5.0,  -1.0f, .0f,0.0f,
            -5.0, -5.0,  5.0,  -1.0f, 0.0f,0.0f,
            -5.0,  5.0,  5.0,  -1.0f, 0.0f,0.0f,

            5.0,  5.0,  5.0,  1.0f, 0.0f,0.0f,
            5.0,  5.0, -5.0,  1.0f, .0f,0.0f,
            5.0, -5.0, -5.0,  1.0f, .0f,0.0f,
            5.0, -5.0, -5.0,  1.0f, .0f,0.0f,
            5.0, -5.0,  5.0,  1.0f, 0.0f,0.0f,
            5.0,  5.0,  5.0,  1.0f, 0.0f,0.0f,

            -5.0, -5.0, -5.0,  0.0f, -1.0f,0.0f,
            5.0, -5.0, -5.0,  .0f, -1.0f,0.0f,
            5.0, -5.0,  5.0,  .0f, -1.0f,0.0f,
            5.0, -5.0,  5.0,  .0f, -1.0f,0.0f,
            -5.0, -5.0,  5.0,  0.0f, -1.0f,0.0f,
            -5.0, -5.0, -5.0,  0.0f, -1.0f,0.0f,

            -5.0,  5.0, -5.0,  0.0f, 1.0f,0.0f,
            5.0,  5.0, -5.0,  .0f, 1.0f,0.0f,
            5.0,  5.0,  5.0,  .0f, 1.0f,0.0f,
            5.0,  5.0,  5.0,  .0f, 1.0f,0.0f,
            -5.0,  5.0,  5.0,  0.0f, 1.0f,0.0f,
            -5.0,  5.0, -5.0f,  0.0f, 1.0f, 0.0f
    };


    float inner_skybox_vertices[] = {
            // positions
            -5.0f,  5.0f, -5.0f,
            -5.0f, -5.0f, -5.0f,
            5.0f, -5.0f, -5.0f,
            5.0f, -5.0f, -5.0f,
            5.0f,  5.0f, -5.0f,
            -5.0f,  5.0f, -5.0f,

            -5.0f, -5.0f,  5.0f,
            -5.0f, -5.0f, -5.0f,
            -5.0f,  5.0f, -5.0f,
            -5.0f,  5.0f, -5.0f,
            -5.0f,  5.0f,  5.0f,
            -5.0f, -5.0f,  5.0f,

            5.0f, -5.0f, -5.0f,
            5.0f, -5.0f,  5.0f,
            5.0f,  5.0f,  5.0f,
            5.0f,  5.0f,  5.0f,
            5.0f,  5.0f, -5.0f,
            5.0f, -5.0f, -5.0f,

            -5.0f, -5.0f,  5.0f,
            -5.0f,  5.0f,  5.0f,
            5.0f,  5.0f,  5.0f,
            5.0f,  5.0f,  5.0f,
            5.0f, -5.0f,  5.0f,
            -5.0f, -5.0f,  5.0f,

            -5.0f,  5.0f, -5.0f,
            5.0f,  5.0f, -5.0f,
            5.0f,  5.0f,  5.0f,
            5.0f,  5.0f,  5.0f,
            -5.0f,  5.0f,  5.0f,
            -5.0f,  5.0f, -5.0f,

            -5.0f, -5.0f, -5.0f,
            -5.0f, -5.0f,  5.0f,
            5.0f, -5.0f, -5.0f,
            5.0f, -5.00f, -5.00f,
            -5.00f, -5.0f,  5.0f,
            5.0f, -5.0f,  5.0f
    };

    // skybox
    float skyboxVertices[] = {
            // positions
            -10.f,  10.f, -10.f,
            -10.f, -10.f, -10.f,
            10.f, -10.f, -10.f,
            10.f, -10.f, -10.f,
            10.f,  10.f, -10.f,
            -10.f,  10.f, -10.f,

            -10.f, -10.f,  10.f,
            -10.f, -10.f, -10.f,
            -10.f,  10.f, -10.f,
            -10.f,  10.f, -10.f,
            -10.f,  10.f,  10.f,
            -10.f, -10.f,  10.f,

            10.f, -10.f, -10.f,
            10.f, -10.f,  10.f,
            10.f,  10.f,  10.f,
            10.f,  10.f,  10.f,
            10.f,  10.f, -10.f,
            10.f, -10.f, -10.f,

            -10.f, -10.f,  10.f,
            -10.f,  10.f,  10.f,
            10.f,  10.f,  10.f,
            10.f,  10.f,  10.f,
            10.f, -10.f,  10.f,
            -10.f, -10.f,  10.f,

            -10.f,  10.f, -10.f,
            10.f,  10.f, -10.f,
            10.f,  10.f,  10.f,
            10.f,  10.f,  10.f,
            -10.f,  10.f,  10.f,
            -10.f,  10.f, -10.f,

            -10.f, -10.f, -10.f,
            -10.f, -10.f,  10.f,
            10.f, -10.f, -10.f,
            10.f, -10.0f, -10.0f,
            -10.0f, -10.f,  10.f,
            10.f, -10.f,  10.f
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(inner_skybox_vertices), inner_skybox_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // skybox
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

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

//    inner_skyboxShader.use();
    inner_skyboxShader.setInt("skybox", 0);

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
        glm::mat4 model = glm::mat4(1.0f);

        cubeShader.use();
        cubeShader.setMat4("projection", projection);
        cubeShader.setMat4("view", view);
        cubeShader.setMat4("model", model);
        cubeShader.setVec3("cameraPos", programState->camera.Position);

        // cube
        glBindVertexArray(cubeVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, programState->cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

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
        inner_skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix()));
        inner_skyboxShader.setMat4("projection", projection);
        inner_skyboxShader.setMat4("view", view);

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

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
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

//void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
//
//}