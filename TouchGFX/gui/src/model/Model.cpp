#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

extern "C" void AudioPlayer_RequestPlayPause(void);
extern "C" void AudioPlayer_RequestNext(void);
extern "C" void AudioPlayer_RequestPrevious(void);
extern "C" void AudioPlayer_RequestVolumeUp(void);
extern "C" void AudioPlayer_RequestVolumeDown(void);
extern "C" void AudioEQ_SetBandGain(uint8_t band, float gainDB);

Model::Model() : modelListener(0)
{
    for (uint8_t band = 0; band < EQ_BAND_COUNT; band++)
    {
        eqSliderValues[band] = 12;
    }
}

void Model::tick()
{

}

void Model::playPause()
{
    AudioPlayer_RequestPlayPause();
}

void Model::next()
{
    AudioPlayer_RequestNext();
}

void Model::previous()
{
    AudioPlayer_RequestPrevious();
}

void Model::volumeUp()
{
    AudioPlayer_RequestVolumeUp();
}

void Model::volumeDown()
{
    AudioPlayer_RequestVolumeDown();
}

void Model::setEQSliderValue(uint8_t band, int value)
{
    if (band >= EQ_BAND_COUNT)
    {
        return;
    }

    if (value < 0)
    {
        value = 0;
    }
    else if (value > 24)
    {
        value = 24;
    }

    eqSliderValues[band] = value;
    AudioEQ_SetBandGain(band, (float)value - 12.0f);
}

int Model::getEQSliderValue(uint8_t band) const
{
    if (band >= EQ_BAND_COUNT)
    {
        return 12;
    }

    return eqSliderValues[band];
}
