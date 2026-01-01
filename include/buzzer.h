#ifndef BUZZER_H
#define BUZZER_H

#include <AL/al.h>
#include <AL/alc.h>

#define SAMPLE_RATE 44100
#define FREQUENCY 220
#define NUM_SAMPLES SAMPLE_RATE

class Buzzer {
  private:
    ALuint source;
    ALCdevice *device;
    ALCcontext *context;

  public:
    Buzzer();
    ~Buzzer();
    void Play();
    void Stop();
};

#endif
