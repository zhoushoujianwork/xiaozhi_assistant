#ifndef __AUDIO_CFG_H__
#define __AUDIO_CFG_H__
#define SAMPLE_RATE 16000
#define BUFFER_SAMPLES 960  //60ms
#define BITS_PER_SAMPLE 16
#define CHANNELS_NUM 1

#define OPUS_OUT_BUFFER_SIZE 1276  // 1276 bytes is recommended by opus_encode
#define OPUS_ENCODER_BITRATE 30000
#define OPUS_ENCODER_COMPLEXITY 0
#endif // __AUDIO_CFG_H__