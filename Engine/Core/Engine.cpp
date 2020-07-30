#include "Core/Engine.hpp"
#include <iostream>
#include <fstream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <taskflow/taskflow.hpp>
#include <nlohmann/json.hpp>
#include "Gameplay/GameplayModule.hpp"
#include "Renderer/RenderModule.hpp"
#include "Core/Components.hpp"
#include "Input/Input.hpp"

// tf::Task ret = flow.emplace([this](tf::Subflow& subflow) {
//     /*tf::Task B1 = */subflow.emplace([this]() { std::cout << m_name << std::endl; }).name("B1");
//     /*tf::Task B2 = */subflow.emplace([]() {std::cout << "Subtask2TestModule\n"; }).name("B2");
//     //tf::Task B3 = subflow.emplace([]() {}).name("B3");
//     //B1.precede(B3);
//     //B2.precede(B3);
// }).name(m_name);

Quark::Engine& Quark::GetEngine()
{
    static Quark::Engine engine;
    return engine;
}

void InputReciever(const Quark::Input::KeyboardEvent& e)
{
    if ((*e.state)[Quark::Input::KeyboardKeys::A] == Quark::Input::FirstDown)
    {
        std::cout << "A\n";
    }
    if ((*e.state)[Quark::Input::KeyboardKeys::D] == Quark::Input::FirstDown)
    {
        std::cout << "D\n";
    }
}

void MouseReciever(const Quark::Input::MouseKeyEvent& e)
{
    if ((*e.state)[Quark::Input::MouseKeys::LeftButton] == Quark::Input::Down)
    {
        std::cout << "LB\n";
    }
    if ((*e.state)[Quark::Input::MouseKeys::RightButton] == Quark::Input::Down)
    {
        std::cout << "RB\n";
    }
}

void MouseMoveReciever(const Quark::Input::MouseMoveEvent& e)
{
    std::cout << "Move\n";
}

int main(int argc, char** argv)
{
    if (!glfwInit())
    {
        return -1;
    }
    int w = 640;
    int h = 480;
    std::string windowName("Q");

    std::ifstream i("settings.json");
    if (i.is_open())
    {
        nlohmann::json j;
        i >> j;

        if (!j.empty())
        {
            if (j.contains("w"))
            {
                w = j["w"];
            }
            if (j.contains("h"))
            {
                h = j["h"];
            }
            if (j.contains("title"))
            {
                windowName = j["title"];
            }
        }
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(w, h, windowName.c_str(), NULL, NULL);

    GameplayModule gameplay;
    RenderModule render(window);
    Quark::Input input(window);
    input.m_inputDispatcher.sink<Quark::Input::KeyboardEvent>().connect<&InputReciever>();
    input.m_inputDispatcher.sink<Quark::Input::MouseKeyEvent>().connect<&MouseReciever>();
    input.m_inputDispatcher.sink<Quark::Input::MouseMoveEvent>().connect<&MouseMoveReciever>();

    auto gameplayComponent = Quark::GetEngine().m_modules.create();
    Quark::GetEngine().m_modules.emplace<Quark::GameplayComponent>(gameplayComponent, &gameplay);

    auto renderComponent = Quark::GetEngine().m_modules.create();
    Quark::GetEngine().m_modules.emplace<Quark::RenderComponent>(renderComponent, &render);
    {
        auto component = Quark::GetEngine().m_modules.create();
        Quark::GetEngine().m_modules.emplace<Quark::InputComponent>(component, &render);

    }
    render.init();
    gameplay.init();
    input.init();

    tf::Executor executor;
    tf::Taskflow jobGraph("EngineJobGraph");

    while (!glfwWindowShouldClose(window))
    {
        jobGraph.clear();

        glfwPollEvents();

        tf::Task gameLogicJob = gameplay.scedule(jobGraph);
        tf::Task renderLogicJob = render.scedule(jobGraph);
        tf::Task inputJob = input.scedule(jobGraph);

        tf::Task beginFrameJob = jobGraph.emplace([]() { /*std::cout << "BeginFrame\n";*/ });
        tf::Task endFrameJob = jobGraph.emplace([]() {/* std::cout << "EndFrame\n";*/ });

        beginFrameJob.precede(inputJob);
        inputJob.precede(gameLogicJob);
        gameLogicJob.precede(renderLogicJob);
        renderLogicJob.precede(endFrameJob);

        executor.run(jobGraph).wait();
    }

    input.deinit();
    gameplay.deinit();
    render.deinit();

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}