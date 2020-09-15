#pragma once
#include <LibGL/Types.h>
#include <LibGUI/Window.h>
namespace GL {
    void Init();
    GLContext CreateContext(GUI::Window&);
}