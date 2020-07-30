#include "Input.hpp"
#include <GLFW/glfw3.h>

namespace
{
    Quark::Input* This = nullptr;
}

namespace Quark
{
    Input::Input(GLFWwindow* window)
        : EngineModule("Input")
        , m_window(window)
    {
    }

    void Input::update()
    {
        for (int key = 7; key < 256; key++)
        {
           State& currentKey = m_keyboard.m_keys[key];
            const short unsigned int currentState = GetAsyncKeyState(key);
            const bool pressed = (currentState & 0x8000) != 0;
            switch (currentKey)
            {
            case State::Up:
                if (pressed) currentKey = State::FirstDown;
                break;
            case State::FirstDown:
                if (pressed) currentKey = State::Down;
                else currentKey = State::FirstUp;
                break;
            case State::Down:
                if (!pressed) currentKey = State::FirstUp;
                break;
            case State::FirstUp:
                if (!pressed) currentKey = State::Up;
                else currentKey = State::FirstDown;
                break;
            default:
                break;
            }
        }

        double xpos, ypos;
        glfwGetCursorPos(m_window, &xpos, &ypos);

        m_inputDispatcher.trigger<KeyboardEvent>(&m_keyboard);

    }

    void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        {
            This->m_mouse.m_keys[0] = Input::State::Down;
        }
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        {
            This->m_mouse.m_keys[0] = Input::State::Up;
        }

        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        {
            This->m_mouse.m_keys[1] = Input::State::Down;
        }
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
        {
            This->m_mouse.m_keys[1] = Input::State::Up;
        }

        This->m_inputDispatcher.trigger<Input::MouseKeyEvent>(&This->m_mouse);
    }

    void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
    {
        This->m_inputDispatcher.trigger<Input::MouseMoveEvent>(xpos, ypos);
    }

    void Input::init()
    {
        This = this;
        //glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetMouseButtonCallback(m_window, mouse_button_callback);
        glfwSetCursorPosCallback(m_window, cursor_position_callback);
    }

    void Input::deinit()
    {
    }
    bool Input::needsToRun() const
    {
        return true;
    }

    tf::Task Input::scedule(tf::Taskflow & flow)
    {
        tf::Task ret = flow.emplace([this]() {
            update();
        });

        return ret;
    }
}