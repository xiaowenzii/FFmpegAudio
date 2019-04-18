package com.xiaowenzi.ffmpeg.audio;

import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;

import com.xiaowenzi.ffpeglib.bean.MuteEnum;
import com.xiaowenzi.ffpeglib.bean.WlTimeInfoBean;
import com.xiaowenzi.ffpeglib.listener.WlOnCompleteListener;
import com.xiaowenzi.ffpeglib.listener.WlOnDBListener;
import com.xiaowenzi.ffpeglib.listener.WlOnErrorListener;
import com.xiaowenzi.ffpeglib.listener.WlOnLoadListener;
import com.xiaowenzi.ffpeglib.listener.WlOnParparedListener;
import com.xiaowenzi.ffpeglib.listener.WlOnPauseResumeListener;
import com.xiaowenzi.ffpeglib.listener.WlOnRecordTimeListener;
import com.xiaowenzi.ffpeglib.listener.WlOnTimeInfoListener;
import com.xiaowenzi.ffpeglib.log.MyLog;
import com.xiaowenzi.ffpeglib.player.WlPlayer;
import com.xiaowenzi.ffpeglib.util.WlTimeUtil;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    private WlPlayer wlPlayer;
    private TextView tvTime;
    private TextView tvVolume;
    private SeekBar seekBarSeek;
    private SeekBar seekBarVolume;
    private TextView tvDb;
    private TextView tvRecordTime;

    private int position = 0;
    private boolean isSeekBar = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        tvTime = findViewById(R.id.tv_time);
        tvVolume = findViewById(R.id.tv_volume);
        seekBarSeek = findViewById(R.id.seekbar_seek);
        seekBarVolume = findViewById(R.id.seekbar_volume);
        tvDb = findViewById(R.id.tv_db);
        tvRecordTime = findViewById(R.id.tv_recore_time);

        //实例化一个播放器
        wlPlayer = new WlPlayer();
        //初始化音量
        wlPlayer.setVolume(50);
        tvVolume.setText("音量 " + wlPlayer.getVolumePercent() + "%");
        seekBarVolume.setProgress(wlPlayer.getVolumePercent());
        //初始化声道
        wlPlayer.setMute(MuteEnum.MUTE_LEFT);
        //初始化声调
        wlPlayer.setPitch(1.5f);
        //初始化音速
        wlPlayer.setSpeed(1.5f);

        wlPlayer.setWlOnParparedListener(new WlOnParparedListener() {
            @Override
            public void onParpared() {
                wlPlayer.start();
            }
        });

        wlPlayer.setWlOnLoadListener(new WlOnLoadListener() {
            @Override
            public void onLoad(boolean load) {
                if (load) {
                    MyLog.e("加载中...");
                } else {
                    MyLog.e("播放中...");
                }
            }
        });

        wlPlayer.setWlOnPauseResumeListener(new WlOnPauseResumeListener() {
            @Override
            public void onPause(boolean pause) {
                if (pause) {
                    MyLog.e("暂停中...");
                } else {
                    MyLog.e("播放中...");
                }
            }
        });

        wlPlayer.setWlOnTimeInfoListener(new WlOnTimeInfoListener() {
            @Override
            public void onTimeInfo(WlTimeInfoBean timeInfoBean) {
                Message message = Message.obtain();
                message.what = 1;
                message.obj = timeInfoBean;
                handler.sendMessage(message);
            }
        });

        wlPlayer.setWlOnErrorListener(new WlOnErrorListener() {
            @Override
            public void onError(int code, String msg) {
                MyLog.e("code:" + code + ", msg:" + msg);
            }
        });

        wlPlayer.setWlOnCompleteListener(new WlOnCompleteListener() {
            @Override
            public void onComplete() {
                MyLog.e("播放完成");
            }
        });

        wlPlayer.setWlOnDBListener(new WlOnDBListener() {
            @Override
            public void onDbValue(int db) {
                Message message = Message.obtain();
                message.what = 2;
                message.obj = db;
                handler.sendMessage(message);
            }
        });

        wlPlayer.setWlOnRecordTimeListener(new WlOnRecordTimeListener() {
            @Override
            public void onRecordTime(int recordTime) {
                Message message = Message.obtain();
                message.what = 3;
                message.obj = recordTime;
                handler.sendMessage(message);
            }
        });

        //调整监听播放进度
        seekBarSeek.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (wlPlayer.getTotalDuration() > 0 && isSeekBar) {
                    position = wlPlayer.getTotalDuration() * progress / 100;
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                isSeekBar = true;
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                wlPlayer.seek(position);
                isSeekBar = false;
            }
        });

        //监听声音大小
        seekBarVolume.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                wlPlayer.setVolume(progress);
                tvVolume.setText("音量 " + wlPlayer.getVolumePercent() + "%");
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });
    }

    //TODO 点击按钮事件
    public void begin(View view) {
        wlPlayer.setSource(Environment.getExternalStorageDirectory() + "/12/music.m4a");
        wlPlayer.parpared();
    }

    public void stop(View view) {
        wlPlayer.stop();
    }

    public void pause(View view) {
        wlPlayer.pause();
    }

    public void play(View view) {
        wlPlayer.resume();
    }

    public void next(View view) {
        wlPlayer.playNext("http://mpge.5nd.com/2015/2015-11-26/69708/1.mp3");
    }

    public void left(View view) {
        wlPlayer.setMute(MuteEnum.MUTE_LEFT);
    }

    public void right(View view) {
        wlPlayer.setMute(MuteEnum.MUTE_RIGHT);
    }

    public void center(View view) {
        wlPlayer.setMute(MuteEnum.MUTE_CENTER);
    }

    public void speed(View view) {
        wlPlayer.setSpeed(1.5f);
        wlPlayer.setPitch(1.0f);
    }

    public void pitch(View view) {
        wlPlayer.setPitch(1.5f);
        wlPlayer.setSpeed(1.0f);
    }

    public void speedpitch(View view) {
        wlPlayer.setSpeed(1.5f);
        wlPlayer.setPitch(1.5f);
    }

    public void normalspeedpitch(View view) {
        wlPlayer.setSpeed(1.0f);
        wlPlayer.setPitch(1.0f);
    }

    public void startRecord(View view) {
        wlPlayer.startRecord(new File(Environment.getExternalStorageDirectory() + "/12/music.aac"));
    }

    public void stopRecord(View view) {
        wlPlayer.stopRecord();
    }

    public void pauseRecord(View view) {
        wlPlayer.pauseRecord();
    }

    public void continueRecord(View view) {
        wlPlayer.resumeRcord();
    }

    Handler handler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            if (msg.what == 1) {
                if (!isSeekBar) {
                    WlTimeInfoBean wlTimeInfoBean = (WlTimeInfoBean) msg.obj;
                    tvTime.setText(WlTimeUtil.secdsToDateFormat(wlTimeInfoBean.getTotalTime(), wlTimeInfoBean.getTotalTime())
                            + "/" + WlTimeUtil.secdsToDateFormat(wlTimeInfoBean.getCurrentTime(), wlTimeInfoBean.getTotalTime()));

                    seekBarSeek.setProgress(wlTimeInfoBean.getCurrentTime() * 100 / wlTimeInfoBean.getTotalTime());
                }
            } else if (msg.what == 2) {
                int db = (int) msg.obj;
                tvDb.setText("DB: " + db);
            } else if (msg.what == 3) {
                int recordTime = (int) msg.obj;
                tvRecordTime.setText("录制时间: " + recordTime);
            }
        }
    };
}
