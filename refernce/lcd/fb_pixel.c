#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/fb.h>

typedef unsigned int U32;

unsigned int makepixel(U32 r, U32 g, U32 b)
{
	return (U32)((r<<16)|(g<<8)|b);
}

int main(int argc, char** argv)
{
	int check;
	int frame_fd;
	U32 pixel;
	int offset;
	struct fb_var_screeninfo fvs;
	
	if((frame_fd=open("/dev/fb0",O_RDWR))<0){
		perror("Frame Buffer Open Error!");
		exit(1);
	}
	
	if((check = ioctl(frame_fd,FBIOGET_VSCREENINFO,&fvs))<0){
		perror("Get Information Error - VSCREENINFO!");
		exit(1);
	}
	
	if(fvs.bits_per_pixel != 32){
		perror("Unsupport Mode, 32Bpp only.");
		exit(1);
	}

	offset = 120*fvs.xres*sizeof(pixel) + 100*sizeof(pixel);
	if(lseek(frame_fd,offset,SEEK_SET) <0){
		perror("LSeek Error!");
		exit(1);
	}
	pixel = makepixel(255,0,0);
	write(frame_fd,&pixel,fvs.bits_per_pixel/8);

	offset = 120*fvs.xres*sizeof(pixel) + 120*sizeof(pixel);
	if(lseek(frame_fd,offset,SEEK_SET)<0){
		perror("LSeek Error!");
		exit(1);
	}
	pixel = makepixel(0,255,0);	
	write(frame_fd,&pixel,fvs.bits_per_pixel/8);

	offset = 120*fvs.xres*sizeof(pixel) + 140*sizeof(pixel);
	if(lseek(frame_fd,offset,SEEK_SET)<0){
		perror("LSeek Error!");
		exit(1);
	}
	pixel = makepixel(0,0,255);	
	write(frame_fd,&pixel,fvs.bits_per_pixel/8);


	close(frame_fd);
	return 0;
}
