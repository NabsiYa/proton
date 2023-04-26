#include "BaseApp.h"

extern "C" {
    #include <GLES/egl.h> // GLES1
    #include <gpu_es4/psp2_pvr_hint.h>  // PVR_PSP2
}

#include <psp2/kernel/threadmgr.h> // sceKernelDelayThread
#include <psp2/kernel/processmgr.h> // sceKernelExitProcess
#include <psp2/kernel/modulemgr.h> // sceKernelLoadStartModule

#include "psp2Touch.h"

int g_winVideoScreenX;
int g_winVideoScreenY;

#define MEMORY_VITAGL_THRESHOLD_MB 32

int GetPrimaryGLX() 
{
	return g_winVideoScreenX;
}

int GetPrimaryGLY() 
{
	return g_winVideoScreenY;
}

int _newlib_heap_size_user = 200 * 1024 * 1024; // extend the newlib heap.
unsigned int sceLibcHeapSize = 16 * 1024 * 1024; // extend libc heap.

void check_system_messages()
{
    while (!GetBaseApp()->GetOSMessages()->empty())
	{
		OSMessage m = GetBaseApp()->GetOSMessages()->front();
		GetBaseApp()->GetOSMessages()->pop_front();

		switch (m.m_type)
		{
			case OSMessage::MESSAGE_CHECK_CONNECTION:
				GetMessageManager()->SendGUI(MESSAGE_TYPE_OS_CONNECTION_CHECKED, (float)RT_kCFStreamEventOpenCompleted, 0.0f);	
				break;
		}
	}
}

EGLConfig config;
EGLint numConfigs, majorVersion, minorVersion;

void load_modules()
{
    sceKernelLoadStartModule("vs0:sys/external/libfios2.suprx", 0, NULL, 0, NULL, NULL);
	sceKernelLoadStartModule("vs0:sys/external/libc.suprx", 0, NULL, 0, NULL, NULL);
	sceKernelLoadStartModule("app0:module/libgpu_es4_ext.suprx", 0, NULL, 0, NULL, NULL);
  	sceKernelLoadStartModule("app0:module/libIMGEGL.suprx", 0, NULL, 0, NULL, NULL);
}

void init_pvr()
{
    PVRSRV_PSP2_APPHINT hint;
  	PVRSRVInitializeAppHint(&hint);
  	PVRSRVCreateVirtualAppHint(&hint);
}

int main() {

    // load required modules for PVR
    load_modules();

    // initialize PVR
    init_pvr();
    
    EGLBoolean result;
	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if(!display)
	{
		sceClibPrintf("EGL display get failed.\n");
		return 0;
	}

	result = eglInitialize(display, &majorVersion, &minorVersion);
	if (result == EGL_FALSE)
	{
		sceClibPrintf("EGL initialize failed.\n");
		return 0;
	}

	eglBindAPI(EGL_OPENGL_ES_API);

    EGLint configAttributes[] =
    {
        EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
    };

	result = eglChooseConfig(display, configAttributes, &config, 1, &numConfigs);
	if (result == EGL_FALSE)
	{
		sceClibPrintf("EGL config initialize failed.\n");
		return 0;
	}

	EGLSurface surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)0, nullptr);
	if (!surface)
	{
		sceClibPrintf("EGL surface create failed.\n");
		return 0;
	}

    EGLint contextAttributeList[] = 
    {
        EGL_CONTEXT_CLIENT_VERSION, 1,  // GLES1
        EGL_NONE
    };

	EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttributeList);
	if (!context)
	{
		sceClibPrintf("EGL content create failed.\n");
		return 0;
	}

	eglMakeCurrent(display, surface, surface, context);

	eglQuerySurface(display, surface, EGL_WIDTH, &g_winVideoScreenX);
	eglQuerySurface(display, surface, EGL_HEIGHT, &g_winVideoScreenY);

    // Inform which resolution we're going to use, and what rotation.
    SetupScreenInfo(GetPrimaryGLX(), GetPrimaryGLY(), ORIENTATION_PORTRAIT);

    // This is required for RTDink networking.
    SetEmulatedPlatformID(PLATFORM_ID_PSVITA);

    if (!GetBaseApp()->Init()) sceKernelExitProcess(0);    // Check the logs for the reason of failure.

    // Initialize touch input
    initialize_touch();

    while (true) {

        // Run logic and draw the graphics.
        GetBaseApp()->Update();
		GetBaseApp()->Draw();

        // Poll for new touch input(s)
        poll_touch();

        // Poll for new system messages
        check_system_messages();

        // Performing buffer swap
		eglSwapBuffers(display, surface);

        // Sleep for 1ms, give time for the program to rest.
        sceKernelDelayThread(1000);
    }

    // terminate PVR
    eglDestroySurface(display, surface);
  	eglDestroyContext(display, context);
  	eglTerminate(display);

    // Terminate touch input
    deinitialize_touch();

    sceKernelExitProcess(0);
}