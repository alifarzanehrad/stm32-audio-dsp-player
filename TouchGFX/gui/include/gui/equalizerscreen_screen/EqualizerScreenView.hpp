#ifndef EQUALIZERSCREENVIEW_HPP
#define EQUALIZERSCREENVIEW_HPP

#include <gui_generated/equalizerscreen_screen/EqualizerScreenViewBase.hpp>
#include <gui/equalizerscreen_screen/EqualizerScreenPresenter.hpp>

class EqualizerScreenView : public EqualizerScreenViewBase
{
public:
    EqualizerScreenView();
    virtual ~EqualizerScreenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // EQUALIZERSCREENVIEW_HPP
