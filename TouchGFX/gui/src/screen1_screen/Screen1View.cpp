#include <gui/screen1_screen/Screen1View.hpp>

Screen1View::Screen1View()
    : playButtonCallback(this, &Screen1View::playButtonClicked),
      pauseButtonCallback(this, &Screen1View::pauseButtonClicked),
      nextButtonCallback(this, &Screen1View::nextButtonClicked),
      previousButtonCallback(this, &Screen1View::previousButtonClicked)
{
}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();

    Play_button.setAction(playButtonCallback);
    Stop_button.setAction(pauseButtonCallback);
    Next_button.setAction(nextButtonCallback);
    Previous_button.setAction(previousButtonCallback);
}

void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

void Screen1View::playButtonClicked(const touchgfx::AbstractButton& source)
{
    presenter->play();
}
void Screen1View::pauseButtonClicked(const touchgfx::AbstractButton& source)
{
    presenter->pause();
}

void Screen1View::nextButtonClicked(const touchgfx::AbstractButton& source)
{
    presenter->next();
}

void Screen1View::previousButtonClicked(const touchgfx::AbstractButton& source)
{
    presenter->previous();
}
