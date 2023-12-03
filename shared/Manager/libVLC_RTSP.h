// libVLC_RTSP.h
#pragma once
#include <string>
#ifdef _WIN32
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif
#include <vlc/vlc.h>


class LibVlcStreamComponent;

class libVLC_RTSP
{
public:
    libVLC_RTSP();
    virtual ~libVLC_RTSP();

    void InitVideoSurfaces();

    bool Init(const std::string& rtsp_url, int cachingMS, SurfaceAnim* pSurfaceToWriteTo, LibVlcStreamComponent* pStreamComp, int width, int height);
    void Update();

    static void* lock(void* data, void** p_pixels);
    static void unlock(void* data, void* id, void* const* p_pixels);
    static void display(void* data, void* id);
    void SetVolume(float vol);  // Set the volume level. Values range between 0 and 1

    libvlc_media_player_t* GetMP() { return m_pVlcMediaPlayer; }
    void Release();

protected:

    void UpdateFrame();

    unsigned char* m_pImageBuffer = NULL; // Buffer for OpenGL texture data

    int m_video_width = 320;
    int m_video_height = 200;

    libvlc_media_player_t* m_pVlcMediaPlayer = NULL;
    libvlc_media_t* m_pVlcMedia = NULL;

    SurfaceAnim *m_pSurface = NULL;
    LibVlcStreamComponent* m_pStreamComp = NULL;
    string m_source;
    int m_cachingMS;
};

void OneTimeReleaseOnAppClose(); //you should probably call this when you exit the app, to release the main VLC instance