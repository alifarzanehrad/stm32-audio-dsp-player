#ifndef EQUALIZERSCREENPRESENTER_HPP
#define EQUALIZERSCREENPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>
#include <stdint.h>

using namespace touchgfx;

class EqualizerScreenView;

class EqualizerScreenPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    EqualizerScreenPresenter(EqualizerScreenView& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    void setEQSliderValue(uint8_t band, int value);
    int getEQSliderValue(uint8_t band) const;

    virtual ~EqualizerScreenPresenter() {}

private:
    EqualizerScreenPresenter();

    EqualizerScreenView& view;
};

#endif // EQUALIZERSCREENPRESENTER_HPP
