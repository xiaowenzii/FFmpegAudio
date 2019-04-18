#ifndef MYMUSIC_WLAUDIO_H
#define MYMUSIC_WLAUDIO_H

#include "WlQueue.h"
#include "WlPlaystatus.h"
#include "WlCallJava.h"
#include "SoundTouch.h"

/* TODO 引入使用 soundtouch */
using namespace soundtouch;

extern "C"
{
#include "libavcodec/avcodec.h"
#include <libswresample/swresample.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <libavutil/time.h>
};

class WlAudio {

public:
    int streamIndex = -1;
    AVCodecContext *avCodecContext = NULL;
    AVCodecParameters *codecpar = NULL;
    WlQueue *queue = NULL;
    WlPlaystatus *playstatus = NULL;
    //调用java类
    WlCallJava *callJava = NULL;

    pthread_t thread_play;
    AVPacket *avPacket = NULL;
    AVFrame *avFrame = NULL;
    int ret = 0;
    uint8_t *buffer = NULL;
    int data_size = 0;
    int sample_rate = 0;

    int duration = 0;
    AVRational time_base;
    //总的播放时长
    double clock;
    //当前frame时间
    double now_time;
    //上一次调用时间
    double last_tiem;

    //音量
    int volumePercent = 100;
    //声道
    int mute = 2;
    //声调
    float pitch = 1.0f;
    //速度
    float speed = 1.0f;
    //是否正在录制
    bool isRecordPcm = false;
    //针对ape音乐，判断AvFrame是否读取完成
    bool readAvFrameFinished = true;

    // 引擎接口
    SLObjectItf engineObject = NULL;
    SLEngineItf engineEngine = NULL;

    //混音器
    SLObjectItf outputMixObject = NULL;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;
    SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

    //pcm
    SLObjectItf pcmPlayerObject = NULL;
    SLPlayItf pcmPlayerPlay = NULL;
    SLVolumeItf pcmVolumePlay = NULL;
    SLMuteSoloItf pcmMutePlay = NULL;

    //缓冲器队列接口
    SLAndroidSimpleBufferQueueItf pcmBufferQueue = NULL;

    //创建 SoundTouch
    SoundTouch *soundTouch = NULL;
    SAMPLETYPE *sampleBuffer = NULL;
    bool finished = true;
    uint8_t *out_buffer = NULL;
    int nb = 0;
    int num = 0;

public:
    WlAudio(WlPlaystatus *playstatus, int sample_rate, WlCallJava *callJava);

    ~WlAudio();

    void play();

    int resampleAudio(void **pcmbuf);

    void initOpenSLES();

    int getCurrentSampleRateForOpensles(int sample_rate);

    void pause();

    void resume();

    void stop();

    void release();

    void setVolume(int percent);

    void setMute(int mute);

    int getSoundTouchData();

    void setPitch(float pitch);

    void setSpeed(float speed);

    int getPCMDB(char *pcmcata, size_t pcmsize);

    void startStopRecord(bool start);
};

#endif //MYMUSIC_WLAUDIO_H
