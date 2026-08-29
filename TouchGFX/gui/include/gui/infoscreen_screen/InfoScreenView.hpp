#ifndef INFOSCREENVIEW_HPP
#define INFOSCREENVIEW_HPP

#include <gui_generated/infoscreen_screen/InfoScreenViewBase.hpp>
#include <gui/infoscreen_screen/InfoScreenPresenter.hpp>

class InfoScreenView : public InfoScreenViewBase
{
public:
    InfoScreenView();
    virtual ~InfoScreenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void handleGestureEvent(const touchgfx::GestureEvent& event);
protected:
    uint16_t updateTickCounter;
    void updateMetrics();
};

#endif // INFOSCREENVIEW_HPP
