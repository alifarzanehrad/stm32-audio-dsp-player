#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

extern "C" void AudioPlayer_RequestPlayPause(void);
extern "C" void AudioPlayer_RequestNext(void);
extern "C" void AudioPlayer_RequestPrevious(void);
extern "C" void AudioPlayer_RequestVolumeUp(void);
extern "C" void AudioPlayer_RequestVolumeDown(void);

Model::Model() : modelListener(0)
{

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
