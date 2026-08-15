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

void Screen1Presenter::playPause()
{
    model->playPause();
}

void Screen1Presenter::next()
{
    model->next();
}

void Screen1Presenter::previous()
{
    model->previous();
}

void Screen1Presenter::volumeUp()
{
    model->volumeUp();
}

void Screen1Presenter::volumeDown()
{
    model->volumeDown();
}

