#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <linux/fb.h>
#include "./glcd-font.h"


#define PUSH_SWITCH_DEVICE "/dev/fpga_push_switch"
#define PUSH_SWITCH_MAX_BUTTON 9

#define TOUCH_SCREEN "/dev/input/event1"

typedef unsigned int U32;

int frame_fd, touch_fd, push_sw_dev;
struct fb_var_screeninfo fvs;
unsigned int* pfbdata;

unsigned int initialize();
unsigned int paint_dot(int x, int y, int width, U32 pixel);

unsigned int makePixel(U32 r, U32 g, U32 b){
	return (U32)((r<<16)|(g<<8)|b);
}


int main(int argc, char** argv){
	unsigned int width,i;
	U32 pixel;
	int offset;
	int posx1,posy1, posx2, posy2;
	int repx, repy;
	unsigned short row,col,touch=0;
	unsigned short  data[8];
	unsigned char push_data[PUSH_SWITCH_MAX_BUTTON];
	long int dist;

	if(!initialize()) exit(1);

	pfbdata = (unsigned int*)mmap(0,fvs.xres*fvs.yres*32/8,PROT_READ|\
		PROT_WRITE,MAP_SHARED,frame_fd,0);
	if((unsigned)pfbdata == (unsigned)-1){
		perror("Error Mapping");
		exit(1);
	}	
	pixel = makePixel(255,0,0);

	while(1){
		read(push_sw_dev,&push_data,sizeof(push_data));
		read(touch_fd,data,8*2);

		if(data[5] == 53){
			col = data[6];
			touch=1;
		}
		else if(data[5] == 54){
			row = data[6];
			touch++;

		}

		if(push_data[4] == 1){	// Clear Screen
			printf("Clear Screen\n");
			for(repy = 0; repy<fvs.yres;repy++){
				offset = repy*fvs.xres;
				for(repx=0;repx<fvs.xres;repx++){
					*(pfbdata+offset+repx) = 0;
				}
			}
		}
		else if(touch>=2){
			touch = 0;
			paint_dot(col,row,10,pixel);
		}
	}
	munmap(pfbdata,fvs.xres*fvs.yres*32/8); close(frame_fd);
	close(touch_fd);
	close(push_sw_dev);
	return 0;
}


unsigned int initialize(){
	int check,i;
	push_sw_dev = open(PUSH_SWITCH_DEVICE,O_RDONLY);
	if(push_sw_dev<0){
		perror("Push Switch Device open error");
		return 0;
	}

	if((frame_fd=open("/dev/fb0",O_RDWR))<0){
		perror("Frame Buffer Open Error!");
		return 0;
	}
	
	if((check= ioctl(frame_fd,FBIOGET_VSCREENINFO,&fvs))<0){
		perror("Get Information Error - VSCREENINFO!");
		return 0;
	}
	
	if(fvs.bits_per_pixel != 32){
		perror("Unsupport Mode, 32Bpp Only!");	
		return 0;
	}	

	touch_fd = open(TOUCH_SCREEN,O_RDONLY);
	if(touch_fd<0){
		printf("%s : Open Error\n",TOUCH_SCREEN);
		return 0;
	}

	return 1;
}
unsigned int paint_dot(int x, int y, int width, U32 pixel){
	int posx1 = x- width/2;
	int posy1 = y - width/2;
	int posx2 = x + width/2;
	int posy2 = y + width/2;

	int repy,repx,offset,dist;
	for(repy = posy1; repy<posy2;repy++){
		offset = repy*fvs.xres;
		for(repx=posx1;repx<posx2;repx++){
			dist = (repy-y)*(repy-y) + (repx-x)*(repx-x);
			if(offset+repx>= 1024*600 || (offset+repx)<=0) continue;
			if(dist <= (width*width)/4) *(pfbdata+offset+repx) = (int)pixel;
		}
	}
}
