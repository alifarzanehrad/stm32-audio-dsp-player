#include <gui/effectsscreen_screen/EffectsScreenView.hpp>

EffectsScreenView::EffectsScreenView()
{

}

void EffectsScreenView::setupScreen()
{
    EffectsScreenViewBase::setupScreen();
    Echo_toggle.forceState(presenter->getEchoEnabled());
    Reverb_toggle.forceState(presenter->getReverbEnabled());
}

void EffectsScreenView::tearDownScreen()
{
    EffectsScreenViewBase::tearDownScreen();
}

void EffectsScreenView::echoToggled()
{
    presenter->setEchoEnabled(Echo_toggle.getState());
}

void EffectsScreenView::reverbToggled()
{
    presenter->setReverbEnabled(Reverb_toggle.getState());
}
