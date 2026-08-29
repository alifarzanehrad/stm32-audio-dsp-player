#include <gui/common/FrontendApplication.hpp>
#include <gui/common/FrontendHeap.hpp>
#include <gui/screen1_screen/Screen1View.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>
#include <gui/equalizerscreen_screen/EqualizerScreenView.hpp>
#include <gui/equalizerscreen_screen/EqualizerScreenPresenter.hpp>
#include <gui/effectsscreen_screen/EffectsScreenView.hpp>
#include <gui/effectsscreen_screen/EffectsScreenPresenter.hpp>
#include <gui/infoscreen_screen/InfoScreenView.hpp>
#include <gui/infoscreen_screen/InfoScreenPresenter.hpp>
#include <touchgfx/transitions/NoTransition.hpp>

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m, heap),
      customTransitionCallback()
{

}

void FrontendApplication::gotoPlayerScreen()
{
    customTransitionCallback = touchgfx::Callback<FrontendApplication>(
        this,
        &FrontendApplication::gotoPlayerScreenImpl
    );
    pendingScreenTransitionCallback = &customTransitionCallback;
}

void FrontendApplication::gotoEqualizerScreen()
{
    customTransitionCallback = touchgfx::Callback<FrontendApplication>(
        this,
        &FrontendApplication::gotoEqualizerScreenImpl
    );
    pendingScreenTransitionCallback = &customTransitionCallback;
}

void FrontendApplication::gotoEffectsScreen()
{
    customTransitionCallback = touchgfx::Callback<FrontendApplication>(
        this,
        &FrontendApplication::gotoEffectsScreenImpl
    );
    pendingScreenTransitionCallback = &customTransitionCallback;
}

void FrontendApplication::gotoInfoScreen()
{
    customTransitionCallback = touchgfx::Callback<FrontendApplication>(
        this,
        &FrontendApplication::gotoInfoScreenImpl
    );
    pendingScreenTransitionCallback = &customTransitionCallback;
}

void FrontendApplication::gotoPlayerScreenImpl()
{
    touchgfx::makeTransition<Screen1View, Screen1Presenter,
        touchgfx::NoTransition, Model>(
        &currentScreen,
        &currentPresenter,
        frontendHeap,
        &currentTransition,
        &model
    );
}

void FrontendApplication::gotoEqualizerScreenImpl()
{
    touchgfx::makeTransition<EqualizerScreenView, EqualizerScreenPresenter,
        touchgfx::NoTransition, Model>(
        &currentScreen,
        &currentPresenter,
        frontendHeap,
        &currentTransition,
        &model
    );
}

void FrontendApplication::gotoEffectsScreenImpl()
{
    touchgfx::makeTransition<EffectsScreenView, EffectsScreenPresenter,
        touchgfx::NoTransition, Model>(
        &currentScreen,
        &currentPresenter,
        frontendHeap,
        &currentTransition,
        &model
    );
}

void FrontendApplication::gotoInfoScreenImpl()
{
    touchgfx::makeTransition<InfoScreenView, InfoScreenPresenter,
        touchgfx::NoTransition, Model>(
        &currentScreen,
        &currentPresenter,
        frontendHeap,
        &currentTransition,
        &model
    );
}
