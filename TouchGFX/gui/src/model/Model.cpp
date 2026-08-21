#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

extern "C"
{
    void AudioPlayer_RequestPlayPause(void);
    void AudioPlayer_RequestNext(void);
    void AudioPlayer_RequestPrevious(void);
    void AudioPlayer_RequestVolumeUp(void);
    void AudioPlayer_RequestVolumeDown(void);
    void AudioEQ_SetBandGain(uint8_t band, float gainDB);
    void AudioEcho_SetEnabled(uint8_t enabled);
    uint8_t AudioEcho_IsEnabled(void);
    void AudioReverb_SetEnabled(uint8_t enabled);
    uint8_t AudioReverb_IsEnabled(void);
    void AudioNoiseReduction_SetEnabled(uint8_t enabled);
    uint8_t AudioNoiseReduction_IsEnabled(void);
}

#ifdef SIMULATOR

extern "C" void AudioPlayer_RequestPlayPause(void)
{
}

extern "C" void AudioPlayer_RequestNext(void)
{
}

extern "C" void AudioPlayer_RequestPrevious(void)
{
}

extern "C" void AudioPlayer_RequestVolumeUp(void)
{
}

extern "C" void AudioPlayer_RequestVolumeDown(void)
{
}

extern "C" void AudioEQ_SetBandGain(uint8_t band, float gainDB)
{
    (void)band;
    (void)gainDB;
}

extern "C" void AudioEcho_SetEnabled(uint8_t enabled)
{
    (void)enabled;
}

extern "C" uint8_t AudioEcho_IsEnabled(void)
{
    return 0U;
}

extern "C" void AudioReverb_SetEnabled(uint8_t enabled)
{
    (void)enabled;
}

extern "C" uint8_t AudioReverb_IsEnabled(void)
{
    return 0U;
}

extern "C" void AudioNoiseReduction_SetEnabled(uint8_t enabled)
{
    (void)enabled;
}

extern "C" uint8_t AudioNoiseReduction_IsEnabled(void)
{
    return 0U;
}

#endif

Model::Model()
    : modelListener(0),
      echoEnabled(AudioEcho_IsEnabled() != 0U),
      reverbEnabled(AudioReverb_IsEnabled() != 0U),
      noiseReductionEnabled(AudioNoiseReduction_IsEnabled() != 0U)
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

void Model::setEchoEnabled(bool enabled)
{
    echoEnabled = enabled;
    AudioEcho_SetEnabled(enabled ? 1U : 0U);
}

bool Model::getEchoEnabled() const
{
    return echoEnabled;
}

void Model::setReverbEnabled(bool enabled)
{
    reverbEnabled = enabled;
    AudioReverb_SetEnabled(enabled ? 1U : 0U);
}

bool Model::getReverbEnabled() const
{
    return reverbEnabled;
}

void Model::setNoiseReductionEnabled(bool enabled)
{
    noiseReductionEnabled = enabled;
    AudioNoiseReduction_SetEnabled(enabled ? 1U : 0U);
}

bool Model::getNoiseReductionEnabled() const
{
    return noiseReductionEnabled;
}
