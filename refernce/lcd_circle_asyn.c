#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <linux/fb.h>

#define PUSH_SWITCH_DEVICE "/dev/fpga_push_switch"
#define PUSH_SWITCH_MAX_BUTTON 9

typedef unsigned int U32;

unsigned int makePixel(U32 r, U32 g, U32 b){
	return (U32)((r<<16)|(g<<8)|b);
}

int main(int argc, char** argv){
	int check,i;
	unsigned int width;
	int frame_fd, touch_fd, push_sw_dev;
	U32 pixel;
	int offset;
	int posx1,posy1, posx2, posy2;
	int repx, repy;
	unsigned int* pfbdata;
	unsigned short row,col,touch=0;
	unsigned short  data[8];
	unsigned char push_data[PUSH_SWITCH_MAX_BUTTON];
	long int dist;
	struct fb_var_screeninfo fvs;

	push_sw_dev = open(PUSH_SWITCH_DEVICE,O_RDONLY);
	if(push_sw_dev<0){
		perror("Push Switch Device open error");
		exit(1);
	}
	

	if((frame_fd=open("/dev/fb0",O_RDWR))<0){
		perror("Frame Buffer Open Error!");
		exit(1);
	}
	
	if((check= ioctl(frame_fd,FBIOGET_VSCREENINFO,&fvs))<0){
		perror("Get Information Error - VSCREENINFO!");
		exit(1);
	}
	
	if(fvs.bits_per_pixel != 32){
		perror("Unsupport Mode, 32Bpp Only!");	
		exit(1);
	}	
	if(argc<2){
		perror("more than 2 arguments need\n");
		exit(1);
	}
	if(argc>=3) width = atoi(argv[2]);
	else width = 30;
	touch_fd = open(argv[1],O_RDONLY);
	if(touch_fd<0){
		printf("%s : Open Error\n",argv[1]);
		exit(1);
	}

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
		printf("data :");
		for(i=0;i<8;i++)
			printf("%d\t",data[i]);
		printf("\n");

		for(i=0;i<PUSH_SWITCH_MAX_BUTTON;i++)
			printf("%d\t",push_data[i]);
		printf("\n");

		if(data[5] == 53){
			col = data[6];
			touch++;
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
			posy1 = (int)((row-width/2)<=0 ? 0 : row-width/2);
			posx1 = (int)((col-width/2)<=0 ? 0 : col-width/2);
			posy2 = (int)((row+width/2)<=0 ? 0 : row+width/2);
			posx2 = (int)((col+width/2)<=0 ? 0 : col+width/2);
			printf("center[%d] : %d,%d\n",touch,col,row);
			printf("Rect : %d,%d  %d,%d\n",posx1,posy1,posx2,posy2);
			
		
			for(repy = posy1; repy<posy2;repy++){
				offset = repy*fvs.xres;
				for(repx=posx1;repx<posx2;repx++){
					dist = (repy-row)*(repy-row) + (repx-col)*(repx-col);
					if(dist <= (width*width)/4) *(pfbdata+offset+repx) = (int)pixel;
				}
			}
			printf("Dist : %d,%d\n",dist,(12^2));
			touch = 0;
		}
	}
	
	munmap(pfbdata,fvs.xres*fvs.yres*32/8);
	close(frame_fd);
	close(touch_fd);
	close(push_sw_dev);
	return 0;
}


