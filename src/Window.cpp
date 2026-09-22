//
// Created by frane on 2/13/2026.
//

#include "Window.h"

#include "Camera.h"
#include "Engine.h"
#include "Logger.h"

void Window::StateToggle(int action, bool &state) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
        state = true;
    else if (action == GLFW_RELEASE)
        state = false;
}

Window::Window(InitResult& success, const EngineConfig &config, Engine* engine) : m_WindowHandle(nullptr), m_Width(0), m_Height(0), m_Engine(engine) {
    success = INIT_NINIT;

    if (!glfwInit()) {
        Logger::GetInstance().err("Failed to initialize GLFW.");
        success = INIT_FAIL;
        return;
    }

    if (!glfwVulkanSupported()) {
        Logger::GetInstance().err("Failed to locate vulkan loader for GLFW.");
        success = INIT_FAIL;
        return;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    m_WindowHandle = glfwCreateWindow(config.width, config.height, config.title, NULL, NULL);

    if (!m_WindowHandle) {
        Logger::GetInstance().err("Failed to initialize window, terminating.", __LINE__, __FILE__);

        glfwTerminate();
        success = INIT_FAIL;
        return;
    }

    m_Width = config.width;
    m_Height = config.height;

    glfwSetWindowUserPointer(m_WindowHandle, this);

    glfwSetWindowSizeCallback(m_WindowHandle, [](GLFWwindow* handle, int width, int height) {
        Window* window = (Window*)glfwGetWindowUserPointer(handle);

        window->OnWindowSizeChanged(width, height);
    });
    glfwSetWindowPosCallback(m_WindowHandle, [](GLFWwindow* handle, int x, int y) {
        Window* window = (Window*)glfwGetWindowUserPointer(handle);
        window->OnWindowMoved(x, y);
    });
    glfwSetFramebufferSizeCallback(m_WindowHandle, [](GLFWwindow* handle, int width, int height) {
        Window* window = (Window*)glfwGetWindowUserPointer(handle);
        window->OnFrameBufferSizeChanged(width, height);
    });
    glfwSetKeyCallback(m_WindowHandle, [](GLFWwindow* handle, int key, int , int action, int) {
        Window* window = (Window*)glfwGetWindowUserPointer(handle);
        window->OnKeyPressed(key, action);
    });
    glfwSetMouseButtonCallback(m_WindowHandle, [](GLFWwindow* handle, int button, int action, int ) {
        Window* window = (Window*)glfwGetWindowUserPointer(handle);
        window->OnMouseButtonDown(button, action);
    });
    glfwSetCursorPosCallback(m_WindowHandle, [](GLFWwindow* handle, double x, double y) {
        Window* window = (Window*)glfwGetWindowUserPointer(handle);
        window->OnMouseMove(x, y);
    });

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    glfwSetWindowPos(m_WindowHandle, mode->width / 2 - m_Width / 2, mode->height / 2 - m_Height / 2);

    glfwGetWindowPos(m_WindowHandle, &m_X, &m_Y);

    success = INIT_SUCCESS;
}

Window::~Window() {
    if (m_WindowHandle)
        glfwDestroyWindow(m_WindowHandle);
    glfwTerminate();
}

void Window::OnWindowSizeChanged(int width, int height) {
    m_Width = width;
    m_Height = height;
}

void Window::OnWindowMoved(int x, int y) {
    m_X = x;
    m_Y = y;
}

void Window::OnFrameBufferSizeChanged(int width, int height) {
    m_Engine->OnFrameBufferResize(width, height);
}

void Window::OnKeyPressed(int key, int action) {
    CameraInput& input = Camera::GetInstance().input;
    if (key == GLFW_KEY_W)
        StateToggle(action, input.keyW);
    if (key == GLFW_KEY_A)
        StateToggle(action, input.keyA);
    if (key == GLFW_KEY_D)
        StateToggle(action, input.keyD);
    if (key == GLFW_KEY_S)
        StateToggle(action, input.keyS);
    if (key == GLFW_KEY_SPACE)
        StateToggle(action, input.keySpace);
    if (key == GLFW_KEY_C)
        StateToggle(action, input.keyC);
}

void Window::OnMouseMove(double x, double y) {
    CameraInput& input = Camera::GetInstance().input;

    input.mouseDeltaX = x - input.cursorX;
    input.mouseDeltaY = y - input.cursorY;

    input.cursorX = x;
    input.cursorY = y;
}

void Window::OnMouseButtonDown(int button, int action) {
    CameraInput& input = Camera::GetInstance().input;
    if (button == GLFW_MOUSE_BUTTON_LEFT)
        StateToggle(action, input.leftButton);
    if (button == GLFW_MOUSE_BUTTON_RIGHT)
        StateToggle(action, input.rightButton);
}

void Window::SetMouseLock(bool state) {
    glfwSetInputMode(m_WindowHandle, GLFW_CURSOR, state ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}
