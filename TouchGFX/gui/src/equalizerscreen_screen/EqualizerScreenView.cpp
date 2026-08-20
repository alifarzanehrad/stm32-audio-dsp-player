#include <gui/equalizerscreen_screen/EqualizerScreenView.hpp>
#include <touchgfx/Unicode.hpp>

EqualizerScreenView::EqualizerScreenView()
{
}

void EqualizerScreenView::setupScreen()
{
    EqualizerScreenViewBase::setupScreen();

    Slider_100Hz.setValue(presenter->getEQSliderValue(Model::EQ_BAND_100HZ));
    Slider_300Hz.setValue(presenter->getEQSliderValue(Model::EQ_BAND_300HZ));
    Slider_1kHz.setValue(presenter->getEQSliderValue(Model::EQ_BAND_1KHZ));
    Slider_3kHz.setValue(presenter->getEQSliderValue(Model::EQ_BAND_3KHZ));
    Slider_8kHz.setValue(presenter->getEQSliderValue(Model::EQ_BAND_8KHZ));

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
    presenter->setEQSliderValue(Model::EQ_BAND_100HZ, value);
}

void EqualizerScreenView::slider300HzChanged(int value)
{
    updateGainText(
        GainValue_300Hz,
        GainValue_300HzBuffer,
        GAINVALUE_300HZ_SIZE,
        value
    );
    presenter->setEQSliderValue(Model::EQ_BAND_300HZ, value);
}

void EqualizerScreenView::slider1kHzChanged(int value)
{
    updateGainText(
        GainValue_1kHz,
        GainValue_1kHzBuffer,
        GAINVALUE_1KHZ_SIZE,
        value
    );
    presenter->setEQSliderValue(Model::EQ_BAND_1KHZ, value);
}

void EqualizerScreenView::slider3kHzChanged(int value)
{
    updateGainText(
        GainValue_3kHz,
        GainValue_3kHzBuffer,
        GAINVALUE_3KHZ_SIZE,
        value
    );
    presenter->setEQSliderValue(Model::EQ_BAND_3KHZ, value);
}

void EqualizerScreenView::slider8kHzChanged(int value)
{
    updateGainText(
        GainValue_8kHz,
        GainValue_8kHzBuffer,
        GAINVALUE_8KHZ_SIZE,
        value
    );
    presenter->setEQSliderValue(Model::EQ_BAND_8KHZ, value);
}

void EqualizerScreenView::flatButtonClicked()
{
    Slider_100Hz.setValue(12);
    Slider_300Hz.setValue(12);
    Slider_1kHz.setValue(12);
    Slider_3kHz.setValue(12);
    Slider_8kHz.setValue(12);

    slider100HzChanged(12);
    slider300HzChanged(12);
    slider1kHzChanged(12);
    slider3kHzChanged(12);
    slider8kHzChanged(12);
}
