#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <functional>

static void dispatch_main(void* fp)
{
	std::function<void()>* func = (std::function<void()>*)fp;
	(*func)();
}
#endif

//http://www.youtube.com/user/thecplusplusguy
//Playing 3D sound with OpenAL, and loading a wav file manually
#include <iostream>
#include <fstream>
#include <cstring>
#include <AL/al.h>
#include <AL/alc.h>

bool isBigEndian()
{
	int a = 1;
	return !((char*)&a)[0];
}

int convertToInt(char* buffer, int len)
{
	int a = 0;
	if (!isBigEndian())
		for (int i = 0; i < len; i++)
			((char*)&a)[i] = buffer[i];
	else
		for (int i = 0; i < len; i++)
			((char*)&a)[3 - i] = buffer[i];
	return a;
}

char* loadWAV(const char* path, int* channels, int* samplerate, int* bps, int* size, unsigned* alformat)
{
	char buffer[4];
	std::ifstream in(path, std::ios::binary);
	in.read(buffer, 4);
	if (strncmp(buffer, "RIFF", 4) != 0)
	{
		std::cout << "this is not a valid WAVE file" << std::endl;
		return NULL;
	}
	in.read(buffer, 4);
	in.read(buffer, 4);      //WAVE
	in.read(buffer, 4);      //fmt
	in.read(buffer, 4);      //16
	in.read(buffer, 2);      //1
	in.read(buffer, 2);
	*channels = convertToInt(buffer, 2);
	in.read(buffer, 4);
	*samplerate = convertToInt(buffer, 4);
	in.read(buffer, 4);
	in.read(buffer, 2);
	in.read(buffer, 2);
	*bps = convertToInt(buffer, 2);
	in.read(buffer, 4);      //data
	in.read(buffer, 4);
	*size = convertToInt(buffer, 4);
	char* data = new char[*size];
	in.read(data, *size);

	if (*channels == 1)
		*alformat = (*bps == 8 ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16);
	else 
		*alformat = (*bps == 8 ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16);	

	return data;
}

int main(int argc, char** argv)
{
	// Device Setup
	ALCdevice* device = alcOpenDevice(NULL);
	if (device == NULL)
	{
		std::cout << "cannot open sound card" << std::endl;
		return 0;
	}
	ALCcontext* context = alcCreateContext(device, NULL);
	if (context == NULL)
	{
		std::cout << "cannot open context" << std::endl;
		return 0;
	}
	alcMakeContextCurrent(context);
	// Loading WAV
	int channel, sampleRate, bps, size;
	unsigned format;
	char* data = loadWAV("loop.wav", &channel, &sampleRate, &bps, &size, &format);
	// Create Buffers
	unsigned bufferid;
	alGenBuffers(1, &bufferid);	
	alBufferData(bufferid, format, data, size, sampleRate);
	// Set up Listener
	ALfloat listenerOri[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	alListener3f(AL_POSITION, 0, 0, 1.0f);
	alListener3f(AL_VELOCITY, 0, 0, 0);
	alListenerfv(AL_ORIENTATION, listenerOri);
	// Create Source
	unsigned int sourceid;
	alGenSources(1, &sourceid);
	alSourcei(sourceid, AL_BUFFER, bufferid); // Bind Buffer
	alSourcei(sourceid, AL_LOOPING, AL_FALSE);
	alSourcef(sourceid, AL_PITCH, 1);
	alSourcef(sourceid, AL_GAIN, 1);
	alSource3f(sourceid, AL_POSITION, 0, 0, 0);
	alSource3f(sourceid, AL_VELOCITY, 0, 0, 0);
	alSourcePlay(sourceid);
	// Checking State

	ALint source_state;
	alGetSourcei(sourceid, AL_SOURCE_STATE, &source_state);
#ifdef __EMSCRIPTEN__
	std::function<void()> mainLoop = [&]() {
#else
	while (source_state == AL_PLAYING) {
#endif
		alGetSourcei(sourceid, AL_SOURCE_STATE, &source_state);
	}
#ifdef __EMSCRIPTEN__
	; emscripten_set_main_loop_arg(dispatch_main, &mainLoop, 0, 1);
#endif

	alDeleteSources(1, &sourceid);
	alDeleteBuffers(1, &bufferid);
	device = alcGetContextsDevice(context);
	alcMakeContextCurrent(NULL);
	alcDestroyContext(context);
	alcCloseDevice(device);
	delete[] data;

	return 0;
}