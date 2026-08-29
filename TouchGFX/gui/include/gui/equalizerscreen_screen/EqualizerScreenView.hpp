#ifndef EQUALIZERSCREENVIEW_HPP
#define EQUALIZERSCREENVIEW_HPP

#include <gui_generated/equalizerscreen_screen/EqualizerScreenViewBase.hpp>
#include <gui/equalizerscreen_screen/EqualizerScreenPresenter.hpp>

class EqualizerScreenView : public EqualizerScreenViewBase
{
public:
    EqualizerScreenView();
    virtual ~EqualizerScreenView() {}

    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);
    virtual void handleGestureEvent(const touchgfx::GestureEvent& event);

    void slider100HzChanged(int value) override;
    void slider300HzChanged(int value) override;
    void slider1kHzChanged(int value) override;
    void slider3kHzChanged(int value) override;
    void slider8kHzChanged(int value) override;

    void flatButtonClicked() override;
    void function1() override;
    void function2() override;

protected:
    bool swipeStartedOnSlider;

    void updateGainText(
        touchgfx::TextAreaWithOneWildcard& textArea,
        touchgfx::Unicode::UnicodeChar* buffer,
        uint16_t bufferSize,
        int sliderValue
    );
    void applyPreset(const int values[Model::EQ_BAND_COUNT]);
};

#endif // EQUALIZERSCREENVIEW_HPP
