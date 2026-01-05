#include <./glad/include/glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <./include/shader_m.h>
#include <./include/camera.h>

#include <./include/objloader.hpp>

#include <iostream>

#include <ctime>

#include <./include/stb_image.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <sndfile.h>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
// NEW camera functions
void moveCamera(int direction);
void increaseArrowSense();
void decreaseArrowSense();

// Texture prepare function
void prepareTextures();

// Funções para modos
void setEasyMode();
void setNormalMode();
void setHardMode();

bool checkCollision(glm::vec3 pos);

void stopFootsteps();
void startFootsteps();
bool initAudio();
void shutdownAudio();

void createFullScreenQuad();
void createSceneFBO(int w, int h);

// Tamanho do labirinto
//
int MAZE_W;
int MAZE_H;
const float CELL_SIZE = 1.0f;

std::vector<std::vector<int>> maze;

/*--------------------------------------*/
int transferDataToGPUMemory(int choice);
void generateMaze();
void carveMaze(int x, int z);

// settings
/*
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;
*/

// Raio do jogador para colisões
//
const float PLAYER_RADIUS = 0.10f;

// Som
//
ALCdevice *alDevice = nullptr;
ALCcontext *alContext = nullptr;
ALuint stepBuffer = 0;
ALuint stepSource = 0;

bool isStepPlaying = false;

// camera
Camera camera(glm::vec3(-1.5f, 0.5f, 1.0f));
float lastX;
float lastY;
bool firstMouse = true;
float centerX;
float centerY;
bool fixY = true; // Controls if the player's Y is fixed or not

float mouse_sense = 1.0f;
float arrow_sense = 10.0f;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// lighting
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

// obj to load
char wall_mesh_File[] = "./meshes/wall.obj";

// Wall
std::vector<float> wall_bufferData;
unsigned int wall_VBO, wall_VAO;

// Arrays que vão armazenar as coordenadas dos vertices, uvs e normais do objeto
std::vector<glm::vec3> wall_vertices;
std::vector<glm::vec2> wall_uvs;
std::vector<glm::vec3> wall_normals;

// Wall Texture
int wallWidth;
int wallHeight;
int wallNrChannels;
char wall_texture_File[] = "./textures/bricks_wall_texture.png";
unsigned int wallTexture;
unsigned char *wallData;

// Floor
void generateFloor(int choice);

std::vector<glm::vec3> floor_vertices;
std::vector<glm::vec2> floor_uvs;
std::vector<glm::vec3> floor_normals;

std::vector<float> floor_bufferData;

unsigned int floor_VBO, floor_VAO;

// Bebado
//
unsigned int sceneFBO = 0;
unsigned int sceneColorTex = 0;
unsigned int sceneRBO = 0;

unsigned int quadVAO = 0, quadVBO = 0;

int SCR_W = 1280;
int SCR_H = 720;

// Floor Texture
int floorWidth;
int floorHeight;
int floorNrChannels;
char floor_texture_File[] = "./textures/floor_texture.png";
unsigned int floorTexture;
unsigned char *floorData;

// Spotlight
float ambientLightStrengh = 0.1f;
float innerCutOff = 12.5f;
float outerCutOff = 17.5f;
bool flashlightOn = true;

void setEasyMode()
{
    MAZE_W = 15;
    MAZE_H = 15;
}

void setNormalMode()
{
    MAZE_W = 21;
    MAZE_H = 21;
}

void setHardMode()
{
    MAZE_W = 51;
    MAZE_H = 51;
    // Colcocar depois o filtro de bebado, noite e uma lanterna
}

int main()
{
    int choice;
    do
    {
        std::cout << "Modo:" << std::endl;
        std::cout << "1 - Fácil" << std::endl;
        std::cout << "2 - Normal" << std::endl;
        std::cout << "3 - Difícil" << std::endl;
        std::cin >> choice;
        if (choice < 1 || choice > 3)
        {
            std::cout << "Opção inválida. Tente novamente." << std::endl;
        }
    } while (choice < 1 || choice > 3);
    switch (choice)
    {
    case 1:
        setEasyMode();
        break;
    case 2:
        setNormalMode();
        break;
    case 3:
        setHardMode();
        break;
    }

    bool drunkMode = (choice == 3);

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    lastX = mode->width / 2.0f;
    lastY = mode->height / 2.0f;

    centerX = mode->width / 2.0f;
    centerY = mode->height / 2.0f;

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(mode->width, mode->height, "Maze", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwShowWindow(window);
    glfwFocusWindow(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    ///////////////////////////////////////////////////////////////////////////////////////////
    if (!initAudio())
        std::cout << "Áudio não iniciado (continuo sem som)\n";

    // build and compile our shader zprogram
    // ------------------------------------
    Shader lightingShader("./shaders/2.1.basic_lighting.vs", "./shaders/2.1.basic_lighting.fs");
    Shader lampShader("./shaders/2.1.lamp.vs", "./shaders/2.1.lamp.fs");

    if (transferDataToGPUMemory(choice) == -1)
        return -1;

    prepareTextures();

    srand(time(NULL));
    generateMaze();

    Shader drunkShader("./shaders/postprocess.vs", "./shaders/drunk.fs");

    if (drunkMode)
    {
        createSceneFBO(SCR_W, SCR_H);
        createFullScreenQuad();
    }

    // Spawn automático na primeira célula de caminho (normalmente (1,1))
    for (int z = 0; z < MAZE_H; z++)
    {
        for (int x = 0; x < MAZE_W; x++)
        {
            if (maze[z][x] == 0)
            {
                camera.Position = glm::vec3(
                    1.0f * CELL_SIZE + 0.5f * CELL_SIZE,
                    0.5f,
                    1.0f * CELL_SIZE + 0.5f * CELL_SIZE);
                z = MAZE_H; // sair dos loops
                break;
            }
        }
    }

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // Verifica se o jogador chegou ao fim do labirinto
        // ( VER MELHOR )
        int px = (int)floor(camera.Position.x / CELL_SIZE);
        int pz = (int)floor(camera.Position.z / CELL_SIZE);

        if (pz == MAZE_H - 2 && px == MAZE_W - 1)
        {
            std::cout << "GANHASTEEEEEEEEE CARALHOOOOOOO!" << std::endl;
            glfwSetWindowShouldClose(window, true);
        }

        if (drunkMode)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
        }
        else
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // be sure to activate shader when setting uniforms/drawing objects
        lightingShader.use();
        // lightingShader.setVec3("objectColor", 1.0f, 0.5f, 0.31f);
        lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
        lightingShader.setFloat("ambientS", ambientLightStrengh);
        lightingShader.setVec3("lightPos", camera.Position);
        lightingShader.setVec3("lightDir", camera.Front);
        lightingShader.setVec3("viewPos", camera.Position);

        lightingShader.setBool("flashlightOn", flashlightOn);

        lightingShader.setFloat("cutOff", cos(glm::radians(innerCutOff)));
        lightingShader.setFloat("outerCutOff", cos(glm::radians(outerCutOff)));

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)mode->width / (float)mode->height, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        // world transformation
        glm::mat4 model = glm::mat4(1.0f);
        lightingShader.setMat4("model", model);

        // render dos cubos
        //
        glBindVertexArray(wall_VAO);

        for (int z = 0; z < MAZE_H; z++)
        {
            for (int x = 0; x < MAZE_W; x++)
            {
                if (maze[z][x] == 1) // arbusto
                {
                    glm::mat4 model = glm::mat4(1.0f);

                    model = glm::translate(model, glm::vec3(
                                                      (x + 0.5f) * CELL_SIZE,
                                                      0.0f,
                                                      (z + 0.5f) * CELL_SIZE));

                    lightingShader.setMat4("model", model);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, wallTexture);
                    lightingShader.setInt("texture1", 0);
                    glDrawArrays(GL_TRIANGLES, 0, wall_vertices.size());
                }
            }
        }

        // Render do chão
        glBindVertexArray(floor_VAO);

        model = glm::mat4(1.0f);
        lightingShader.setMat4("model", model);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        lightingShader.setInt("texture1", 1);

        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)floor_vertices.size());

        // also draw the lamp object
        lampShader.use();
        lampShader.setMat4("projection", projection);
        lampShader.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
        lampShader.setMat4("model", model);

        glBindVertexArray(wall_VAO);
        glDrawArrays(GL_TRIANGLES, 0, wall_vertices.size());

        if (drunkMode)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDisable(GL_DEPTH_TEST);

            drunkShader.use();
            drunkShader.setInt("sceneTex", 0);
            drunkShader.setFloat("time", (float)glfwGetTime());
            drunkShader.setFloat("intensity", 1.0f); // 0.8 a 1.4

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sceneColorTex);

            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
        }

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &wall_VAO);
    glDeleteBuffers(1, &wall_VBO);
    glDeleteVertexArrays(1, &floor_VAO);
    glDeleteBuffers(1, &floor_VBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    shutdownAudio();
    return 0;
}

// Funções
//

int loadMeshFromFile(char obj_file[], std::vector<float> &bufferData, std::vector<glm::vec3> &vertices, std::vector<glm::vec2> &uvs, std::vector<glm::vec3> &normals)
{
    if (!loadOBJ(obj_file, vertices, uvs, normals))
    {
        std::cout << "Failed to load OBJ file!" << std::endl;
        return -1;
    }

    // Store data in the buffer
    for (size_t i = 0; i < vertices.size(); ++i)
    {
        // Add vertex position
        bufferData.push_back(vertices[i].x);
        bufferData.push_back(vertices[i].y);
        bufferData.push_back(vertices[i].z);

        // Add normals (if they exist)
        if (i < normals.size())
        {
            bufferData.push_back(normals[i].x);
            bufferData.push_back(normals[i].y);
            bufferData.push_back(normals[i].z);
        }
        else
        {
            bufferData.push_back(0.0f);
            bufferData.push_back(0.0f);
            bufferData.push_back(0.0f);
        }

        // Add UV coordinates
        if (i < uvs.size())
        {
            bufferData.push_back(uvs[i].x);
            bufferData.push_back(uvs[i].y);
        }
        else
        {
            bufferData.push_back(0.0f);
            bufferData.push_back(0.0f);
        }
    }

    return 0;
}

int transferDataToGPUMemory(int choice)
{
    // Wall
    if (loadMeshFromFile(wall_mesh_File, wall_bufferData, wall_vertices, wall_uvs, wall_normals) == -1)
        return -1;

    // configure the deer's VAO (and VBO)
    glGenVertexArrays(1, &wall_VAO);
    glGenBuffers(1, &wall_VBO);

    glBindBuffer(GL_ARRAY_BUFFER, wall_VBO);
    glBufferData(GL_ARRAY_BUFFER, wall_bufferData.size() * sizeof(float), wall_bufferData.data(), GL_DYNAMIC_DRAW);

    glBindVertexArray(wall_VAO);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // UV atribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Floor
    generateFloor(choice);
    return 0;
}

void generateFloor(int choice)
{
    std::cout << "Generating scene floor\n";

    float size;

    switch (choice)
    {
    case 1:
        size = 16.0f;
        break;
    case 2:
        size = 21.0f;
        break;
    case 3:
        size = 51.0f;
        break;
    default:
        size = 16.0f;
        break;
    }

    float x0 = -2.0f;
    float z0 = -2.0f;
    float x1 = size;
    float z1 = size;

    float uMax = (x1 - x0) / CELL_SIZE; // quantas “células” o chão tem em X
    float vMax = (z1 - z0) / CELL_SIZE; // quantas “células” o chão tem em Z

    // --------- VÉRTICES (2 TRIÂNGULOS) ---------
    floor_vertices = {
        {x0, 0.0f, z0},
        {x0, 0.0f, z1},
        {x1, 0.0f, z0},

        {x1, 0.0f, z0},
        {x0, 0.0f, z1},
        {x1, 0.0f, z1}};

    // --------- UVs (TEXTURA REPETIDA) ----------
    floor_uvs = {
        {0.0f, 0.0f},
        {0.0f, vMax},
        {uMax, 0.0f},

        {uMax, 0.0f},
        {0.0f, vMax},
        {uMax, vMax}};

    // --------- NORMAIS (TODAS PARA CIMA) -------
    for (int i = 0; i < 6; i++)
    {
        floor_normals.push_back({0.0f, 1.0f, 0.0f});
    }

    // Store data in the buffer
    for (size_t i = 0; i < floor_vertices.size(); ++i)
    {
        // Add vertex position
        floor_bufferData.push_back(floor_vertices[i].x);
        floor_bufferData.push_back(floor_vertices[i].y);
        floor_bufferData.push_back(floor_vertices[i].z);

        // Add normals (if they exist)
        if (i < floor_normals.size())
        {
            floor_bufferData.push_back(floor_normals[i].x);
            floor_bufferData.push_back(floor_normals[i].y);
            floor_bufferData.push_back(floor_normals[i].z);
        }
        else
        {
            floor_bufferData.push_back(0.0f);
            floor_bufferData.push_back(0.0f);
            floor_bufferData.push_back(0.0f);
        }

        // Add UV coordinates
        if (i < floor_uvs.size())
        {
            floor_bufferData.push_back(floor_uvs[i].x);
            floor_bufferData.push_back(floor_uvs[i].y);
        }
        else
        {
            floor_bufferData.push_back(0.0f);
            floor_bufferData.push_back(0.0f);
        }
    }

    // Gerar VAO e VBO (uma vez)
    glGenVertexArrays(1, &floor_VAO);
    glGenBuffers(1, &floor_VBO);

    glBindVertexArray(floor_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, floor_VBO);
    glBufferData(GL_ARRAY_BUFFER, floor_bufferData.size() * sizeof(float), floor_bufferData.data(), GL_STATIC_DRAW);

    // STRIDE: 8 floats por vértice
    GLsizei stride = 8 * sizeof(float);

    // ---------- POSIÇÃO ----------
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
    glEnableVertexAttribArray(0);

    // ---------- NORMAL ----------
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // ---------- UV ----------
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void prepareTextures()
{
    // Wall
    std::cout << "Loading wall texture...\n";

    glGenTextures(1, &wallTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, wallTexture);

    // Wrapping / filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Flip (Blender compatibility)
    stbi_set_flip_vertically_on_load(true);

    wallData = stbi_load(wall_texture_File, &wallWidth, &wallHeight, &wallNrChannels, 0);

    if (wallData)
    {
        GLenum format;
        if (wallNrChannels == 1)
            format = GL_RED;
        else if (wallNrChannels == 3)
            format = GL_RGB;
        else if (wallNrChannels == 4)
            format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, wallWidth, wallHeight, 0, format, GL_UNSIGNED_BYTE, wallData);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load wall texture\n";
    }

    stbi_image_free(wallData);

    // Floor
    std::cout << "Loading floor texture...\n";

    glGenTextures(1, &floorTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, floorTexture);

    // Wrapping / filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Flip (Blender compatibility)
    stbi_set_flip_vertically_on_load(true);

    floorData = stbi_load(floor_texture_File, &floorWidth, &floorHeight, &floorNrChannels, 0);

    if (floorData)
    {
        GLenum format;
        if (floorNrChannels == 1)
            format = GL_RED;
        else if (floorNrChannels == 3)
            format = GL_RGB;
        else if (floorNrChannels == 4)
            format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, floorWidth, floorHeight, 0, format, GL_UNSIGNED_BYTE, floorData);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load floor texture\n";
    }

    stbi_image_free(floorData);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    static bool fPressedLastFrame = false;

    bool wantMove =
        glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    glm::vec3 oldPos = camera.Position;

    // mover (WASD)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime, fixY);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime, fixY);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime, fixY);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime, fixY);

    glm::vec3 target = camera.Position;

    // slide por eixos (X depois Z)
    camera.Position = glm::vec3(target.x, oldPos.y, oldPos.z);
    if (checkCollision(camera.Position))
        camera.Position.x = oldPos.x;

    camera.Position = glm::vec3(camera.Position.x, oldPos.y, target.z);
    if (checkCollision(camera.Position))
        camera.Position.z = oldPos.z;

    float moved = glm::length(glm::vec2(camera.Position.x - oldPos.x, camera.Position.z - oldPos.z));

    if (wantMove && moved > 0.0001f)
        startFootsteps();
    else
        stopFootsteps();

    // resto do teu input (inalterado)
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
        fixY = !fixY;

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime, fixY);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime, fixY);

    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
        increaseArrowSense();
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
        decreaseArrowSense();

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        moveCamera(1);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        moveCamera(2);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        moveCamera(3);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        moveCamera(4);

    // Flashlight
    bool fPressed = (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS); // Ativar/desativar o filtro
    if (fPressed && !fPressedLastFrame)
        flashlightOn = !flashlightOn;
    fPressedLastFrame = fPressed;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = (xpos - lastX) * mouse_sense;
    float yoffset = (lastY - ypos) * mouse_sense; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);

    glfwSetCursorPos(window, centerX, centerY);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}

void moveCamera(int direction)
{
    switch (direction)
    {
        /*
    1 -> cima
    2 -> baixo
    3 -> esquerda
    4 -> direita
    */
    case 1:
        camera.ProcessMouseMovement(0.0f, arrow_sense);
        break;
    case 2:
        camera.ProcessMouseMovement(0.0f, -arrow_sense);
        break;
    case 3:
        camera.ProcessMouseMovement(-arrow_sense, 0.0f);
        break;
    case 4:
        camera.ProcessMouseMovement(arrow_sense, 0.0f);
        break;

    default:
        break;
    }
}

void increaseArrowSense()
{
    arrow_sense += 0.5f;
}
void decreaseArrowSense()
{
    arrow_sense -= 0.5f;

    if (arrow_sense <= 0.0f)
        arrow_sense = 0.5f;
}

// Gerar labirinto
//

void carveMaze(int x, int z)
{
    static int dirs[4][2] = {
        {1, 0},  // direita
        {-1, 0}, // esquerda
        {0, 1},  // baixo
        {0, -1}  // cima
    };

    // baralhar direções
    for (int i = 0; i < 4; i++)
    {
        int r = rand() % 4;
        std::swap(dirs[i], dirs[r]);
    }

    for (int i = 0; i < 4; i++)
    {
        int nx = x + dirs[i][0] * 2;
        int nz = z + dirs[i][1] * 2;

        if (nx > 0 && nz > 0 && nx < MAZE_W - 1 && nz < MAZE_H - 1)
        {
            if (maze[nz][nx] == 1)
            {
                maze[z + dirs[i][1]][x + dirs[i][0]] = 0;
                maze[nz][nx] = 0;
                carveMaze(nx, nz);
            }
        }
    }
}

void generateMaze()
{
    maze.resize(MAZE_H, std::vector<int>(MAZE_W, 1));

    for (int z = 0; z < MAZE_H; z++)
        for (int x = 0; x < MAZE_W; x++)
            maze[z][x] = 1;

    maze[1][1] = 0;
    carveMaze(1, 1);

    for (int x = 0; x < MAZE_W; x++)
    {
        maze[0][x] = 1;
        maze[MAZE_H - 1][x] = 1;
    }

    for (int z = 0; z < MAZE_H; z++)
    {
        maze[z][0] = 1;
        maze[z][MAZE_W - 1] = 1;
    }

    maze[1][0] = 0;
    maze[1][1] = 0;

    int exitZ = MAZE_H - 2;

    maze[exitZ][MAZE_W - 2] = 0;
    maze[exitZ][MAZE_W - 1] = 0;
}

// Função de colisão
//
static inline float clampf(float v, float a, float b) { return (v < a) ? a : (v > b) ? b
                                                                                     : v; }

bool checkCollision(glm::vec3 pos)
{
    const float r = PLAYER_RADIUS;
    const float r2 = r * r;

    // limites do mundo
    const float minWorldX = 0.0f;
    const float minWorldZ = 0.0f;
    const float maxWorldX = MAZE_W * CELL_SIZE;
    const float maxWorldZ = MAZE_H * CELL_SIZE;

    if (pos.x - r < minWorldX || pos.x + r > maxWorldX ||
        pos.z - r < minWorldZ || pos.z + r > maxWorldZ)
        return true;

    int cx = (int)floor(pos.x / CELL_SIZE);
    int cz = (int)floor(pos.z / CELL_SIZE);

    for (int dz = -1; dz <= 1; dz++)
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            int mx = cx + dx;
            int mz = cz + dz;

            if (mx < 0 || mx >= MAZE_W || mz < 0 || mz >= MAZE_H)
                continue;

            if (maze[mz][mx] != 1)
                continue;

            float minX = mx * CELL_SIZE;
            float maxX = minX + CELL_SIZE;
            float minZ = mz * CELL_SIZE;
            float maxZ = minZ + CELL_SIZE;

            float closestX = clampf(pos.x, minX, maxX);
            float closestZ = clampf(pos.z, minZ, maxZ);

            float dxp = pos.x - closestX;
            float dzp = pos.z - closestZ;

            if ((dxp * dxp + dzp * dzp) < r2)
                return true;
        }
    }
    return false;
}

// Som
// FUTURO: COLOCAR SOM DA LATERNA
//
bool loadWavToOpenAL(const char *filename, ALuint &outBuffer)
{
    SF_INFO sfinfo;
    SNDFILE *sndfile = sf_open(filename, SFM_READ, &sfinfo);
    if (!sndfile)
    {
        std::cout << "Erro a abrir som: " << filename << "\n";
        return false;
    }

    std::vector<short> samples(sfinfo.frames * sfinfo.channels);
    sf_readf_short(sndfile, samples.data(), sfinfo.frames);
    sf_close(sndfile);

    ALenum format;
    if (sfinfo.channels == 1)
        format = AL_FORMAT_MONO16;
    else if (sfinfo.channels == 2)
        format = AL_FORMAT_STEREO16;
    else
    {
        std::cout << "Formato WAV não suportado (channels=" << sfinfo.channels << ")\n";
        return false;
    }

    alGenBuffers(1, &outBuffer);
    alBufferData(outBuffer, format, samples.data(),
                 (ALsizei)(samples.size() * sizeof(short)), sfinfo.samplerate);

    return true;
}

bool initAudio()
{
    alDevice = alcOpenDevice(nullptr);
    if (!alDevice)
    {
        std::cout << "Falha a abrir OpenAL device\n";
        return false;
    }

    alContext = alcCreateContext(alDevice, nullptr);
    if (!alContext || !alcMakeContextCurrent(alContext))
    {
        std::cout << "Falha a criar OpenAL context\n";
        return false;
    }

    if (!loadWavToOpenAL("./sounds/step.wav", stepBuffer))
        return false;

    alGenSources(1, &stepSource);
    alSourcei(stepSource, AL_BUFFER, stepBuffer);
    alSourcef(stepSource, AL_GAIN, 1.0f);       // volume
    alSourcef(stepSource, AL_PITCH, 1.0f);      // pitch
    alSourcei(stepSource, AL_LOOPING, AL_TRUE); // loop contínuo

    return true;
}

void shutdownAudio()
{
    if (stepSource)
        alDeleteSources(1, &stepSource);
    if (stepBuffer)
        alDeleteBuffers(1, &stepBuffer);

    if (alContext)
    {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(alContext);
    }
    if (alDevice)
        alcCloseDevice(alDevice);
}

void startFootsteps()
{
    if (!isStepPlaying)
    {
        alSourcePlay(stepSource);
        isStepPlaying = true;
    }
}

void stopFootsteps()
{
    if (isStepPlaying)
    {
        alSourceStop(stepSource);
        isStepPlaying = false;
    }
}

// Bebado
//
void createSceneFBO(int w, int h)
{
    if (sceneFBO == 0)
        glGenFramebuffers(1, &sceneFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

    if (sceneColorTex == 0)
        glGenTextures(1, &sceneColorTex);
    glBindTexture(GL_TEXTURE_2D, sceneColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneColorTex, 0);

    if (sceneRBO == 0)
        glGenRenderbuffers(1, &sceneRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, sceneRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, sceneRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "FBO incompleto!\n";

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void createFullScreenQuad()
{
    float quadVertices[] = {
        // pos      // uv
        -1.f, -1.f, 0.f, 0.f,
        1.f, -1.f, 1.f, 0.f,
        1.f, 1.f, 1.f, 1.f,

        -1.f, -1.f, 0.f, 0.f,
        1.f, 1.f, 1.f, 1.f,
        -1.f, 1.f, 0.f, 1.f};

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));

    glBindVertexArray(0);
}
