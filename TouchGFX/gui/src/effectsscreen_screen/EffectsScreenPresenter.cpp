#include <gui/effectsscreen_screen/EffectsScreenView.hpp>
#include <gui/effectsscreen_screen/EffectsScreenPresenter.hpp>

EffectsScreenPresenter::EffectsScreenPresenter(EffectsScreenView& v)
    : view(v)
{

}

void EffectsScreenPresenter::activate()
{

}

void EffectsScreenPresenter::deactivate()
{

}

void EffectsScreenPresenter::setEchoEnabled(bool enabled)
{
    model->setEchoEnabled(enabled);
}

bool EffectsScreenPresenter::getEchoEnabled() const
{
    return model->getEchoEnabled();
}

void EffectsScreenPresenter::setReverbEnabled(bool enabled)
{
    model->setReverbEnabled(enabled);
}

bool EffectsScreenPresenter::getReverbEnabled() const
{
    return model->getReverbEnabled();
}

void EffectsScreenPresenter::setNoiseReductionEnabled(bool enabled)
{
    model->setNoiseReductionEnabled(enabled);
}

bool EffectsScreenPresenter::getNoiseReductionEnabled() const
{
    return model->getNoiseReductionEnabled();
}
