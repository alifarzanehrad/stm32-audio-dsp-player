#ifndef EFFECTSSCREENVIEW_HPP
#define EFFECTSSCREENVIEW_HPP

#include <gui_generated/effectsscreen_screen/EffectsScreenViewBase.hpp>
#include <gui/effectsscreen_screen/EffectsScreenPresenter.hpp>

class EffectsScreenView : public EffectsScreenViewBase
{
public:
    EffectsScreenView();
    virtual ~EffectsScreenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleGestureEvent(const touchgfx::GestureEvent& event);
    virtual void echoToggled();
    virtual void reverbToggled();
    virtual void noiseReductionToggled();

protected:
};

#endif // EFFECTSSCREENVIEW_HPP
