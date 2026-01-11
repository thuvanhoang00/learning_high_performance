#include "circle.h"
#include "external_drawing_lib.h"

int main(){
    OpenGLCirleDraw openGL_drawer;
    BoostCirleDraw boost_drawer;

    Circle<OpenGLCirleDraw> circle(openGL_drawer);
    circle.draw();

    Circle<BoostCirleDraw> circle2(boost_drawer);
    circle2.draw();

    return 0;
}