#include "buzzer.h"
#include <AL/al.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void checkError();

Buzzer::Buzzer() {
  ALuint buffer;
  ALuint samples[NUM_SAMPLES];

  // Generate Tone
  for (int i = 0; i < NUM_SAMPLES; i++) {
    float t = (float)i / SAMPLE_RATE;
    samples[i] = 16383 * ((sinf(2.0f * M_PI * FREQUENCY * t)) >= 0 ? 1.0f : -1.0f);
  }

  // OpenAL
  device = alcOpenDevice(NULL);
  if (!device) {
    printf("Failed to open device\n");
    exit(EXIT_FAILURE);
  }
  context = alcCreateContext(device, NULL);
  alcMakeContextCurrent(context);
  checkError();
  alGenBuffers(1, &buffer);
  checkError();
  alBufferData(buffer, AL_FORMAT_MONO16, samples, NUM_SAMPLES * sizeof(int), SAMPLE_RATE);
  alGenSources(1, &source);
  checkError();
  alSourcei(source, AL_BUFFER, buffer);
  checkError();
}

void checkError() {
  ALenum error;
  if ((error = alGetError()) != AL_NO_ERROR) {
    printf("OpenAL Error:\n");
    switch (error) {
      case ALC_INVALID_DEVICE:
        printf("Invalid Device\n");
        break;
      case ALC_INVALID_CONTEXT:
        printf("Invalid Context\n");
        break;
      case ALC_INVALID_ENUM:
        printf("Invalid Enum\n");
        break;
      case ALC_INVALID_VALUE:
        printf("Invalid Value\n");
        break;
      case ALC_OUT_OF_MEMORY:
        printf("Out of Memory\n");
        break;
    }
    exit(EXIT_FAILURE);
  }
}

void Buzzer::Play() {
  alSourcePlay(source);
}

void Buzzer::Stop() {
  alSourceStop(source);
  alSourceRewind(source);
}

Buzzer::~Buzzer() {
  alcMakeContextCurrent(NULL);
  alcDestroyContext(context);
  alcCloseDevice(device);
}
