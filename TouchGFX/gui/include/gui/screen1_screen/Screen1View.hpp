#ifndef SCREEN1VIEW_HPP
#define SCREEN1VIEW_HPP

#include <gui_generated/screen1_screen/Screen1ViewBase.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>
#include <touchgfx/Callback.hpp>

class Screen1View : public Screen1ViewBase
{
public:
    Screen1View();
    virtual ~Screen1View() {}

    virtual void setupScreen();
    virtual void tearDownScreen();

protected:

    touchgfx::Callback<Screen1View, const touchgfx::AbstractButton&> playPauseButtonCallback;
    touchgfx::Callback<Screen1View, const touchgfx::AbstractButton&> nextButtonCallback;
    touchgfx::Callback<Screen1View, const touchgfx::AbstractButton&> previousButtonCallback;
    touchgfx::Callback<Screen1View, const touchgfx::AbstractButton&> volumeUpButtonCallback;
    touchgfx::Callback<Screen1View, const touchgfx::AbstractButton&> volumeDownButtonCallback;

    void playPauseButtonClicked(const touchgfx::AbstractButton& source);
    void nextButtonClicked(const touchgfx::AbstractButton& source);
    void previousButtonClicked(const touchgfx::AbstractButton& source);
    void volumeUpButtonClicked(const touchgfx::AbstractButton& source);
    void volumeDownButtonClicked(const touchgfx::AbstractButton& source);
};

#endif
