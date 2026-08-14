#include <gui/screen1_screen/Screen1View.hpp>

Screen1View::Screen1View()
    : playButtonCallback(this, &Screen1View::playButtonClicked)
{

}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();

    Play_button.setAction(playButtonCallback);
}

void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

void Screen1View::playButtonClicked(const touchgfx::AbstractButton& source)
{
    presenter->play();
}
