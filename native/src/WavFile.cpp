#define DR_WAV_IMPLEMENTATION
#include "../third_party/dr_wav.h"

#include "WavFile.h"

#include <stdexcept>
#include <string>
#include <vector>

StereoBuffer loadWavStereo(const std::string& path){
  drwav wav;
  if(!drwav_init_file(&wav, path.c_str(), nullptr)){
    throw std::runtime_error("Could not open input WAV: " + path);
  }

  const drwav_uint64 totalFrames = wav.totalPCMFrameCount;
  if(totalFrames == 0){
    drwav_uninit(&wav);
    throw std::runtime_error("Input WAV has zero frames: " + path);
  }

  std::vector<float> interleaved(static_cast<std::size_t>(totalFrames * wav.channels));
  const drwav_uint64 framesRead = drwav_read_pcm_frames_f32(&wav, totalFrames, interleaved.data());
  drwav_uninit(&wav);

  StereoBuffer buffer;
  buffer.sampleRate = static_cast<uint32_t>(wav.sampleRate);
  buffer.left.resize(static_cast<std::size_t>(framesRead));
  buffer.right.resize(static_cast<std::size_t>(framesRead));

  for(drwav_uint64 i=0;i<framesRead;i++){
    if(wav.channels == 1){
      const float s = interleaved[static_cast<std::size_t>(i)];
      buffer.left[static_cast<std::size_t>(i)] = s;
      buffer.right[static_cast<std::size_t>(i)] = s;
    }else{
      const std::size_t base = static_cast<std::size_t>(i * wav.channels);
      buffer.left[static_cast<std::size_t>(i)] = interleaved[base];
      buffer.right[static_cast<std::size_t>(i)] = interleaved[base + 1];
    }
  }

  return buffer;
}

void writeWavStereo(const std::string& path, const StereoBuffer& buffer){
  if(buffer.left.size() != buffer.right.size()){
    throw std::runtime_error("Left/right channel size mismatch when writing WAV: " + path);
  }

  drwav_data_format format{};
  format.container = drwav_container_riff;
  format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
  format.channels = 2;
  format.sampleRate = buffer.sampleRate;
  format.bitsPerSample = 32;

  drwav wav;
  if(!drwav_init_file_write(&wav, path.c_str(), &format, nullptr)){
    throw std::runtime_error("Could not open output WAV: " + path);
  }

  const std::size_t frames = buffer.left.size();
  std::vector<float> interleaved(frames * 2);
  for(std::size_t i=0;i<frames;i++){
    interleaved[2 * i] = buffer.left[i];
    interleaved[2 * i + 1] = buffer.right[i];
  }

  drwav_write_pcm_frames(&wav, frames, interleaved.data());
  drwav_uninit(&wav);
}

