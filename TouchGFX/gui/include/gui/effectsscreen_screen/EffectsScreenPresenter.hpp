#ifndef EFFECTSSCREENPRESENTER_HPP
#define EFFECTSSCREENPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class EffectsScreenView;

class EffectsScreenPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    EffectsScreenPresenter(EffectsScreenView& v);

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

    void setEchoEnabled(bool enabled);
    bool getEchoEnabled() const;

    void setReverbEnabled(bool enabled);
    bool getReverbEnabled() const;

    virtual ~EffectsScreenPresenter() {}

private:
    EffectsScreenPresenter();

    EffectsScreenView& view;
};

#endif // EFFECTSSCREENPRESENTER_HPP
