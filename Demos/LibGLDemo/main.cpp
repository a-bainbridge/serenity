#include <LibGL/Init.h>
#include <LibGUI/Application.h>
#include <LibGUI/Window.h>
#include <LibGUI/Painter.h>
#include <LibGUI/Widget.h>

class ExampleGLWidget final : public GUI::Widget {
    C_OBJECT(ExampleGLWidget)

    public:
    static constexpr int width = 640;
    static constexpr int height = 480;
    private:
    virtual void paint_event(GUI::PaintEvent&) override;
};

void ExampleGLWidget::paint_event(GUI::PaintEvent&) {
    /*
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();//load identity matrix
    
    glTranslatef(0.0f,0.0f,-4.0f);//move forward 4 units
    
    glColor3f(0.0f,0.0f,1.0f); //blue color
    
    glBegin(GL_TRIANGLES);//start drawing triangles
      glVertex3f(-1.0f,-0.1f,0.0f);//triangle one first vertex
      glVertex3f(-0.5f,-0.25f,0.0f);//triangle one second vertex
      glVertex3f(-0.75f,0.25f,0.0f);//triangle one third vertex
      //drawing a new triangle
      glVertex3f(0.5f,-0.25f,0.0f);//triangle two first vertex
      glVertex3f(1.0f,-0.25f,0.0f);//triangle two second vertex
      glVertex3f(0.75f,0.25f,0.0f);//triangle two third vertex
    glEnd();//end drawing of triangles
    */
    GL::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    update();
}

int main(int argc, char** argv) {
    auto app = GUI::Application::construct(argc, argv);
    auto window = GUI::Window::construct();
    window->set_double_buffering_enabled(true);
    window->set_resizable(false);
    window->set_title("LibGL Demo");
    window->resize(640, 480);
    window->show();
    GL::Init();
    auto context = GL::CreateContext(window);
    window->set_main_widget<ExampleGLWidget>();
    dbg() << "ctx " << context;
    return app->exec();
}