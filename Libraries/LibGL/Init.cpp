#include <LibGL/Init.h>
#include <AK/LogStream.h>
#include <LibGUI/Window.h>
#include <LibGUI/WindowServerConnection.h>
#include <WindowServer/WindowClientEndpoint.h>
#include <WindowServer/WindowServerEndpoint.h>
#include <serenity.h>

namespace GL {

void Init() {
    dbg() << "Hello world!";
}

GLContext CreateContext(GUI::Window& window) {
    dbg() << "Making context for " << window.window_id();
    auto response = GUI::WindowServerConnection::the().send_sync<Messages::WindowServer::CreateGLContext>(window.window_id());
    return response->context_id();
}

}