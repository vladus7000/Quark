#include <string>
#include <Core/Types.hpp>
#include <taskflow/taskflow.hpp>

class Module
{
public:
    Module(const std::string& name)
        : m_name(name)
    {}

    virtual bool needsToRun() const = 0;
    virtual tf::Task scedule(tf::Taskflow& flow) = 0;

protected:
    std::string m_name;
};
