#include <gui/equalizerscreen_screen/EqualizerScreenView.hpp>
#include <gui/equalizerscreen_screen/EqualizerScreenPresenter.hpp>

EqualizerScreenPresenter::EqualizerScreenPresenter(EqualizerScreenView& v)
    : view(v)
{

}

void EqualizerScreenPresenter::activate()
{

}

void EqualizerScreenPresenter::deactivate()
{

}

void EqualizerScreenPresenter::setEQSliderValue(uint8_t band, int value)
{
    model->setEQSliderValue(band, value);
}

int EqualizerScreenPresenter::getEQSliderValue(uint8_t band) const
{
    return model->getEQSliderValue(band);
}
