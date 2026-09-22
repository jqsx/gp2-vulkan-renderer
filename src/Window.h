//
// Created by frane on 2/13/2026.
//
#pragma once

#ifndef WINDOW_H
#define WINDOW_H
#include "structs.h"
#include <GLFW/glfw3.h>

class Engine;

class Window final {
public:
private:
    GLFWwindow* m_WindowHandle;
    int m_Width, m_Height, m_X, m_Y;
    Engine* m_Engine;

    void StateToggle(int action, bool& state);

public:
    explicit Window(InitResult& success, const EngineConfig& config, Engine* engine);

    ~Window();

    Window(Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&) = delete;
    Window& operator=(Window&&) = delete;

    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    int GetX() const { return m_X; }
    int GetY() const { return m_Y; }

    GLFWwindow* GetWindowHandle() const { return m_WindowHandle; }

    void OnWindowSizeChanged(int width, int height);
    void OnWindowMoved(int x, int y);
    void OnFrameBufferSizeChanged(int width, int height);
    void OnKeyPressed(int key, int action);
    void OnMouseMove(double x, double y);
    void OnMouseButtonDown(int button, int action);

    void SetMouseLock(bool state);
};

#endif //WINDOW_H
