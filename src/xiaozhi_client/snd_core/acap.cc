// SPDX-License-Identifier: GPL-3.0-only
/*
 * Copyright (c) 2025, Canaan Bright Sight Co., Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "acap.h"
#include "capture_pcm.h"
#include "play_pcm.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <cstdlib>
#include <sys/time.h>
#include "opus/opus.h"
#include "audio_cfg.h"
#include "webrtc_audio_improvement.h"
#include "ipc_udp.h"

extern p_ipc_endpoint_t g_ipc_wakeup_detect_audio_ep;
extern int g_wakeup_word_start;

#define ENABLE_DEBUG_AUDIO 0
#define ENABLE_AUDIO_ENHANCEMENT 0
static pthread_t capture_thread, play_thread;
static char pcm_capture_buf[BUFFER_SAMPLES * BITS_PER_SAMPLE / 8];
static char pcm_paly_buf[BUFFER_SAMPLES * BITS_PER_SAMPLE / 8];
static int stop_threads = 0; // 用于控制线程的停止
static char pcm_out_buf[BUFFER_SAMPLES * BITS_PER_SAMPLE / 8];

static opus_int16 *output_buffer = NULL;
static OpusDecoder *opus_decoder = NULL;

static OpusEncoder *opus_encoder = NULL;
static char *encoder_input_buffer = NULL;
static uint8_t *encoder_output_buffer = NULL;
static acap_opus_data_callback g_opus_data_callback = NULL;

static FILE* pcm_speaker_file = NULL;
static FILE* pcm_mic_file = NULL;
static FILE* pcm_out_file = NULL;
static int init_file()
{
    pcm_speaker_file = fopen("speaker.pcm", "wb");
    if (!pcm_speaker_file) {
        printf("Failed to open speaker PCM file");
        return -1;
    }

    pcm_mic_file = fopen("mic.pcm", "wb");
    if (!pcm_mic_file) {
        printf("Failed to open mic PCM file");
        return -1;
    }

    pcm_out_file = fopen("out.pcm", "wb");
    if (!pcm_out_file) {
        printf("Failed to open out PCM file");
        return -1;
    }

    return 0;
}

static int deinit_file()
{
    if (pcm_speaker_file)
    {
        fclose(pcm_speaker_file);
        pcm_speaker_file = NULL;
    }

    if (pcm_mic_file)
    {
        fclose(pcm_mic_file);
        pcm_mic_file = NULL;
    }

    if (pcm_out_file)
    {
        fclose(pcm_out_file);
        pcm_out_file = NULL;
    }

    return 0;
}

static void init_audio_encoder() {
    int encoder_error;
    opus_encoder = opus_encoder_create(SAMPLE_RATE, 1, OPUS_APPLICATION_VOIP,
                                       &encoder_error);
    if (encoder_error != OPUS_OK) {
        printf("Failed to create OPUS encoder");
        return;
    }

    if (opus_encoder_init(opus_encoder, SAMPLE_RATE, 1, OPUS_APPLICATION_VOIP) != OPUS_OK) {
        printf("Failed to initialize OPUS encoder");
        return;
    }
    opus_encoder_ctl(opus_encoder, OPUS_SET_BITRATE(OPUS_ENCODER_BITRATE));
    opus_encoder_ctl(opus_encoder, OPUS_SET_COMPLEXITY(OPUS_ENCODER_COMPLEXITY));
    opus_encoder_ctl(opus_encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    encoder_input_buffer = (char *)malloc(BUFFER_SAMPLES * CHANNELS_NUM * BITS_PER_SAMPLE / 8);
    encoder_output_buffer = (uint8_t *)malloc(OPUS_OUT_BUFFER_SIZE);
    printf("%s\n", "init_audio_encoder");
}

static void init_audio_decoder(void) {
    int decoder_error = 0;
    opus_decoder = opus_decoder_create(SAMPLE_RATE, 1, &decoder_error);
    if (decoder_error != OPUS_OK) {
        printf("Failed to create OPUS decoder");
        return;
    }
    output_buffer = (opus_int16 *)malloc(BUFFER_SAMPLES * 2 * sizeof(opus_int16));
    printf("%s\n", "init_audio_decoder");
}

int play_opus_stream(const unsigned char* data, int size)
{
    int decoded_size = opus_decode(opus_decoder, data, size, output_buffer, BUFFER_SAMPLES, 0);
    {

        if (pcm_speaker_file != NULL)
            fwrite(output_buffer,1,decoded_size*2,pcm_speaker_file);

    }

    // 播放 PCM 数据
    play_pcm((char*)output_buffer);

#if ENABLE_AUDIO_ENHANCEMENT
    webrtc_process_reference_audio((char*)output_buffer,decoded_size*2);
#endif

    return 0;
}

// 采集音频的线程函数
static void* capture_thread_func(void* arg) {
    int ret = -1;
    while (!stop_threads) {
        // 采集 PCM 数据
        if (0 != capture_pcm(pcm_capture_buf)) {
            usleep(1000 * 1);
            continue;
        }

        if (g_wakeup_word_start)
        {
            continue;
        }

        g_ipc_wakeup_detect_audio_ep->send(g_ipc_wakeup_detect_audio_ep, pcm_capture_buf, BUFFER_SAMPLES * 2);

        if (pcm_mic_file != NULL)
            fwrite(pcm_capture_buf,1,BUFFER_SAMPLES*2,pcm_mic_file);

#if ENABLE_AUDIO_ENHANCEMENT
        ret = webrtc_enhance_audio_quality(pcm_capture_buf,(short*)pcm_out_buf);

#else
        ret = -1;
#endif

        if (ret == 0)
        {

            if (pcm_out_file != NULL)
                fwrite(pcm_out_buf,1,BUFFER_SAMPLES*2,pcm_out_file);

            // 编码音频数据并检查错误
            int encoded_size = opus_encode(opus_encoder, (opus_int16 *)pcm_out_buf,
                                          BUFFER_SAMPLES, encoder_output_buffer,
                                          OPUS_OUT_BUFFER_SIZE);
            if (encoded_size < 0) {
                printf("Opus encode error: %s\n", opus_strerror(encoded_size));
                continue;
            }
            if (g_opus_data_callback)
            {
                // 调用回调函数处理编码后的数据
                g_opus_data_callback(encoder_output_buffer, encoded_size);
            }

        }
        else
        {

            if (pcm_out_file != NULL)
                fwrite(pcm_capture_buf,1,BUFFER_SAMPLES*2,pcm_out_file);

            // 编码音频数据并检查错误
            int encoded_size = opus_encode(opus_encoder, (opus_int16 *)pcm_capture_buf,
                                          BUFFER_SAMPLES, encoder_output_buffer,
                                          OPUS_OUT_BUFFER_SIZE);
            if (encoded_size < 0) {
                printf("Opus encode error: %s\n", opus_strerror(encoded_size));
                continue;
            }
            if (g_opus_data_callback)
            {
                // 调用回调函数处理编码后的数据
                g_opus_data_callback(encoder_output_buffer, encoded_size);
            }
            //printf("Opus encode size: %d\n", encoded_size);

        }

    }
    printf("capture_thread_func end\n");

    return NULL;
}

void acap_init(acap_opus_data_callback callback) {

    g_opus_data_callback = callback;
#if ENABLE_AUDIO_ENHANCEMENT
    webrtc_audio_quality_init(SAMPLE_RATE,CHANNELS_NUM);
#endif

    init_capture_pcm(SAMPLE_RATE, CHANNELS_NUM, BUFFER_SAMPLES, BITS_PER_SAMPLE);
    init_play_pcm(SAMPLE_RATE, CHANNELS_NUM, BUFFER_SAMPLES, BITS_PER_SAMPLE);
    init_audio_encoder();
    init_audio_decoder();
#if ENABLE_DEBUG_AUDIO
    init_file();
#endif
}

void acap_deinit() {
    deinit_capture_pcm();
    deinit_play_pcm();
}

void acap_start() {

    // 创建采集音频的线程
    if (pthread_create(&capture_thread, NULL, capture_thread_func, NULL) != 0) {
        perror("Failed to create capture thread");
        return;
    }

}

void acap_stop() {
    stop_threads = 1; // 设置标志位，停止线程
    // 等待线程结束
    pthread_join(capture_thread, NULL);
    //pthread_join(play_thread, NULL);
}

// 播放指定 PCM 文件的函数
static void acap_play_pcm_file(const char* file_path) {
    FILE* pcm_file = fopen(file_path, "rb");
    if (!pcm_file) {
        perror("Failed to open PCM file");
        return;
    }

    init_play_pcm(SAMPLE_RATE, CHANNELS_NUM, BUFFER_SAMPLES, BITS_PER_SAMPLE);

    char pcm_play_buf[BUFFER_SAMPLES * BITS_PER_SAMPLE / 8];

    while (true) {
        // 从文件读取 PCM 数据
        size_t read_size = fread(pcm_play_buf, 1, sizeof(pcm_play_buf), pcm_file);
        if (read_size < sizeof(pcm_play_buf)) {
            // 如果文件读取完毕，退出循环
            printf("End of PCM file reached\n");
            break;
        }

        // 播放 PCM 数据
        play_pcm(pcm_play_buf);
    }

    deinit_play_pcm();
    fclose(pcm_file);
}

void set_tts_state(int tts_state)
{
    webrtc_enable_ref_audio(tts_state);
}
