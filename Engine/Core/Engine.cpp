#include <iostream>

#include "EngineModule.hpp"
#include <GLFW/glfw3.h>
#include <taskflow/taskflow.hpp>

// tf::Task ret = flow.emplace([this](tf::Subflow& subflow) {
//     /*tf::Task B1 = */subflow.emplace([this]() { std::cout << m_name << std::endl; }).name("B1");
//     /*tf::Task B2 = */subflow.emplace([]() {std::cout << "Subtask2TestModule\n"; }).name("B2");
//     //tf::Task B3 = subflow.emplace([]() {}).name("B3");
//     //B1.precede(B3);
//     //B2.precede(B3);
// }).name(m_name);

class GameLogic : public Module
{
public:
    GameLogic()
        : Module("GameLogic")
    {}

    virtual bool needsToRun() const override { return true; }
    virtual tf::Task scedule(tf::Taskflow& flow) override
    {
        tf::Task ret = flow.emplace([]() {
            std::cout << "GameLogic\n";
        });
        return ret;
    }
};

class RenderLogic : public Module
{
public:
    RenderLogic()
        : Module("RenderLogic")
    {}

    virtual bool needsToRun() const override { return true; }
    virtual tf::Task scedule(tf::Taskflow& flow) override
    {
        tf::Task ret = flow.emplace([]() {
            std::cout << "RenderLogic\n";
        });
        return ret;
    }
};

int EngineMain(int argc, char** argv)
{
    GameLogic gameLogicTask;
    RenderLogic renderLogicTask;

    tf::Executor executor;
    tf::Taskflow jobGraph("EngineJobGraph");
    
    if (!glfwInit())
    {
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(640, 480, "Q", NULL, NULL);
    while (!glfwWindowShouldClose(window))
    {
        jobGraph.clear();

        glfwPollEvents();

        tf::Task gameLogicJob = gameLogicTask.scedule(jobGraph);
        tf::Task renderLogicJob = renderLogicTask.scedule(jobGraph);

        tf::Task beginFrameJob = jobGraph.emplace([]() { std::cout << "BeginFrame\n"; });
        tf::Task endFrameJob = jobGraph.emplace([]() { std::cout << "EndFrame\n"; });

        beginFrameJob.precede(gameLogicJob);
        gameLogicJob.precede(renderLogicJob);
        renderLogicJob.precede(endFrameJob);

        executor.run(jobGraph).wait();
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}