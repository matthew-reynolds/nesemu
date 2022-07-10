#include <nesemu/logger.h>
#include <nesemu/ui/audio.h>

#include <cstdint>

// Trampoline
extern "C" {
void audioCallback(void* userdata, uint8_t* stream, int len) {
  return static_cast<ui::Audio*>(userdata)->audioCallback(stream, len);
}
}

bool ui::Audio::init() {

  SDL_AudioSpec desired_spec;
  desired_spec.freq     = OUTPUT_C;
  desired_spec.format   = OUTPUT_FORMAT;
  desired_spec.channels = OUTPUT_CHANNELS;
  desired_spec.samples  = OUTPUT_SAMPLES;
  desired_spec.callback = nullptr;
  // desired_spec.callback = &::audioCallback;
  // desired_spec.userdata = this;

  // Attach audio device
  if (!(device_ = SDL_OpenAudioDevice(NULL, 0, &desired_spec, &audio_spec_, SDL_AUDIO_ALLOW_ANY_CHANGE))) {
    logger::log<logger::ERROR>("Couldn't open audio device: %s\n", SDL_GetError());
    return 1;
  }

  // Create downsamplers
  if (!(downsampler_a_ = SDL_NewAudioStream(AUDIO_U8, 1, FREQ_A, AUDIO_U8, 1, 1))) {
    logger::log<logger::ERROR>("Couldn't open create audio stream: %s\n", SDL_GetError());
    return 1;
  }
  if (!(downsampler_b_ = SDL_NewAudioStream(AUDIO_U8, 1, FREQ_B, AUDIO_U8, 1, audio_spec_.freq))) {
    logger::log<logger::ERROR>("Couldn't open create audio stream: %s\n", SDL_GetError());
    return 1;
  }

  // Start playback
  SDL_PauseAudioDevice(device_, 0);

  return 0;
}

void ui::Audio::close() {
  SDL_CloseAudioDevice(device_);
  SDL_FreeAudioStream(downsampler_a_);
  SDL_FreeAudioStream(downsampler_b_);
}

void ui::Audio::update(uint8_t* stream, size_t len) {

  // Upscale by 11 and feed the first downsampler
  for (size_t i = 0; i < len; i++) {
    for (size_t j = 0; j < 11; j++) {
      if (0 != SDL_AudioStreamPut(downsampler_a_, stream + i, 1)) {
        logger::log<logger::ERROR>("Failed to put samples in first downsampler: %s\n", SDL_GetError());
      }
    }
  }

  // When the first downsampler has output, feed the second downsampler
  uint8_t sample = 0;
  while (SDL_AudioStreamGet(downsampler_a_, &sample, 1)) {
    if (0 != SDL_AudioStreamPut(downsampler_b_, &sample, 1)) {
      logger::log<logger::ERROR>("Failed to put samples in second downsampler: %s\n", SDL_GetError());
    }
  }

  // When the second downsampler has output, feed the output device
  while (SDL_AudioStreamGet(downsampler_b_, &sample, 1)) {
    sample *= volume_;
    if (0 != SDL_QueueAudio(device_, &sample, 1)) {
      logger::log<logger::ERROR>("Failed to output audio: %s\n", SDL_GetError());
    }
  }
}


void ui::Audio::audioCallback(uint8_t* stream, int len) {
  SDL_memset(stream, 0, len);

  static uint8_t buffer[4096];
  for (int i = 0; i < len; i += 128) {
    buffer[i] = 255;
  }

  SDL_MixAudio(stream, buffer, len, SDL_MIX_MAXVOLUME);
  logger::log<logger::ERROR>("len: %d\tstream[0]: %u\n", len, stream[0]);
}
