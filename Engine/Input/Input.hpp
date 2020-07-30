#pragma once

#include <Core/EngineModule.hpp>
#include <windows.h>
#include <Core/Types.hpp>
#include <entt/entt.hpp>

struct GLFWwindow;

namespace Quark
{
    class Input : public EngineModule
    {
    public:
        Input(GLFWwindow* window);

        enum class KeyboardKeys
        {
            Tab = VK_TAB,
            Enter = VK_RETURN,
            Esc = VK_ESCAPE,
            Space = VK_SPACE,
            Left = VK_LEFT,
            Up = VK_UP,
            Right = VK_RIGHT,
            Down = VK_DOWN,
            LShift = VK_LSHIFT,
            Ctrl = VK_CONTROL,
            //Alt,

            A = 0x41,
            D = 0x44,
            S = 0x53,
            W = 0x57,

            Num_1 = VK_NUMPAD1,
            Num_2 = VK_NUMPAD2,
            Num_3 = VK_NUMPAD3,
            Num_4 = VK_NUMPAD4,
            Num_5 = VK_NUMPAD5,

            MaxKeys
        };

        enum State
        {
            Up,
            FirstDown,
            Down,
            FirstUp
        };

        enum class MouseKeys
        {
            LeftButton,
            RightButton,
            MiddleButton,
        };

        template <int N>
        struct BaseState
        {
            BaseState()
            {
                for (int key = 0; key < N; key++)
                {
                    m_keys[key] = State::Up;
                }
            }
            std::array<State, N> m_keys;
        };

        struct KeyboardState : BaseState<256>
        {
            State operator[] (KeyboardKeys key) const
            {
                return m_keys[(int)key];
            }
        };

        struct MouseState : BaseState<3>
        {
            State operator[] (MouseKeys key) const
            {
                return m_keys[(int)key];
            }
        };

        struct MouseMoveEvent
        {
            double mouseX = 0.0;
            double mouseY = 0.0;
        };

        struct MouseKeyEvent
        {
            MouseState* state;
        };

        struct KeyboardEvent
        {
            KeyboardState* state;
        };

        entt::dispatcher m_inputDispatcher;

        void update();

        virtual void init() override;
        virtual void deinit() override;
        virtual bool needsToRun() const override;
        virtual tf::Task scedule(tf::Taskflow& flow) override;

    private:
        friend void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
        friend void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
        KeyboardState m_keyboard;
        MouseState m_mouse;
        GLFWwindow* m_window;
    };
}