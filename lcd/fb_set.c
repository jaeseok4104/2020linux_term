#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

typedef struct _user_fb{
	int xres;
	int yres;
	int bpps;
} user_fb;

int main(int argc, char** argv){
	int frame_fd;
	int check;
	struct fb_var_screeninfo st_fvs;
	user_fb my_fb;
	
	if(argc<4){
		my_fb.xres = 1024;
		my_fb.yres = 600;
		my_fb.bpps = 32;
	}
	else{
		my_fb.xres = atoi(argv[1]);
		my_fb.yres = atoi(argv[2]);
		my_fb.bpps = atoi(argv[3]);
	}	
	
	frame_fd = open("/dev/fb0",O_RDWR);
	if(frame_fd < 0)
	{
		perror("Frame Buffer Open Error\n");
		exit(1);
	}
	
	if(check=ioctl(frame_fd,FBIOGET_VSCREENINFO,&st_fvs))
	{
		perror("Get Information Error - VSCREENINFO!");
		exit(1);
	}
	
	st_fvs.xres = my_fb.xres;
	st_fvs.yres = my_fb.yres;
	st_fvs.bits_per_pixel = my_fb.bpps;

	if(check=ioctl(frame_fd,FBIOPUT_VSCREENINFO,&st_fvs))
	{
		perror("PUT information Error - VSCREENINFO!");
		exit(1);
	}
	
	printf("===================\n");
	printf("Frame Buffer Info\n");
	printf("-------------------\n");
	printf("X - res     : %d\n",st_fvs.xres);
	printf("Y - res     : %d\n",st_fvs.yres);
	printf("X - v_res   : %d\n",st_fvs.xres_virtual);
	printf("Y - v_res   : %d\n",st_fvs.yres_virtual);
	printf("Bit/Pixel   : %d\n",st_fvs.bits_per_pixel);
	printf("===================\n");

	close(frame_fd);
	return 0;
}
