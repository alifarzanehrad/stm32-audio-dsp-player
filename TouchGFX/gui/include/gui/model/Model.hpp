#ifndef MODEL_HPP
#define MODEL_HPP

#include <stdint.h>

class ModelListener;

class Model
{
public:
    enum EqualizerBand
    {
        EQ_BAND_100HZ = 0,
        EQ_BAND_300HZ,
        EQ_BAND_1KHZ,
        EQ_BAND_3KHZ,
        EQ_BAND_8KHZ,
        EQ_BAND_COUNT
    };

    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();

    void playPause();
    void next();
    void previous();

    void volumeUp();
    void volumeDown();

    void setEQSliderValue(uint8_t band, int value);
    int getEQSliderValue(uint8_t band) const;

    void setEchoEnabled(bool enabled);
    bool getEchoEnabled() const;

protected:
    ModelListener* modelListener;

private:
    int eqSliderValues[EQ_BAND_COUNT];
    bool echoEnabled;
};

#endif // MODEL_HPP
