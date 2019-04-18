#ifndef MYMUSIC_WLPLAYSTATUS_H
#define MYMUSIC_WLPLAYSTATUS_H

class WlPlaystatus {

public:
    bool exit;
    bool load = true;
    bool seek = false;

public:
    WlPlaystatus();
    ~WlPlaystatus();
};

#endif //MYMUSIC_WLPLAYSTATUS_H
