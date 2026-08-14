#include <gui/screen1_screen/Screen1View.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>

Screen1Presenter::Screen1Presenter(Screen1View& v)
    : view(v)
{

}

void Screen1Presenter::activate()
{

}

void Screen1Presenter::deactivate()
{

}

void Screen1Presenter::play()
{
    model->play();
}

void Screen1Presenter::pause()
{
    model->pause();
}

void Screen1Presenter::next()
{
    model->next();
}

void Screen1Presenter::previous()
{
    model->previous();
}
