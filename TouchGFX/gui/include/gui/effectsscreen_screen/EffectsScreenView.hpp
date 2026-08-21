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
    virtual void echoToggled();
    virtual void reverbToggled();

protected:
};

#endif // EFFECTSSCREENVIEW_HPP
