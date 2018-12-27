#include <stdio.h>
#include <syslog.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>

#include <bcm_host.h>

#define PAGE_SIZ 153600 

#pragma pack(1) // to keep the member of the structure next to each other
typedef union    // screen Configuration
{
	char totalscreen[PAGE_SIZ + PAGE_SIZ];
	struct
	{
		char screen1[PAGE_SIZ];
		char screen2[PAGE_SIZ];
	} screens;
} mainDisplay;


int process() {

    DISPMANX_MODEINFO_T display_info;
    uint32_t image_prt;
    VC_RECT_T rect1;
    int ret;

    struct fb_var_screeninfo vinfo1, vinfo2;
    struct fb_fix_screeninfo finfo1, finfo2;

    bcm_host_init();

    DISPMANX_DISPLAY_HANDLE_T display = vc_dispmanx_display_open(0);
    if (!display) {
        syslog(LOG_ERR, "Unable to open primary display");
        return -1;
    }
    ret = vc_dispmanx_display_get_info(display, &display_info);
    if (ret) {
        syslog(LOG_ERR, "Unable to get primary display information");
        return -1;
    }
    syslog(LOG_INFO, "Primary display is %d x %d", display_info.width, display_info.height);

    int fbfd1 = open("/dev/fb1", O_RDWR);
    if (fbfd1 == -1) {
        syslog(LOG_ERR, "Unable to open secondary display");
        return -1;
    }
    if (ioctl(fbfd1, FBIOGET_FSCREENINFO, &finfo1)) {
        syslog(LOG_ERR, "Unable to get secondary display information");
        return -1;
    }
    if (ioctl(fbfd1, FBIOGET_VSCREENINFO, &vinfo1)) {
        syslog(LOG_ERR, "Unable to get secondary display information");
        return -1;
    }

    syslog(LOG_INFO, "Second display is %d x %d %dbps\n", vinfo1.xres, vinfo1.yres, vinfo1.bits_per_pixel);

	int fbfd2 = open("/dev/fb2", O_RDWR);
    if (fbfd2 == -1) {
        syslog(LOG_ERR, "Unable to open third display");
        return -1;
    }
    if (ioctl(fbfd2, FBIOGET_FSCREENINFO, &finfo2)) {
        syslog(LOG_ERR, "Unable to get third display information");
        return -1;
    }
    if (ioctl(fbfd2, FBIOGET_VSCREENINFO, &vinfo2)) {
        syslog(LOG_ERR, "Unable to get third display information");
        return -1;
    }

    syslog(LOG_INFO, "third display is %d x %d %dbps\n", vinfo2.xres, vinfo2.yres, vinfo2.bits_per_pixel);

    DISPMANX_RESOURCE_HANDLE_T screen_resource = vc_dispmanx_resource_create(VC_IMAGE_RGB565, (display_info.width), (display_info.height), &image_prt);
    if (!screen_resource) {
        syslog(LOG_ERR, "Unable to create screen buffer");
        close(fbfd1);
        close(fbfd2);
        vc_dispmanx_display_close(display);
        return -1;
    }
    

    ftruncate(fbfd1, PAGE_SIZ);
	ftruncate(fbfd2, PAGE_SIZ);
	
	mainDisplay DISPLAY; // CREATE THE DISPLAY
	
	syslog(LOG_INFO, "---------------------------------------\n");
	syslog(LOG_INFO, "pa for the main screen: %p\n", DISPLAY.totalscreen);
	syslog(LOG_INFO, "pa for the screen 1: %p\n", DISPLAY.screens.screen1);
	syslog(LOG_INFO, "pa for the screen 2: %p\n", DISPLAY.screens.screen2);
	
    char* fbp1 = (char*) mmap(NULL, PAGE_SIZ, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd1, 0);
    if (fbp1 <= 0) {
        syslog(LOG_ERR, "Unable to create memory mapping for second display %i", fbp1);
        close(fbfd1);
        close(fbfd2);
        ret = vc_dispmanx_resource_delete(screen_resource);
        vc_dispmanx_display_close(display);
        return -1;
    }
	else{
		syslog(LOG_INFO, "pa for the mapped screen 1: %p\n", fbp1);
	}

    char* fbp2 = (char*) mmap(fbp1+PAGE_SIZ, PAGE_SIZ, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd2, 0);
    if (fbp2 <= 0) {
        syslog(LOG_ERR, "Unable to create memory mapping for third display %i", fbp2);
        munmap(fbp1, PAGE_SIZ);
        close(fbfd1);
        close(fbfd2);
        ret = vc_dispmanx_resource_delete(screen_resource);
        vc_dispmanx_display_close(display);
        return -1;
    }
	else{
		syslog(LOG_INFO, "pa for the mapped screen 2: %p\n", fbp2);
	}

    ret = vc_dispmanx_rect_set(&rect1, 0, 0, display_info.width , display_info.height);
    
    if (ret) {
        syslog(LOG_ERR, "Unable to set rect 1");
    }

    syslog(LOG_INFO, "everything is initialized");

    while (1) {
        ret = vc_dispmanx_snapshot(display, screen_resource, 0);
        vc_dispmanx_resource_read_data(screen_resource, &rect1, DISPLAY.totalscreen, vinfo1.xres * vinfo1.bits_per_pixel / 8);
        
        //-------------------------------------------
        memcpy(fbp2,DISPLAY.screens.screen2, PAGE_SIZ);
        memcpy(fbp1,DISPLAY.screens.screen1, PAGE_SIZ);
        
    }

    munmap(fbp1, PAGE_SIZ);
	munmap(fbp2, PAGE_SIZ);
    close(fbfd1);
    close(fbfd2);
    ret = vc_dispmanx_resource_delete(screen_resource);
    vc_dispmanx_display_close(display);
}

int main(int argc, char **argv) {
    setlogmask(LOG_UPTO(LOG_DEBUG));
    openlog("fbcp", LOG_NDELAY | LOG_PID, LOG_USER);

    return process();
}
