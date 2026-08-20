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

    void slider100HzChanged(int value) override;
    void slider300HzChanged(int value) override;
    void slider1kHzChanged(int value) override;
    void slider3kHzChanged(int value) override;
    void slider8kHzChanged(int value) override;

    void flatButtonClicked() override;

protected:
    void updateGainText(
        touchgfx::TextAreaWithOneWildcard& textArea,
        touchgfx::Unicode::UnicodeChar* buffer,
        uint16_t bufferSize,
        int sliderValue
    );
};

#endif // EQUALIZERSCREENVIEW_HPP
