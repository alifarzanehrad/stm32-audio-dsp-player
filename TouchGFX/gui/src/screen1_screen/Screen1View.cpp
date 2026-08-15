#include <gui/screen1_screen/Screen1View.hpp>

Screen1View::Screen1View()
    : playPauseButtonCallback(this, &Screen1View::playPauseButtonClicked),
      nextButtonCallback(this, &Screen1View::nextButtonClicked),
      previousButtonCallback(this, &Screen1View::previousButtonClicked),
      volumeUpButtonCallback(this, &Screen1View::volumeUpButtonClicked),
      volumeDownButtonCallback(this, &Screen1View::volumeDownButtonClicked)
{
}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();

    Stop_button.setAction(playPauseButtonCallback);

    Next_button.setAction(nextButtonCallback);
    Previous_button.setAction(previousButtonCallback);

    VolumeUp_button.setAction(volumeUpButtonCallback);
    VolumeDown_button.setAction(volumeDownButtonCallback);
}

void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

void Screen1View::playPauseButtonClicked(const touchgfx::AbstractButton& source)
{
    presenter->playPause();
}

void Screen1View::nextButtonClicked(const touchgfx::AbstractButton& source)
{
    presenter->next();
}

void Screen1View::previousButtonClicked(const touchgfx::AbstractButton& source)
{
    presenter->previous();
}

void Screen1View::volumeUpButtonClicked(const touchgfx::AbstractButton& source)
{
    presenter->volumeUp();
}

void Screen1View::volumeDownButtonClicked(const touchgfx::AbstractButton& source)
{
    presenter->volumeDown();
}
