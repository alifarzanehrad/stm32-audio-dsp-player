#ifndef FRONTENDAPPLICATION_HPP
#define FRONTENDAPPLICATION_HPP

#include <gui_generated/common/FrontendApplicationBase.hpp>

class FrontendHeap;

using namespace touchgfx;

class FrontendApplication : public FrontendApplicationBase
{
public:
    FrontendApplication(Model& m, FrontendHeap& heap);
    virtual ~FrontendApplication() { }

    void gotoPlayerScreen();
    void gotoEqualizerScreen();
    void gotoEffectsScreen();
    void gotoInfoScreen();

    virtual void handleTickEvent()
    {
        model.tick();
        FrontendApplicationBase::handleTickEvent();
    }
private:
    touchgfx::Callback<FrontendApplication> customTransitionCallback;

    void gotoPlayerScreenImpl();
    void gotoEqualizerScreenImpl();
    void gotoEffectsScreenImpl();
    void gotoInfoScreenImpl();
};

#endif // FRONTENDAPPLICATION_HPP
