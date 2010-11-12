/* Now double buffered with animation.
 */ 

#include <psl1ght/lv2.h>

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include <sysutil/video.h>
#include <rsx/gcm.h>
#include <rsx/reality.h>

#include <io/pad.h>
#include <io/cam.h>

#include <psl1ght/lv2.h>
#include <sysmodule/sysmodule.h>

gcmContextData *context; // Context to keep track of the RSX buffer.

VideoResolution res; // Screen Resolution

int currentBuffer = 0;
u32 *buffer[2]; // The buffer we will be drawing into.

void waitFlip() { // Block the PPU thread untill the previous flip operation has finished.
	while(gcmGetFlipStatus() != 0) 
		usleep(200);
	gcmResetFlipStatus();
}

void flip(s32 buffer) {
	assert(gcmSetFlip(context, buffer) == 0);
	realityFlushBuffer(context);
	gcmSetWaitFlip(context); // Prevent the RSX from continuing until the flip has finished.
}

// Initilize everything. You can probally skip over this function.
void init_screen() {
	// Allocate a 1Mb buffer, alligned to a 1Mb boundary to be our shared IO memory with the RSX.
	void *host_addr = memalign(1024*1024, 1024*1024);
	assert(host_addr != NULL);

	// Initilise Reality, which sets up the command buffer and shared IO memory
	context = realityInit(0x10000, 1024*1024, host_addr); 
	assert(context != NULL);

	VideoState state;
	assert(videoGetState(0, 0, &state) == 0); // Get the state of the display
	assert(state.state == 0); // Make sure display is enabled

	// Get the current resolution
	assert(videoGetResolution(state.displayMode.resolution, &res) == 0);
	
	// Configure the buffer format to xRGB
	VideoConfiguration vconfig;
	memset(&vconfig, 0, sizeof(VideoConfiguration));
	vconfig.resolution = state.displayMode.resolution;
	vconfig.format = VIDEO_BUFFER_FORMAT_XRGB;
	vconfig.pitch = res.width * 4;

	assert(videoConfigure(0, &vconfig, NULL, 0) == 0);
	assert(videoGetState(0, 0, &state) == 0); 

	s32 buffer_size = 4 * res.width * res.height; // each pixel is 4 bytes
	
	gcmSetFlipMode(GCM_FLIP_VSYNC); // Wait for VSYNC to flip

	// Allocate two buffers for the RSX to draw to the screen (double buffering)
	buffer[0] = rsxMemAlign(16, buffer_size);
	buffer[1] = rsxMemAlign(16, buffer_size);
	assert(buffer[0] != NULL && buffer[1] != NULL);

	u32 offset[2];
	assert(realityAddressToOffset(buffer[0], &offset[0]) == 0);
	assert(realityAddressToOffset(buffer[1], &offset[1]) == 0);
	// Setup the display buffers
	assert(gcmSetDisplayBuffer(0, offset[0], res.width * 4, res.width, res.height) == 0);
	assert(gcmSetDisplayBuffer(1, offset[1], res.width * 4, res.width, res.height) == 0);

	gcmResetFlipStatus();
	flip(1);
}

void fillFrame(u32 *buffer, u32 color) {
	s32 i, j;
	for(i = 0; i < res.height; i++) {
		for(j = 0; j < res.width; j++)
			buffer[i* res.width + j] = color;
	}

}
void unload_modules(){
	SysUnloadModule(SYSMODULE_CAM);
}

u32 YUV_to_RGB(int y,int u,int v)
{
   int r,g,b;
   v -= 128;
   u -= 128;
   // Conversion
   r = y + u;
   g = y-u/2-v/8;
   b = y+v;

   // Clamp to 0..1
   if (r < 0) r = 0;
   if (g < 0) g = 0;
   if (b < 0) b = 0;
   if (r > 255) r = 255;
   if (g > 255) g = 255;
   if (b > 255) b = 255;


   return r << 16 | g << 8 | b;
}

void Convert422(u8* yuv, u32 *rgb1, u32 *rgb2)
{
	int y1,y2,u,v;

	// Extract yuv components
	y1 = yuv[0];
	v  = yuv[1];
	y2 = yuv[2];
	u  = yuv[3];

	// yuv to rgb
	*rgb1 = YUV_to_RGB(y1,u,v);
	*rgb2 = YUV_to_RGB(y2,u,v);
}


s32 main(s32 argc, const char* argv[])
{
	PadInfo padinfo;
	PadData paddata;
	
	atexit(unload_modules);
	
	int i, j, ret;
	int running = 1, cameraSetup = 0;
	
	mem_container_t container;
	
	ret = lv2MemContinerCreate(&container, 0x200000);
	
	printf("lv2MemContinerCreate() returned %d\n", ret);
	CameraType type;
	CameraInfoEx cameraInfo;
	
	
	init_screen();
	ioPadInit(7);
	SysLoadModule(SYSMODULE_CAM);
	
	printf("cameraInit() returned %d\n", cameraInit());
	
	// Ok, everything is setup. Now for the main loop.
	while(running){
		// Check the pads.
		ioPadGetInfo(&padinfo);
		for(i=0; i<MAX_PADS; i++){
			if(padinfo.status[i]){
				ioPadGetData(i, &paddata);
				
				if(paddata.BTN_CROSS){
				
					running = 0;
				}
			}
			
		}

		waitFlip(); // Wait for the last flip to finish, so we can draw to the old buffer
		fillFrame(buffer[currentBuffer], 0x00FFFFFF); // Draw into the unused buffer
		
		if(!cameraSetup){
			cameraGetType(0, &type);
			if (type == CAM_TYPE_PLAYSTATION_EYE){
				cameraSetup = 1;
				printf("Found me an eye, arrr!\n");
				cameraInfo.format=CAM_FORM_YUV422;
				cameraInfo.framerate=30;
				cameraInfo.resolution=CAM_RESO_VGA;
				cameraInfo.info_ver=0x101;
				cameraInfo.container=container;
				cameraOpenEx(0, &cameraInfo);
				printf("Video dimensions: %dx%d\n", cameraInfo.width, cameraInfo.height);
				printf("Buffer at %08X\n", cameraInfo.buffer);
			}
		}else{
			s32 readcount, frame;
			ret = cameraRead(0, &frame, &readcount);
			if(ret == CAMERA_ERRO_NO_DEVICE_FOUND) {
				cameraSetup = 0;
			}else if(ret == CAMERA_ERRO_NEED_START) {      
				cameraReset(0);
				cameraStart(0);
			}else if (ret == 0){
				//printf("Ok lets try to print this image.\n");
				u8 * buf = (u8*)(u64)cameraInfo.buffer;
				//printf("Buffer at %p\n", buf);
				for(i=0;i<cameraInfo.height; i++){
					for(j=0;j<cameraInfo.width; j += 2){
						u32 pixel1, pixel2;
						Convert422(buf, &pixel1, &pixel2);
						buf += 4;
						buffer[currentBuffer][i* res.width + j] = pixel1;
						buffer[currentBuffer][(i)* res.width + j + 1] = pixel2;
						//printf("Pixel x: %d, y: %d = 0x%08X\n", i,j,pixel1);
						//printf("Pixel x: %d, y: %d = 0x%08X\n", i+1,j,pixel2);
					}
				}
			}
		}
		
		flip(currentBuffer); // Flip buffer onto screen
		currentBuffer = !currentBuffer;
	}
	
	cameraStop(0);
	
	cameraClose(0);
	cameraEnd();
  
	lv2MemContinerDestroy(container);
	printf("Exiting.\n");
	return 0;
}
