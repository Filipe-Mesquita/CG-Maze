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

// Tamanho do labirinto
//
int MAZE_W;
int MAZE_H;
const float CELL_SIZE = 1.0f;

std::vector<std::vector<int>> maze;

/*--------------------------------------*/
int transferDataToGPUMemory(void);
void generateMaze();
void carveMaze(int x, int z);

// settings
/*
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;
*/

// camera
Camera camera(glm::vec3(-1.5f, 0.5f, 1.0f));
float lastX;
float lastY;
bool firstMouse = true;
float centerX;
float centerY;
float fixY = true; // Controls if the player's Y is fixed or not

float mouse_sense = 1.0f;
float arrow_sense = 3.0f;

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

// Textura
int wallWidth;
int wallHeight;
int wallNrChannels;
char wall_texture_File[] = "./textures/bricks_wall_texture.png";
unsigned int wallTexture;
unsigned char *wallData;

// Floor
std::vector<glm::vec3> floor_vertices;
std::vector<glm::vec2> floor_uvs;
std::vector<glm::vec3> floor_normals;


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

int choice;

int main()
{
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

    // build and compile our shader zprogram
    // ------------------------------------
    Shader lightingShader("./shaders/2.1.basic_lighting.vs", "./shaders/2.1.basic_lighting.fs");
    Shader lampShader("./shaders/2.1.lamp.vs", "./shaders/2.1.lamp.fs");

    if (transferDataToGPUMemory() == -1)
        return -1;

    prepareTextures();

    srand(time(NULL));
    generateMaze();

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

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // be sure to activate shader when setting uniforms/drawing objects
        lightingShader.use();
        // lightingShader.setVec3("objectColor", 1.0f, 0.5f, 0.31f);
        lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
        lightingShader.setVec3("lightPos", lightPos);
        lightingShader.setVec3("viewPos", camera.Position);

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
                                                      x * CELL_SIZE,
                                                      0.0f,
                                                      z * CELL_SIZE));

                    lightingShader.setMat4("model", model);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, wallTexture);
                    lightingShader.setInt("texture1", 0);
                    glDrawArrays(GL_TRIANGLES, 0, wall_vertices.size());
                }
            }
        }

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

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &wall_VAO);
    glDeleteBuffers(1, &wall_VBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

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

int transferDataToGPUMemory(void)
{
    //Wall
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

    //Floor


    return 0;
}

void prepareTextures()
{
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

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            format,
            wallWidth,
            wallHeight,
            0,
            format,
            GL_UNSIGNED_BYTE,
            wallData);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load wall texture\n";
    }

    stbi_image_free(wallData);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime, fixY);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime, fixY);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime, fixY);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime, fixY);

    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
    {
        if (fixY)
            fixY = false;
        else
            fixY = true;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime, fixY);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime, fixY);

    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
        increaseArrowSense();
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
        decreaseArrowSense();

    /*
    1 -> cima
    2 -> baixo
    3 -> esquerda
    4 -> direita
    */
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        moveCamera(1);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        moveCamera(2);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        moveCamera(3);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        moveCamera(4);
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
