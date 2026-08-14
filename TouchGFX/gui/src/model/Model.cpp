#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

extern "C" void AudioPlayer_RequestPlay(void);
extern "C" void AudioPlayer_RequestPause(void);
extern "C" void AudioPlayer_RequestNext(void);
extern "C" void AudioPlayer_RequestPrevious(void);

Model::Model() : modelListener(0)
{

}

void Model::tick()
{

}

void Model::play()
{
    AudioPlayer_RequestPlay();
}

void Model::pause()
{
    AudioPlayer_RequestPause();
}

void Model::next()
{
    AudioPlayer_RequestNext();
}

void Model::previous()
{
    AudioPlayer_RequestPrevious();
}
