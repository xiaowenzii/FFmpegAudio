#include "WlFFmpeg.h"

WlFFmpeg::WlFFmpeg(WlPlaystatus *playstatus, WlCallJava *callJava, const char *url) {
    this->playstatus = playstatus;
    this->callJava = callJava;
    this->url = url;
    exit = false;
    pthread_mutex_init(&init_mutex, NULL);
    pthread_mutex_init(&seek_mutex, NULL);
}

void *decodeFFmpeg(void *data) {
    WlFFmpeg *wlFFmpeg = (WlFFmpeg *) data;
    wlFFmpeg->decodeFFmpegThread();
    pthread_exit(&wlFFmpeg->decodeThread);
}

void WlFFmpeg::parpared() {
    pthread_create(&decodeThread, NULL, decodeFFmpeg, this);
}

int avformat_callback(void *ctx) {
    WlFFmpeg *fFmpeg = (WlFFmpeg *) ctx;
    if (fFmpeg->playstatus->exit) {
        return AVERROR_EOF;
    }
    return 0;
}

void WlFFmpeg::decodeFFmpegThread() {

    pthread_mutex_lock(&init_mutex);

    //1.注册解码器并初始化网络
    av_register_all();
    avformat_network_init();

    pFormatCtx = avformat_alloc_context();
    pFormatCtx->interrupt_callback.callback = avformat_callback;
    pFormatCtx->interrupt_callback.opaque = this;

    //2.打开文件或网络流
    if (avformat_open_input(&pFormatCtx, url, NULL, NULL) != 0) {
        if (LOG_DEBUG) {
            LOGE("can not open url :%s", url);
        }
        callJava->onCallError(CHILD_THREAD, 1001, "can not open url");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    //3.获取流信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        if (LOG_DEBUG) {
            LOGE("can not find streams from %s", url);
        }
        callJava->onCallError(CHILD_THREAD, 1002, "can not find streams from url");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        //4.获取音频流
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (audio == NULL) {
                //初始化 WlAudio
                audio = new WlAudio(playstatus, pFormatCtx->streams[i]->codecpar->sample_rate,
                                    callJava);
                audio->streamIndex = i;
                audio->codecpar = pFormatCtx->streams[i]->codecpar;
                //设置总时长
                audio->duration = pFormatCtx->duration / AV_TIME_BASE;
                audio->time_base = pFormatCtx->streams[i]->time_base;
                duration = audio->duration;
            }
        }
    }
    //5.获取解码器
    AVCodec *dec = avcodec_find_decoder(audio->codecpar->codec_id);
    if (!dec) {
        if (LOG_DEBUG) {
            LOGE("can not find decoder");
        }
        callJava->onCallError(CHILD_THREAD, 1003, "can not find decoder");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    //6.利用解码器创建解码器上下文
    audio->avCodecContext = avcodec_alloc_context3(dec);
    if (!audio->avCodecContext) {
        if (LOG_DEBUG) {
            LOGE("can not alloc new decodecctx");
        }
        callJava->onCallError(CHILD_THREAD, 1004, "can not alloc new decodecctx");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    if (avcodec_parameters_to_context(audio->avCodecContext, audio->codecpar) < 0) {
        if (LOG_DEBUG) {
            LOGE("can not fill decodecctx");
        }
        callJava->onCallError(CHILD_THREAD, 1005, "ccan not fill decodecctx");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    //7.打开解码器
    if (avcodec_open2(audio->avCodecContext, dec, 0) != 0) {
        if (LOG_DEBUG) {
            LOGE("cant not open audio strames");
        }
        callJava->onCallError(CHILD_THREAD, 1006, "cant not open audio strames");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    if (callJava != NULL) {
        if (playstatus != NULL && !playstatus->exit) {
            //回调 onCallParpared 函数
            callJava->onCallParpared(CHILD_THREAD);
        } else {
            exit = true;
        }
    }
    pthread_mutex_unlock(&init_mutex);
}

void WlFFmpeg::start() {
    if (audio == NULL) {
        if (LOG_DEBUG) {
            LOGE("audio is null");
            return;
        }
    }

    //调用
    audio->play();
    while (playstatus != NULL && !playstatus->exit) {
        if (playstatus->seek) {
            av_usleep(1000 * 100);
            continue;
        }

        //队列缓存10帧
        if (audio->queue->getQueueSize() > 10) {
            av_usleep(1000 * 100);
            continue;
        }
        //8.读取音频帧
        AVPacket *avPacket = av_packet_alloc();
        pthread_mutex_lock(&seek_mutex);
        int ret = av_read_frame(pFormatCtx, avPacket);
        pthread_mutex_unlock(&seek_mutex);
        if (ret == 0) {
            if (avPacket->stream_index == audio->streamIndex) {
                //解码操作
                audio->queue->putAvpacket(avPacket);
            } else {
                av_packet_free(&avPacket);
                av_free(avPacket);
            }
        } else {
            av_packet_free(&avPacket);
            av_free(avPacket);
            while (playstatus != NULL && !playstatus->exit) {
                if (audio->queue->getQueueSize() > 0) {
                    av_usleep(1000 * 100);
                    continue;
                } else {
                    playstatus->exit = true;
                    break;
                }
            }
        }
    }
    if (callJava != NULL) {
        callJava->onCallComplete(CHILD_THREAD);
    }
    exit = true;
}

void WlFFmpeg::pause() {
    if (audio != NULL) {
        audio->pause();
    }
}

void WlFFmpeg::resume() {
    if (audio != NULL) {
        audio->resume();
    }
}

void WlFFmpeg::release() {
    if (LOG_DEBUG) {
        LOGE("开始释放FFmpeg");
    }
    playstatus->exit = true;

    pthread_mutex_lock(&init_mutex);
    int sleepCount = 0;
    while (!exit) {
        if (sleepCount > 1000) {
            exit = true;
        }
        if (LOG_DEBUG) {
            LOGE("wait FFmpeg  exit %d", sleepCount);
        }
        sleepCount++;
        //暂停10毫秒
        av_usleep(1000 * 10);
    }

    if (LOG_DEBUG) {
        LOGE("释放 Audio");
    }

    if (audio != NULL) {
        audio->stop();
        audio->release();
        delete (audio);
        audio = NULL;
    }

    if (LOG_DEBUG) {
        LOGE("释放 封装格式上下文");
    }
    if (pFormatCtx != NULL) {
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        pFormatCtx = NULL;
    }
    if (LOG_DEBUG) {
        LOGE("释放 callJava");
    }
    if (callJava != NULL) {
        callJava = NULL;
    }
    if (LOG_DEBUG) {
        LOGE("释放 playstatus");
    }
    if (playstatus != NULL) {
        playstatus = NULL;
    }
    pthread_mutex_unlock(&init_mutex);
}

WlFFmpeg::~WlFFmpeg() {
    pthread_mutex_destroy(&init_mutex);
    pthread_mutex_destroy(&seek_mutex);
}

void WlFFmpeg::seek(int64_t secds) {
    if (duration <= 0) {
        return;
    }
    if (secds >= 0 && secds <= duration) {
        if (audio != NULL) {
            playstatus->seek = true;
            audio->queue->clearAvpacket();
            audio->clock = 0;
            audio->last_tiem = 0;
            pthread_mutex_lock(&seek_mutex);
            int64_t rel = secds * AV_TIME_BASE;
            //清空原来缓存的，直接跳到seek位置
            avcodec_flush_buffers(audio->avCodecContext);
            avformat_seek_file(pFormatCtx, -1, INT64_MIN, rel, INT64_MAX, 0);
            pthread_mutex_unlock(&seek_mutex);
            playstatus->seek = false;
        }
    }
}

void WlFFmpeg::setVolume(int percent) {
    if (audio != NULL) {
        audio->setVolume(percent);
    }
}

void WlFFmpeg::setMute(int mute) {
    if (audio != NULL) {
        audio->setMute(mute);
    }
}

void WlFFmpeg::setPitch(float pitch) {
    if (audio != NULL) {
        audio->setPitch(pitch);
    }
}

void WlFFmpeg::setSpeed(float speed) {
    if (audio != NULL) {
        audio->setSpeed(speed);
    }
}

int WlFFmpeg::getSampleRate() {
    if (audio != NULL) {
        return audio->avCodecContext->sample_rate;
    }
    return 0;
}

void WlFFmpeg::startStopRecord(bool start) {
    if (audio != NULL) {
        audio->startStopRecord(start);
    }
}
