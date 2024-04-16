#include "engine.h"
#include <sstream>
#include <random>
#include <cstdlib>
#include <ctime>


enum state {start, play, over};
state screen;

int clickedLights = 0;
bool gameStarted = false;
double startTime = 0;
double endTime = 0;


// Colors
color yellow, gray;


Engine::Engine() : keys() {
    this->initWindow();
    this->initShaders();
    this->initShapes();

    yellow = {1, 1, 0, 1};
    gray = {.5,.5,.5,1};
}

Engine::~Engine() {}

unsigned int Engine::initWindow(bool debug) {
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, false);

    window = glfwCreateWindow(width, height, "Lights Out!", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    // OpenGL configuration
    glViewport(0, 0, width, height);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glfwSwapInterval(1);

    return 0;
}

void Engine::initShaders() {
    // load shader manager
    shaderManager = make_unique<ShaderManager>();

    // Load shader into shader manager and retrieve it
    shapeShader = this->shaderManager->loadShader("../res/shaders/shape.vert", "../res/shaders/shape.frag",  nullptr, "shape");

    // Configure text shader and renderer
    textShader = shaderManager->loadShader("../res/shaders/text.vert", "../res/shaders/text.frag", nullptr, "text");
    fontRenderer = make_unique<FontRenderer>(shaderManager->getShader("text"), "../res/fonts/MxPlus_IBM_BIOS.ttf", 24);

    // Set uniforms
    textShader.setVector2f("vertex", vec4(100, 100, .5, .5));
    shapeShader.use();
    shapeShader.setMatrix4("projection", this->PROJECTION);
}

void Engine::initShapes() {
    srand(static_cast<unsigned int>(time(0)));
    int x = 0;
    int y = 0;
    for (int i = 0; i < 5; i++) {
        x = 0;
        for (int j = 0; j < 5; j++) {
            vec2 pos = {150 + x, 50 + y};
            vec2 size = {90, 90};
            color color = {255, 255, 0};

            //added randomizer for lights on/off
            //bool rando = rand() % 2 == 0;
            //if (rando)
                //color = {.5,.5,.5,1};

            light.push_back(make_unique<Rect>(shapeShader, pos, size, color));
            x += 125;
        }
        y += 125;
    }
    border = make_unique<Rect>(shapeShader, vec2{-1000,-1000}, vec2{100, 100}, color{1, 0, 0, 1});
}

void Engine::processInput() {
    glfwPollEvents();

    // Set keys to true if pressed, false if released
    for (int key = 0; key < 1024; ++key) {
        if (glfwGetKey(window, key) == GLFW_PRESS)
            keys[key] = true;
        else if (glfwGetKey(window, key) == GLFW_RELEASE)
            keys[key] = false;
    }

    // Close window if escape key is pressed
    if (keys[GLFW_KEY_ESCAPE])
        glfwSetWindowShouldClose(window, true);

    // Mouse position saved to check for collisions
    glfwGetCursorPos(window, &MouseX, &MouseY);

    // Hint: The index is GLFW_KEY_S
    if (screen == start) {
        if (keys[GLFW_KEY_S]) {
            screen = play;
            gameStarted = true;
            startTime = glfwGetTime();
        }
    }

    // Mouse position is inverted because the origin of the window is in the top left corner
    MouseY = height - MouseY; // Invert y-axis of mouse position

    bool mousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    // Hint: look at the color objects declared at the top of this file
    if (screen == play) {
        for (int i = 0; i < light.size(); i++) {
            if (light[i]->isOverlapping(vec2(MouseX, MouseY))) {
                border->setPos(vec2{light[i]->getPosX(),light[i]->getPosY()});
                if (mousePressedLastFrame && !mousePressed) {
                    clickedLights++;
                    if (light[i]->getRight() == 695) {
                        surrounding.push_back(i-1);
                        surrounding.push_back(i+5);
                        surrounding.push_back(i-5);
                    } else if (light[i]->getRight() == 195) {
                        surrounding.push_back(i+1);
                        surrounding.push_back(i+5);
                        surrounding.push_back(i-5);
                    } else {
                        surrounding.push_back(i+1);
                        surrounding.push_back(i-1);
                        surrounding.push_back(i+5);
                        surrounding.push_back(i-5);
                    }
                    for (int j = 0; j < surrounding.size(); j++) {
                        int index = surrounding[j];
                        if (index >= 0 && index < 25) {
                            if (light[index]->getBlue() == 0.0) {
                                light[index]->setColor(gray);
                            } else {
                                light[index]->setColor(yellow);
                            }
                        }
                    }
                    if(light[i]->getBlue() == 0.0) {

                        light[i]->setColor(gray);
                    } else {
                        light[i]->setColor(yellow);
                    }
                    surrounding.clear();
                }
            }

        }

        int count = 0;
        for (int i = 0; i < light.size(); i++) {
            if (light[i]->getBlue() == .5) {
                count++;
                if (count == 25) {
                    screen = over;
                }
            }
        }
        mousePressedLastFrame = mousePressed;


    }


}

void Engine::update() {
    // Calculate delta time
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    if (gameStarted && screen == over && endTime == 0) {
        endTime = currentFrame;
    }
}



void Engine::render() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Set background color
    glClear(GL_COLOR_BUFFER_BIT);

    // Set shader to use for all shapes
    shapeShader.use();
    // Render differently depending on screen

    double timeTaken = endTime - startTime;
    stringstream timeSS;
    timeSS << "Time taken: " << timeTaken << " seconds";
    string timeStr = timeSS.str();


    switch (screen) {
        case start: {
            string m1 = "Welcome to Lights Out!";
            string m2 = "Turn off all the lights to win";
            string m3 = "Press 'S' to start";
            fontRenderer->renderText(m1, width/2 - (12 * m1.length()), 350, 1, vec3{1, 1, 1});
            fontRenderer->renderText(m2, width/2 - (12 * m2.length()), 300, 1, vec3{1, 1, 1});
            fontRenderer->renderText(m3, width/2 - (12 * m3.length()), 250, 1, vec3{1, 1, 1});
            break;
        }
        case play: {
            border->setUniforms();
            border->draw();
            for (int i = 0; i < light.size(); i++) {
                light[i]->setUniforms();
                light[i]->draw();
            }
            int count = 0;
            for (int i = 0; i < light.size(); i++) {
                if (light[i]->getBlue() == .5) {
                    count++;
                }
            }
            stringstream ss;
            ss << clickedLights;
            string str = ss.str();
            fontRenderer->renderText("Lights clicked: " + str + "/25", width/2 - 240, 550, 1, vec3{1, 1, 1});
            break;
        }
        case over: {
            string message = "You win!";
            for (int i = 0; i < light.size(); i++) {
                light[i]->setUniforms();
                light[i]->draw();
            }
            fontRenderer->renderText(timeStr, width/2 - (12 * timeStr.length()), height/2 - 50, 1, vec3{1, 1, 1});
            fontRenderer->renderText(message, width/2 - (12 * message.length()), height/2, 1, vec3{1, 1, 1});
        }
    }

    glfwSwapBuffers(window);
}

bool Engine::shouldClose() {
    return glfwWindowShouldClose(window);
}

GLenum Engine::glCheckError_(const char *file, int line) {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        string error;
        switch (errorCode) {
            case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        cout << error << " | " << file << " (" << line << ")" << endl;
    }
    return errorCode;
}
