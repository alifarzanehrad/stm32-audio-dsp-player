#include <gui/effectsscreen_screen/EffectsScreenView.hpp>

EffectsScreenView::EffectsScreenView()
{

}

void EffectsScreenView::setupScreen()
{
    EffectsScreenViewBase::setupScreen();
    Echo_toggle.forceState(presenter->getEchoEnabled());
    Reverb_toggle.forceState(presenter->getReverbEnabled());
    NoiseReduction_toggle.forceState(
        presenter->getNoiseReductionEnabled()
    );
}

void EffectsScreenView::tearDownScreen()
{
    EffectsScreenViewBase::tearDownScreen();
}

void EffectsScreenView::handleGestureEvent(
    const touchgfx::GestureEvent& event
)
{
    if (event.getType() != touchgfx::GestureEvent::SWIPE_HORIZONTAL)
    {
        EffectsScreenViewBase::handleGestureEvent(event);
        return;
    }

    if (event.getVelocity() > 0)
    {
        application().gotoPlayerScreen();
    }
    else if (event.getVelocity() < 0)
    {
        application().gotoInfoScreen();
    }
}

void EffectsScreenView::echoToggled()
{
    presenter->setEchoEnabled(Echo_toggle.getState());
}

void EffectsScreenView::reverbToggled()
{
    presenter->setReverbEnabled(Reverb_toggle.getState());
}

void EffectsScreenView::noiseReductionToggled()
{
    presenter->setNoiseReductionEnabled(
        NoiseReduction_toggle.getState()
    );
}
