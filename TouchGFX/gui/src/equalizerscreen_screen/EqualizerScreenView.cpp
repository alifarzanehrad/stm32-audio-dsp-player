#include <gui/equalizerscreen_screen/EqualizerScreenView.hpp>
#include <touchgfx/Unicode.hpp>

EqualizerScreenView::EqualizerScreenView()
{
}

void EqualizerScreenView::setupScreen()
{
    EqualizerScreenViewBase::setupScreen();

    slider100HzChanged(Slider_100Hz.getValue());
    slider300HzChanged(Slider_300Hz.getValue());
    slider1kHzChanged(Slider_1kHz.getValue());
    slider3kHzChanged(Slider_3kHz.getValue());
    slider8kHzChanged(Slider_8kHz.getValue());
}

void EqualizerScreenView::tearDownScreen()
{
    EqualizerScreenViewBase::tearDownScreen();
}

void EqualizerScreenView::updateGainText(
    touchgfx::TextAreaWithOneWildcard& textArea,
    touchgfx::Unicode::UnicodeChar* buffer,
    uint16_t bufferSize,
    int sliderValue
)
{
    int gainDB = sliderValue - 12;

    if (gainDB > 0)
    {
        touchgfx::Unicode::snprintf(
            buffer,
            bufferSize,
            "+%d",
            gainDB
        );
    }
    else
    {
        touchgfx::Unicode::snprintf(
            buffer,
            bufferSize,
            "%d",
            gainDB
        );
    }

    textArea.invalidate();
}

void EqualizerScreenView::slider100HzChanged(int value)
{
    updateGainText(
        GainValue_100Hz,
        GainValue_100HzBuffer,
        GAINVALUE_100HZ_SIZE,
        value
    );
}

void EqualizerScreenView::slider300HzChanged(int value)
{
    updateGainText(
        GainValue_300Hz,
        GainValue_300HzBuffer,
        GAINVALUE_300HZ_SIZE,
        value
    );
}

void EqualizerScreenView::slider1kHzChanged(int value)
{
    updateGainText(
        GainValue_1kHz,
        GainValue_1kHzBuffer,
        GAINVALUE_1KHZ_SIZE,
        value
    );
}

void EqualizerScreenView::slider3kHzChanged(int value)
{
    updateGainText(
        GainValue_3kHz,
        GainValue_3kHzBuffer,
        GAINVALUE_3KHZ_SIZE,
        value
    );
}

void EqualizerScreenView::slider8kHzChanged(int value)
{
    updateGainText(
        GainValue_8kHz,
        GainValue_8kHzBuffer,
        GAINVALUE_8KHZ_SIZE,
        value
    );
}

void EqualizerScreenView::flatButtonClicked()
{
    Slider_100Hz.setValue(12);
    Slider_300Hz.setValue(12);
    Slider_1kHz.setValue(12);
    Slider_3kHz.setValue(12);
    Slider_8kHz.setValue(12);
}
