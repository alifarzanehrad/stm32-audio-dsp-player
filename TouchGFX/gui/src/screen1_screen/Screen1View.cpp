#include <gui/screen1_screen/Screen1View.hpp>
#include <touchgfx/Color.hpp>

extern "C"
{
    extern float fftBandsSmoothed[16];
}

Screen1View::Screen1View()
    : fftTickCounter(0),
      playPauseButtonCallback(this, &Screen1View::playPauseButtonClicked),
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

    const int16_t startX = 40;
    const int16_t bottomY = 125;

    const int16_t barWidth = 18;
    const int16_t gap = 5;

    for (uint8_t i = 0; i < FFT_BAR_COUNT; i++)
    {
        fftBars[i].setColor(touchgfx::Color::getColorFromRGB(0, 200, 255));

        fftBars[i].setPosition(
            startX + i * (barWidth + gap),
            bottomY - 2,
            barWidth,
            2
        );

        add(fftBars[i]);
    }
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

void Screen1View::handleTickEvent()
{
    fftTickCounter++;

    if (fftTickCounter < 3)
    {
        return;
    }

    fftTickCounter = 0;

    const int16_t bottomY = 125;
    const int16_t maxHeight = 100;

    for (uint8_t i = 0; i < FFT_BAR_COUNT; i++)
    {
        float value = fftBandsSmoothed[i];

        if (value < 0.0f)
        {
            value = 0.0f;
        }

        if (value > 100.0f)
        {
            value = 100.0f;
        }

        int16_t height =
            (int16_t)((value / 100.0f) * maxHeight);

        if (height < 2)
        {
            height = 2;
        }

        fftBars[i].invalidate();

        fftBars[i].setY(bottomY - height);
        fftBars[i].setHeight(height);

        fftBars[i].invalidate();
    }
}
