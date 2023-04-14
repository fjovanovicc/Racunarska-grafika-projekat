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

// Callbacks
void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

// Pomocne funkcije
unsigned int loadCubemap(vector<std::string> faces);

unsigned int loadTexture(char const * path);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float yaw   = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch =  0.0f;
float fov   =  45.0f;

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

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    float backpackScale = 0.2f;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if(in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Postavljanje callback funkcija (mis, tastatura itd)
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if(!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
//    programState->LoadFromFile("resources/program_state.txt");
    if(programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    glEnable(GL_DEPTH_TEST);        // Objekti na sceni trebaju biti ispred skybox-a
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);           // Odseci prednju stranu
    glFrontFace(GL_CW);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader blendingShader("resources/shaders/blending.vs", "resources/shaders/blending.fs");
    Shader faceCullingShader("resources/shaders/face_culling.vs", "resources/shaders/face_culling.fs");

    // load models
    // -----------
    Model modelEarth("resources/objects/earth/Earth.obj");
    modelEarth.SetShaderTextureNamePrefix("material.");
    Model modelRocket("resources/objects/rocket/Toy_Rocket.obj");
    modelRocket.SetShaderTextureNamePrefix("material.");
    Model modelAstronaut("resources/objects/astronaut/Astronaut.obj");
    modelAstronaut.SetShaderTextureNamePrefix("material.");
    Model modelMars("resources/objects/mars/Mars_2K.obj");
    modelMars.SetShaderTextureNamePrefix("material.");
    Model modelSun("resources/objects/sun/sun.obj");
    modelSun.SetShaderTextureNamePrefix(("material."));


    // ------------------------------------ SKYBOX ------------------------------------
    // Skybox ("kutija" 1x1x1)
    float skyboxVertices[] = {
        // positions
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

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // Ucitavanje tekstura za skybox
    vector<std::string> faces = {
        FileSystem::getPath("resources/textures/skybox/_front.png"),
        FileSystem::getPath("resources/textures/skybox/_back.png"),
        FileSystem::getPath("resources/textures/skybox/_bottom.png"),
        FileSystem::getPath("resources/textures/skybox/_top.png"),
        FileSystem::getPath("resources/textures/skybox/_right.png"),
        FileSystem::getPath("resources/textures/skybox/_left.png"),
    };

    unsigned int cubemapTexture = loadCubemap(faces);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);
    // ------------------------------------ SKYBOX ------------------------------------


    // ------------------------------------ BLENDING ------------------------------------
    float outsideTransparentVertices[] = {
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
            0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };

    float transparentVertices[] = {
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    };

    // outsideTransparent setup
    unsigned int outsideTransparentVerticesVAO, outsideTransparentVerticesVBO;
    glGenVertexArrays(1, &outsideTransparentVerticesVAO);
    glGenBuffers(1, &outsideTransparentVerticesVBO);
    glBindVertexArray(outsideTransparentVerticesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, outsideTransparentVerticesVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(outsideTransparentVertices), &outsideTransparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    // transparent setup
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), &transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    unsigned int outsideTransparentTexture = loadTexture(FileSystem::getPath("resources/textures/container.jpg").c_str());
    unsigned int transparentTexture = loadTexture(FileSystem::getPath("resources/textures/window_60percent.png").c_str());

    blendingShader.use();
    blendingShader.setInt("texture1", 0);
    // ------------------------------------ BLENDING ------------------------------------


    // ---------------------------------- FACE CULLING ----------------------------------
    float faceCullingBoxVertices[] = {
            // back face
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // bottom-left
            0.5f, -0.5f, -0.5f,  1.0f, 0.0f, // bottom-right
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // top-left
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // bottom-left
            // front face
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
            0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // top-right
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-right
            0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // top-right
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
            -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, // top-left
            // left face
            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-right
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-left
            -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-left
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-left
            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-right
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-right
            // right face
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-left
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
            0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-right
            0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-right
            0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-left
            // bottom face
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // top-right
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-left
            0.5f, -0.5f, -0.5f,  1.0f, 1.0f, // top-left
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-left
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // top-right
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-right
            // top face
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // top-left
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // bottom-right
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // bottom-right
            -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f  // top-left
    };

    // faceCullingBox setup
    unsigned int faceCullingBoxVAO, faceCullingBoxVBO;
    glGenVertexArrays(1, &faceCullingBoxVAO);
    glGenBuffers(1, &faceCullingBoxVBO);
    glBindVertexArray(faceCullingBoxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, faceCullingBoxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(faceCullingBoxVertices), &faceCullingBoxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    unsigned int faceCullingTexture = loadTexture(FileSystem::getPath("resources/textures/container.jpg").c_str());

    faceCullingShader.use();
    faceCullingShader.setInt("texture1", 0);
    // ---------------------------------- FACE CULLING ----------------------------------


    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(4.0f, 4.0, -10.0);    // Postavi na z=-10 da bude svetlije
    pointLight.ambient = glm::vec3(0.7, 0.7, 0.7);
    pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
//    pointLight.quadratic = 0.032f;    // Slabljenje svetlosti (kako se udaljava od pozicije tako slabi)
    pointLight.quadratic = 0.0f;


    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // ---------------------------- BLENDING (MAKETA RAKETE) ----------------------------
        blendingShader.use();
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);
        blendingShader.setMat4("projection", projection);
        blendingShader.setMat4("view", view);

        // Netransparentne strane kocke
        glBindVertexArray(outsideTransparentVerticesVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, outsideTransparentTexture);
        model = glm::translate(model, glm::vec3(-5.0f, 0.0f, -1.0f));
        blendingShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 30);      // Samo 5 strana kocke, 6 trouglova po strani, 6*5=30 trouglova

        // modelRocket (Velicina svakako nije proporcionalna zemlji i marsu)
        glm::mat4 modelMatrixRocketMini= glm::mat4(1.0f);
//        modelMatrixRocketMini = glm::translate(modelMatrixRocketMini, glm::vec3(-5.0f, -0.3f, -1.0f));    // Staticki model
        modelMatrixRocketMini = glm::translate(modelMatrixRocketMini, glm::vec3(-5.0f, -0.1f * cos(currentFrame) - 0.3f, -1.0f));
        modelMatrixRocketMini = glm::scale(modelMatrixRocketMini, glm::vec3(0.2f));
        ourShader.setMat4("model", modelMatrixRocketMini);
        modelRocket.Draw(ourShader);

        // Transparentna strana kocke
        glBindVertexArray(transparentVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-5.0f, 0.0f, -1.0f));
        blendingShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 6);   // Samo jedna strana kocke, tj. 6 trouglova
        // ---------------------------- BLENDING (MAKETA RAKETE) ----------------------------


        // ---------------------------- FACE CULLING (MAKETA ASTRONAUTA) ----------------------------
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glFrontFace(GL_CW);
        glBindVertexArray(faceCullingBoxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, faceCullingTexture);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-5.0f, 0.0f, -5.0f));
        faceCullingShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDisable(GL_CULL_FACE);    // Da ne bi uticalo na druge objekte na sceni

        // modelAstronaut (Velicina svakako nije proporcionalna zemlji i marsu)
        glm::mat4 modelMatrixAstronautMini= glm::mat4(1.0f);
//        modelMatrixRocketMini = glm::translate(modelMatrixRocketMini, glm::vec3(-5.0f, -0.3f, -1.0f));    // Staticki model
        modelMatrixAstronautMini = glm::translate(modelMatrixAstronautMini, glm::vec3(-5.0f, -0.1f * cos(currentFrame) - 0.3f, -5.0f));
        modelMatrixAstronautMini = glm::rotate(modelMatrixAstronautMini, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrixAstronautMini = glm::scale(modelMatrixAstronautMini, glm::vec3(0.15f));
        ourShader.setMat4("model", modelMatrixAstronautMini);
        modelAstronaut.Draw(ourShader);
        // ---------------------------- FACE CULLING (MAKETA ASTRONAUTA) ----------------------------


        // ourShader setup (koriscen za sve modele zemlje, marsa, atronauta i rakete)
        ourShader.use();
        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);
        projection = glm::perspective(glm::radians(programState->camera.Zoom),(float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);


        //------------------------------------ RENDEROVANJE MODELA  ------------------------------------
        // modelEarth
        glm::mat4 modelMatrixEarth = glm::mat4(1.0f);
        modelMatrixEarth = glm::translate(modelMatrixEarth, glm::vec3(0.0f, -5.0f, -25.0f));
        // Rotiramo Zemlju tako da izgleda kao da raketa polece sa Floride
        modelMatrixEarth = glm::rotate(modelMatrixEarth, glm::radians(170.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrixEarth = glm::rotate(modelMatrixEarth, glm::radians(-40.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrixEarth = glm::scale(modelMatrixEarth, glm::vec3(4.5f));
        ourShader.setMat4("model", modelMatrixEarth);
        modelEarth.Draw(ourShader);

        // modelRocket (Velicina svakako nije proporcionalna zemlji i marsu)
        glm::mat4 modelMatrixRocket= glm::mat4(1.0f);
        modelMatrixRocket = glm::translate(modelMatrixRocket, glm::vec3(8.0f, 1.9f, -20.0f));
        // Rotiramo raketu da izgleda kao da polece pod nekim uglom (vizuelno)
        modelMatrixRocket = glm::rotate(modelMatrixRocket, glm::radians(-50.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        modelMatrixRocket = glm::scale(modelMatrixRocket, glm::vec3(0.7f));
        ourShader.setMat4("model", modelMatrixRocket);
        modelRocket.Draw(ourShader);

        // modelMars   (Zemlja je priblizno 2 puta veca od marsa (0.53*Zemlja = Mars))
        glm::mat4 modelMatrixMars = glm::mat4(1.0f);
        modelMatrixMars = glm::translate(modelMatrixMars, glm::vec3(35.0f, 8.0f, -15.0f));
        modelMatrixMars = glm::scale(modelMatrixMars, glm::vec3(1.4f));
        ourShader.setMat4("model", modelMatrixMars);
        modelMars.Draw(ourShader);

        // modelAstronaut (Velicina svakako nije proporcionalna zemlji i marsu)
        glm::mat4 modelMatrixAstronaut = glm::mat4(1.0f);
        modelMatrixAstronaut = glm::translate(modelMatrixAstronaut, glm::vec3(34.5f, 12.7f, -14.0f));
        modelMatrixAstronaut = glm::rotate(modelMatrixAstronaut, glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrixAstronaut = glm::scale(modelMatrixAstronaut, glm::vec3(0.15f));
        ourShader.setMat4("model", modelMatrixAstronaut);
        modelAstronaut.Draw(ourShader);

        // modelAstronaut (drugi atronaut zarotiran ka prvom astronautu)
        glm::mat4 modelMatrixAstronaut2 = glm::mat4(1.0f);
        modelMatrixAstronaut2 = glm::translate(modelMatrixAstronaut2, glm::vec3(34.9f, 12.7f, -14.0f));
        modelMatrixAstronaut2 = glm::rotate(modelMatrixAstronaut2, glm::radians(-30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrixAstronaut2 = glm::scale(modelMatrixAstronaut2, glm::vec3(0.15f));
        ourShader.setMat4("model", modelMatrixAstronaut2);
        modelAstronaut.Draw(ourShader);
        //------------------------------------ RENDEROVANJE MODELA  ------------------------------------


        //------------------------------------ SKYBOX ------------------------------------
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix()));
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);
        //------------------------------------ SKYBOX ------------------------------------


        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
    glDeleteVertexArrays(1, &outsideTransparentVerticesVAO);
    glDeleteBuffers(1, &outsideTransparentVerticesVBO);
    glDeleteVertexArrays(1, &transparentVAO);
    glDeleteBuffers(1, &transparentVBO);
    glDeleteVertexArrays(1, &faceCullingBoxVAO);
    glDeleteBuffers(1, &faceCullingBoxVBO);
    glfwTerminate();
    return 0;
}

unsigned int loadCubemap(vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrComponents;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
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

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)   // ESC => terminate
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)                    // W => Pomeri se napred
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)                    // S => Pomeri se nazad
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)                    // A => Pomeri se levo
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)                    // D => Pomeri se desno
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
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

    float sensitivity = 0.5f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    // Ako smes da pomeras scenu misem onda update scenu (npr. ako je GUI ukljucen onda ne treba vrsiti update scene (kamere))
    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Backpack position", (float*)&programState->backpackPosition);
        ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("Change mouse speed", &programState->camera.speedCoef, 0.05, 1.0, 5.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if(programState->ImGuiEnabled) {    // Ukoliko je GUI otvoren radi sledece
            programState->CameraMouseMovementUpdateEnabled = false;                     // Ne pomeraj scenu (kameru) na pokret misa
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);      // Prikazi kursor
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);    // Ukloni kursor sa ekrana
        }
    }
}

unsigned int loadTexture(char const * path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
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