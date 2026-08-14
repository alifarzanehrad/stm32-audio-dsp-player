#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

extern "C" void AudioPlayer_RequestPlay(void);

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
