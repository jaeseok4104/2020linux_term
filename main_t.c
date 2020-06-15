#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <sys/signal.h>
#include <termios.h>
#include <pthread.h>


#include "./glcd-font.h"
#include "./mario.h"
#include "./nfc.h"



#define PUSH_SWITCH_DEVICE "/dev/fpga_push_switch"
#define PUSH_SWITCH_MAX_BUTTON 9

#define SCREEN_BPP 32

#define TOUCH_SCREEN "/dev/input/event1"
#define NFC_PORT "/dev/ttyUSB0"
#define KEYBOARD_EVENT "/dev/input/event6"
#define MARIO_STEP 15


typedef unsigned int U32;

typedef struct _user_fb{
	int xres;
	int yres;
	int bpps;
} user_fb;

typedef struct _U_file{
	FILE *user_file;
	char name[100];
	unsigned char line[100];
	int num;
}U_file;

typedef struct _user_info{
	char name[20];
	char id[20];
	char check;
}U_info;

typedef struct {
	int posX;
	int posY;

	int rows;
	int cols;

	int width;
	int height;

	char **str;  // cell string data [rows][cols]

	int borderWidth;
	int fontSize;

	U32 borderColor;
	U32 bgColor;
	U32 fontColor;
}t_table;

typedef struct{
	int posX;
	int posY;
	int width;
	int height;

	U32 color;
	char caption[100];
}t_button;

t_table table1;

t_button button_list;
t_button button_check;
t_button button_register;
t_button button_back;

int frame_fd, push_sw_dev,touch_fd,keyboard_fd;
struct fb_var_screeninfo fvs;
unsigned int* pfbdata;

unsigned int initialize();
unsigned int button_init();

unsigned int touch_scan(int touchX,int touchY);

unsigned int clear_Screen();
unsigned int paint_dot(int x, int y, int width, U32 pixel,int full);
unsigned int paint_rect(int x, int y, int width, U32 pixel,int full);
unsigned int paint_rect2(int x, int y, int width,int height, U32 pixel,int full);
unsigned int paint_font(int num,int x, int y);
unsigned int paint_str(int x, int y,char* str,int length,int fontSize,int fontpadding,U32 pixel,U32 bgColor,int transparent);
unsigned int paint_mario(int x, int y,int Size,int draw);
unsigned int move_mario(unsigned char* push_data);

unsigned int setTable();
unsigned int paint_table(t_table t);

unsigned int paint_menu();
unsigned int paint_register();

unsigned int makePixel(U32 r, U32 g, U32 b){
	//return (U32)((r<<11)|(g<<5)|b);
	return (U32)((r<<16)|(g<<8)|b);
}

/*********** NFC_Function   **************/
char open_file(U_file* pU_file, char *mode);

void made_checkboard(void);
void add_user(void);
void callname_user(void);

void read_ACK(unsigned char temp);
void send_tag(void);
char check_id(void);
void wake_card();

static int hNFC;
U_file user_data;
U_file user_check;

unsigned char old_id[5];
unsigned char receive_ACK[25];
unsigned char id_buff[100];
unsigned char temp_data;

struct termios oldtio,newtio;
/////////////////////////////////////////////

// Keyboard

char keyBuf[300];
char keymapping[255];

unsigned int key_read(char* buf);
char key_map(char* arr);


//////////////////////////////////////////////

unsigned int mario_x,mario_y;
unsigned int mario_x_prev,mario_y_prev;
unsigned int lastPush;

// Thread //
void *pThreadFunc(void* arg);
void *pThreadTimer(void* arg);


pthread_t threadID,threadID2;
///////////////////////////////////////////
int exitTouch;
int state;  // 0: menu, 1: Register card, 2: Check, 3: show List
int paintOnce;	//Paint Once Flag int cell =0;
struct input_event touch_iev[3];

int dirx;
int diry;
int main(int argc, char** argv){
	time_t tt;
	U32 pixel;
	int cell;

	state = 0;
	paintOnce =1;


	unsigned char push_data[PUSH_SWITCH_MAX_BUTTON];


	char strBuf[30];

	if(argc==4){
		int r = atoi(argv[1]);
		int g = atoi(argv[2]);
		int b = atoi(argv[3]);
		pixel = makePixel(r,g,b);
	}
	else pixel = makePixel(0,255,0);
	
	//Device Init
	if(!initialize()) exit(1);
	button_init();

	strcpy(user_data.name,"linux.txt");
	strcpy(user_check.name,"2020-06-05.txt");
	// Table Data Setting
	setTable();


	// pointer of FrameBuffer 
	pfbdata = (unsigned int*)mmap(0,fvs.xres*fvs.yres*SCREEN_BPP /8,PROT_READ|\
		PROT_WRITE,MAP_SHARED,frame_fd,0);
	if((unsigned)pfbdata == (unsigned)-1){
		perror("Error Mapping\n");
		exit(1);
	}	

	clear_Screen();

	//paint_table(table1);
	mario_x = 50;
	mario_x_prev = 850;
	mario_y = 30;
	mario_y_prev = 30;
	dirx = 5;
	diry = 2;

	//paint_menu();

	printf("Program Start\n");

	while(1){
		read(push_sw_dev,&push_data,sizeof(push_data));
		if(push_data[4] == 1){	// Clear Screen
			printf("Clear Screen\n");
			clear_Screen();
			paintOnce = 1;
			//made_checkboard();
			//paint_table(table1);
		}

		move_mario(push_data);
		time(&tt);
		sprintf(strBuf,"Now :%s",ctime(&tt));
		paint_str(100,50,strBuf,strlen(strBuf),3,3,makePixel(0,255,0),0,0);

		if(paintOnce){
			paintOnce = 0;
			switch(state){
				case 0:		//menu
					break;
				case 1:		// Register Card
					break;
				case 2:		// Check 
				
					break;
				case 3:		// Show List
					break;
				default:
					break;
				}
		}

		mario_x += dirx;
		mario_y += diry;

		if( mario_x <=dirx) {
			mario_x = 0;
			dirx = -dirx;
		}
		if( mario_y <=diry){
			mario_y = 0;
			diry = -diry;
		}
		if(mario_x >=(fvs.xres - 6*MARIO_SIZE_X-dirx)){
			mario_x = (fvs.xres - 6*MARIO_SIZE_X-dirx);
			dirx = -dirx;
		}
		if(mario_y >=(fvs.yres - 6*MARIO_SIZE_Y-diry)){
			mario_y = (fvs.yres - 6*MARIO_SIZE_Y-diry);
			diry = -diry;

		}

		
	} 
	munmap(pfbdata,fvs.xres*fvs.yres*SCREEN_BPP /8);
	close(frame_fd);
	//tcsetattr(hNFC,TCSANOW,&oldtio);
	
	close(push_sw_dev);
	//close(hNFC);
	//close(keyboard_fd);
	close(touch_fd);
	
	for(cell =0; cell<table1.rows*table1.cols;cell++){
		free(table1.str[cell]);
	}
	free(table1.str);

	return 0; 
} 

unsigned int initialize(){
	int check,i;
	int nRet,status;
	if((push_sw_dev = open(PUSH_SWITCH_DEVICE,O_RDONLY))<0){
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
	fvs.xres = 1024;
	fvs.yres = 600;
	fvs.bits_per_pixel = 32;

	if(check=ioctl(frame_fd,FBIOPUT_VSCREENINFO,&fvs))
	{
		perror("PUT information Error - VSCREENINFO!");
		exit(1);
	}
	if((check= ioctl(frame_fd,FBIOGET_VSCREENINFO,&fvs))<0){
		perror("Get Information Error - VSCREENINFO!");
		return 0;
	}
	
	if(fvs.bits_per_pixel != SCREEN_BPP){
		perror("Unsupport Mode, 32Bpp Only!");	//return 0;
		return 0;
	}	

	if((touch_fd = open(TOUCH_SCREEN,O_RDONLY))<0){
		perror("Touch Screen Open Error");
		return 0;
	}

	/*
	if((keyboard_fd = open(KEYBOARD_EVENT,O_RDONLY))<0){
		perror("Keyboard Open Error");
		return 0;
	}

	if((hNFC = open(NFC_PORT,O_RDWR | O_NOCTTY))<0){
			perror("could not open NFC");
			return 0;
	}

	tcgetattr(hNFC,&oldtio);
	memset(&newtio,1,sizeof(newtio));
	memset(&newtio,1,sizeof(newtio));

	newtio.c_cflag = B115200|CS8|CLOCAL|CREAD;
	newtio.c_iflag = IGNPAR | ICRNL;
	newtio.c_oflag = OPOST | ONLCR;
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN] = 0;
	tcflush(hNFC,TCIFLUSH);
	tcsetattr(hNFC,TCSANOW,&newtio);
	printf("Initialized\n");

	wake_card();
	usleep(100000);
	read_ACK(15);
	usleep(100000);
	for(i=0;i<25;i++)
		receive_ACK[i] = 0;
		*/

	// Thread
	if((nRet = pthread_create(&threadID,NULL,pThreadFunc,NULL))!= 0){
		perror("pThread create error!\n");
		return 0;
	}
	if((nRet = pthread_create(&threadID2,NULL,pThreadTimer,NULL))!= 0){
		perror("pThread create error!\n");
		return 0;
	}
	

	return 1;
}

unsigned int button_init(){
	button_register.posX = 31;
	button_register.posY = 160;
	button_register.width = 453;
	button_register.height = 200;
	button_register.color = makePixel(195,195,195);
	strcpy(button_register.caption,"Register Card");

	button_check.posX = 542;
	button_check.posY = 160;
	button_check.width = 453;
	button_check.height = 200;
	button_check.color = makePixel(195,195,195);
	strcpy(button_check.caption,"Check");

	button_list.posX = 31;
	button_list.posY = 385;
	button_list.width = 964;
	button_list.height = 200;
	button_list.color = makePixel(195,195,195);
	strcpy(button_list.caption,"Show List");

	button_back.posX = 10;
	button_back.posY = 20;
	button_back.width = 60;
	button_back.height = 60;
	button_back.color = makePixel(120,120,120);
	strcpy(button_back.caption,"<<");
}

unsigned int touch_scan(int touchX,int touchY){
	t_button* button;
	int i;
	unsigned int ret =0;
	button = &button_register;
	ret = 1;
	if(touchX>=button->posX && touchX<=(button->posX+button->width)){
		if(touchY>=button->posY && touchY <= (button->posY + button->height)){
			return ret;
		}
	}

	button = &button_check;
	ret = 2;
	if(touchX>=button->posX && touchX<=(button->posX+button->width)){
		if(touchY>=button->posY && touchY <= (button->posY + button->height)){
			return ret;
		}
	}

	button = &button_list;
	ret = 3;
	if(touchX>=button->posX && touchX<=(button->posX+button->width)){
		if(touchY>=button->posY && touchY <= (button->posY + button->height)){
			return ret;
		}
	}

	button = &button_back;
	ret = 0;
	if(touchX>=button->posX && touchX<=(button->posX+button->width)){
		if(touchY>=button->posY && touchY <= (button->posY + button->height)){
			return ret;
		}
	}

	return 255;
}
unsigned int clear_Screen(){
	int repx,repy,offset;
	int fontSize =5;
	for(repy = 0; repy<fvs.yres;repy++){
		offset = repy*fvs.xres;
		for(repx=0;repx<fvs.xres;repx++){
			*(pfbdata+offset+repx) = 0;
		}
	}
	paint_rect2(button_back.posX,button_back.posY,button_back.width,button_back.height,button_back.color,1);
	paint_str(button_back.posX+5,button_back.posY+button_back.height/2-(fontSize*3),button_back.caption,strlen(button_back.caption),fontSize,3,makePixel(255,255,255),0,1);
}
unsigned int paint_dot(int x, int y, int width, U32 pixel,int full){
	int posx1 = x- width/2;
	int posy1 = y - width/2;
	int posx2 = x + width/2;
	int posy2 = y + width/2;

	int repy,repx,offset,dist;
	for(repy = posy1; repy<=posy2;repy++){
		offset = repy*fvs.xres;
		for(repx=posx1;repx<=posx2;repx++){
			dist = (repy-y)*(repy-y) + (repx-x)*(repx-x);
			if(offset+repx>= 1024*600 || (offset+repx)<=0) continue;
			if(full){
				if(dist <= (width*width)/4) *(pfbdata+offset+repx) = (int)pixel;	
			}else{
				if(dist <= (width*width)/4 && dist >= ((width)*(width)/5)) *(pfbdata+offset+repx) = (int)pixel;	
			}
		}
	}
}

unsigned int paint_rect(int x, int y, int width, U32 pixel,int full){
	int posx1 = x- width/2;
	int posy1 = y - width/2;
	int posx2 = x + width/2;
	int posy2 = y + width/2;

	int repy,repx,offset;
	int distx,disty;
	for(repy = posy1; repy<=posy2;repy++){
		offset = repy*fvs.xres;
		for(repx=posx1;repx<=posx2;repx++){
			distx = (repx-x)*(repx-x);
			disty = (repy-y)*(repy-y);
			
			if(offset+repx>= 1024*600 || (offset+repx)<0) continue;
			if(full){
				*(pfbdata+offset+repx) = (int)pixel;
			}else{
				if(distx >= (width*width)/5) *(pfbdata+offset+repx) = (int)pixel;
				if(disty >= (width*width)/5) *(pfbdata+offset+repx) = (int)pixel;
			}
		}
	}
}
unsigned int paint_rect2(int x, int y, int width,int height, U32 pixel,int full){
	int posx = x + width;
	int posy = y + height;

	int repy,repx,offset;
	int distx,disty;
	for(repy = y; repy<=posy;repy++){
		offset = repy*fvs.xres;
		for(repx=x;repx<=posx;repx++){
			distx = (repx-x)*(repx-x);
			disty = (repy-y)*(repy-y);
			
			if(offset+repx>= 1024*600 || (offset+repx)<0) continue;
			if(full){
				*(pfbdata+offset+repx) = (int)pixel;
			}else{
				if(repx==0 || repx==posx || repy ==0 || repy==posy) 
					*(pfbdata+offset+repx) = (int)pixel;
				//if(distx >= (width*width)/5) *(pfbdata+offset+repx) = (int)pixel;
				//if(disty >= (width*width)/5) *(pfbdata+offset+repx) = (int)pixel;
			}
		}
	}

}
unsigned int paint_font(int num,int x, int y){
	unsigned char* f = &System5x7[(GLCD_NUMBER+(num%10))*5];
	unsigned char data;
	int i=0,j=0;
	int width = 10;
	U32 pix[2];// = {{0,0,0},{100,100,0}};
	pix[0] = makePixel(0,0,0);
	pix[1] = makePixel(255,0,255);

	for(i =0 ; i<8;i++){
		for(j=0;j<GLCD_FONT_LEN;j++){
			data = f[j];
			if(data&(0x01<<i))paint_dot(x+j*width,y+i*width,width,pix[1],1);
		}
	}
}


unsigned int paint_str(int x, int y,char* str,int length,int fontSize,int fontpadding,U32 pixel,U32 bgColor,int transparent){
	unsigned char* f;
	unsigned char data;
	int len = strlen(str);
	int i=0,j=0,c=0;
	int width = fontSize;
	int height = fontSize;
	int padding = fontpadding;
	U32 pix[2];
	pix[0] = bgColor;
	pix[1] = pixel;
	
	for(c=0;c<length;c++){
		if((c<((length-len)/2)) || c>=(length-len)/2+len){
			f = &System5x7[(' '+GLCD_CHAR_OFFSET)*5];
		}
		else{		
			f = &System5x7[(str[c-(length-len)/2]+GLCD_CHAR_OFFSET)*5];
			if(str[c]=='\n'){
				y+=height*8+padding*8;
				x-=(c*(padding+width*5))+(width*GLCD_FONT_LEN+1);
				continue;	
			}
		}
		for(i =0 ; i<8;i++){
			for(j=0;j<GLCD_FONT_LEN;j++){
				data = f[j];
				if(data&(0x01<<i))	paint_rect(x+(j*width-1)+(c*(width*5+padding)),y+(i*height-1),width,pix[1],1);
				else if(!transparent){
					paint_rect(x+(j*width-1)+(c*(padding+width*5)),y+(i*height-1),width,pix[0],1);
				}
				
			}
		}
	}
}


unsigned int paint_mario(int x, int y,int Size,int draw){
	unsigned char* f =(unsigned char*) Mario_map;
	unsigned char data;
	int px =0, py =0;
	int width = Size;
	int height = Size;
	U32 pix;

	for(px =0; px<MARIO_SIZE_X;px++){
		for(py=0;py<MARIO_SIZE_Y;py++){
			data = f[py*MARIO_SIZE_X+px];
			if(draw) pix = Mario_Color[data];
			else pix = makePixel(0,0,0);
			if(data != 2) paint_rect(x+((MARIO_SIZE_X-1-px)*width-1),y+(py*height-1),width,pix,1);
		}
	}
}
unsigned int move_mario(unsigned char* push_data){
		if(push_data[1] == 1){
			if(lastPush == 0) mario_y = (mario_y - MARIO_STEP)<0 ? 0 : mario_y - MARIO_STEP;
			lastPush = 1;
		}
		else if(push_data[7] == 1){
			if(lastPush == 0) mario_y = (mario_y + MARIO_STEP)>(fvs.yres-MARIO_SIZE_Y) ? fvs.yres - MARIO_SIZE_Y : mario_y + MARIO_STEP;
			lastPush = 1;
		}
		else if(push_data[3] == 1){
			if(lastPush == 0) mario_x = (mario_x - MARIO_STEP)<0 ? 0 : mario_x - MARIO_STEP;
			lastPush = 1;
		}
		else if(push_data[5] == 1){
			if(lastPush == 0) mario_x = (mario_x + MARIO_STEP)>(fvs.xres-MARIO_SIZE_X) ? fvs.xres - MARIO_SIZE_X : mario_x + MARIO_STEP;
			lastPush = 1;
		}
		else lastPush = 0;

		if(mario_x_prev != mario_x || mario_y_prev != mario_y) 
			paint_mario(mario_x_prev,mario_y_prev,6,0);
		paint_mario(mario_x,mario_y,6,1);
		mario_x_prev = mario_x;
		mario_y_prev = mario_y;
}

unsigned int setTable(){
	int cell;
	table1.posX = 12;
	table1.posY = 130;
	table1.rows = 7;
	table1.cols = 3;
	table1.width = 1000;
	table1.height = 450;
	table1.borderWidth = 4;
	table1.fontSize = 3;
	table1.borderColor = makePixel(50,50,255);
	table1.bgColor = makePixel(255,255,255);
	table1.fontColor = makePixel(0,0,0);
	table1.str = (char**)malloc(table1.rows*table1.cols*sizeof(char *));
	for(cell =0; cell<table1.rows*table1.cols;cell++){
		table1.str[cell] = (char *)malloc(100 * sizeof(char));
	}
	made_checkboard();
}

unsigned int paint_table(t_table t){
	int r,c;
	int h,w;
	int offsetX = t.posX;
	int offsetY = t.posY;
	int bw = t.borderWidth;
	int cWidth = t.width/t.cols;
	int cHeight = t.height/t.rows;

	char strBuf[100];
	time_t tt;
	clear_Screen();
	if(cWidth*t.cols<t.width){
		cWidth++;
		t.width++;
	}
	if(cHeight*t.rows<t.height){
		cHeight++;
		t.height++;
	}
	time(&tt);
	sprintf(strBuf,"#Day:%s",user_check.name);
	paint_str(100,80,strBuf,strlen(strBuf),3,3,makePixel(255,0,0),0,0);

	//Draw Border
	for(h=0; h<=t.height;h++){
		for(w =0; w<=t.width;w++){
			if(h<=cHeight)
				paint_dot(offsetX+w,offsetY + h,1,makePixel(0,155,0),1);
			else
				paint_dot(offsetX+w,offsetY + h,1,t.bgColor,1);
		}
	}
	for(h=0; h<=t.height;h++){
		for(w =0; w<=t.width;w++){
			if(((h%(cHeight) ==0)|| (w%(cWidth)==0)) || (h==t.height || w==t.width))
				paint_dot(offsetX+w,offsetY + h,bw,t.borderColor,1);
		}
	}
	// Draw String data
	for(r=0;r<t.rows;r++){
		for(c=0;c<t.cols;c++){
			char* s = t.str[r*t.cols + c];
			
			paint_str(2*bw+offsetX+(c*cWidth),bw*2+offsetY+(r*cHeight),s,strlen(s),t.fontSize,1,t.fontColor,0,1);
			printf("[%d,%d]%s\n",r,c,s);
		}
	}
}

unsigned int paint_menu(){
	int fontSize = 5;
	clear_Screen();

	
	// Paint Button Frame
	paint_rect2(button_register.posX,button_register.posY,button_register.width,button_register.height,button_register.color,1);

	paint_rect2(button_check.posX,button_check.posY,button_check.width,button_check.height,button_check.color,1);

	paint_rect2(button_list.posX,button_list.posY,button_list.width,button_list.height,button_list.color,1);
	
	paint_rect2(button_back.posX,button_back.posY,button_back.width,button_back.height,button_back.color,1);


	//Paint Button Caption
	paint_str(button_register.posX+10,button_register.posY+button_register.height/2-(fontSize*7),button_register.caption,15,fontSize,3,makePixel(0,0,0),0,1);
	paint_str(button_check.posX+10,button_check.posY+button_check.height/2-(fontSize*7),button_check.caption,15,fontSize,3,makePixel(0,0,0),0,1);
	paint_str(button_list.posX+10,button_list.posY+button_list.height/2-(fontSize*7),button_list.caption,32,fontSize,3,makePixel(0,0,0),0,1);
	paint_str(button_back.posX+5,button_back.posY+button_back.height/2-(fontSize*3),button_back.caption,strlen(button_back.caption),fontSize,3,makePixel(255,255,255),0,1);
	//paint_str(int x, int y,char* str,int length,int fontSize,int fontpadding,U32 pixel,int transparent);
}

unsigned int paint_register(){
	char str[100];
	//paint_rect(int x, int y, int width, U32 pixel,int full);
	paint_rect2(button_register.posX,button_register.posY,button_list.width,430,makePixel(255,255,255),1);
}

////////////////////////////////////////// NFC JaeSeock
void made_checkboard(void){
    struct _user_info user_info ={{0,},{0,},0};
    char flag = 0;
    char i = 0;
	char* userName;
	char* userID;
	char* userCheck;
	char data[30];
	int idx=0,j=0;

    
    if(!open_file(&user_data, "r"))
        return;
    if(!open_file(&user_check, "r")){
        open_file(&user_check, "w+");

        fprintf(user_check.user_file, "Name\t\t\tid\t\t\tCheck\n");
        while(!feof(user_data.user_file))
        {
            fgets(user_data.line, 100, user_data.user_file);
            sscanf(user_data.line, "%s\t\t\t%s", user_info.name, user_info.id);
            if(!feof(user_data.user_file))
                fprintf(user_check.user_file, "%s\t\t\t%s\t\t\tX\n", user_info.name, user_info.id);
            for (i = 0; i < 20; i++){
                user_info.name[i] = 0;
                user_info.id[i] = 0;
            }
        }
    }
    rewind(user_check.user_file);
    while(!feof(user_check.user_file)){
        fgets(user_check.line, 100, user_check.user_file);
        if(!feof(user_check.user_file)){
            //printf("%s",user_check.line);
			userName =(char*)malloc(100*sizeof(char));
			userID=(char*)malloc(100*sizeof(char));
			userCheck =(char*)malloc(3*sizeof(char));

			sscanf(user_check.line,"%s\t\t\t%s\t\t\t%s\n",userName,data,userCheck);
			
			if(idx>0){ 
				sprintf(userID,"%d",data[j]); 
				for(j=1;j<10;j++){ 
					sprintf(userID,"%s %d",userID,data[j]);
				}
				userID[j] = 0x00;
			}
			else{
				sprintf(userID,"%s",data); 
			}
			table1.str[idx*3] = userName;
			table1.str[idx*3+1] = userID;
			table1.str[idx*3+2] = userCheck;
			//printf("%s, %s, %s\n", userName,userID,userCheck);
			//printf("%d %d %d %d %d %d \n",userName[0],userName[1],userName[2],userName[3],userName[4],userName[5]);
			idx++;
		}
    }
    fclose(user_data.user_file);
    fclose(user_check.user_file);
}

char open_file(U_file* pU_file,char *mode){
	pU_file->user_file = fopen(pU_file->name,mode);
	if(pU_file->user_file == NULL){
		printf(" Error : %s file open error\n",pU_file->name);
		return 0;
	}
	return 1;
}

void add_user(void){
	char exist_name_flag = 1;
	char add_Name[30] ={0,};
	char ack_buff1[5]	={0,};
	char ack_buff2[5]	={0,};
	char check_arr[5]	={0,};
	char flag = 0;
	char exitFlag = 0;
	signed char count = 30;

	char strBuf[30];

	char i=0;

	open_file(&user_data,"a+");
	if((open_file(&user_check,"r"))){
		fclose(user_check.user_file);
		open_file(&user_check,"a+");
		flag =1;
	}

	printf("Tagging Please\n");
	while(!exitTouch){
		if(count>0){
			sprintf(strBuf,"%d",count);
			printf("Count : %d\n",count--);
			paint_str(462,370,strBuf,5,10,0,makePixel(0,0,0),makePixel(255,255,255),0);
		}
		else{
			exitFlag =1;
			break;
		}

		send_tag();
		for (i=0;i<25;i++)
			receive_ACK[i] = 0;
		read_ACK(25);
		for(i=0;i<5;i++)
			ack_buff1[i] = receive_ACK[i+19];
		usleep(100000);

		send_tag();
		for (i=0;i<25;i++)
			receive_ACK[i] = 0;
		read_ACK(25);
		for(i=0;i<5;i++)
			ack_buff2[i] = receive_ACK[i+19];

		if(!strcmp(ack_buff1,check_arr)){
			for(i=0;i<5;i++){
				id_buff[i] = ack_buff2[i];
				id_buff[i+5] = ack_buff1[i];
			}
		}
		else{
			for(i=0;i<5;i++){
				id_buff[i] = ack_buff1[i];
				id_buff[i+5] = ack_buff2[i];
			}
		}
		usleep(100000);

		if(check_id()){
			while(!feof(user_data.user_file)){
				fgets(user_data.line,100,user_data.user_file);
				if(strstr(user_data.line,id_buff) != NULL){
					printf("already exist ID : %s \n",user_data.line);
					char tmp[30];
					int n;
					strcpy(tmp,user_data.line);
					for(n=0;tmp[n]!='\t';n++);
					tmp[n] = '\0';
					
					sprintf(strBuf,"already exist ID : %s \n",tmp);
					paint_rect2(button_register.posX,button_register.posY,button_list.width,430,makePixel(255,255,255),1);
					paint_str(200,370,strBuf,strlen(strBuf),5,1,makePixel(0,0,0),makePixel(255,255,255),0);

					exist_name_flag=0;
					//getchar();
					break;
				}
			}
			if(exist_name_flag){
				printf("add Name : \n");
				sprintf(strBuf,"add Name :");
				paint_rect2(button_register.posX,button_register.posY,button_list.width,430,makePixel(255,255,255),1);
				paint_str(400,370,strBuf,strlen(strBuf),3,0,makePixel(0,0,0),makePixel(255,255,2550),0);
				//gets(add_Name);
				key_read(add_Name);
				fprintf(user_data.user_file,"%s\t\t\t%s\n", add_Name,id_buff);
				if(flag) fprintf(user_check.user_file,"%s\t\t\t%s\t\t\tX\n",add_Name,id_buff);
			}
			break;
		}
	}
	read_ACK(25);
	for(i=0;i<100;i++){
		id_buff[i] = 0;
	}

	fclose(user_data.user_file);
	if(flag) fclose(user_check.user_file);

	paintOnce = 1;
	state = 0;
	if(!exitFlag && !exitTouch)usleep(1500000);
	exitTouch = 0;
}

void read_ACK(unsigned char temp){
	unsigned char i;
	for (i=0;i<temp;i++){
		if(read(hNFC,&temp_data,sizeof(temp_data)))
			receive_ACK[i] = temp_data;
	}
}
void callname_user(void){
	char exist_name_flag = 0;
	char add_Name[30] ={0,};
	char ack_buff1[5]	={0,};
	char ack_buff2[5]	={0,};
	char check_arr[5]	={0,};
	char flag = 0;
	signed char count = 30;
	char strBuf[30];
	char i=0;

	struct _user_info tagging_data = {{0,},{0,},0};
	struct _user_info user_check_data = {{0,},{0,},'X'};

    if(!open_file(&user_check, "r+")){
        printf("There is no attendance\n");
        return;
    }

    rewind(user_check.user_file);
    if(!feof(user_check.user_file))
        fgets(user_check.line, 100,user_check.user_file);
    while (!exitTouch)
    {
        if(count>0){
            sprintf(strBuf,"Tagging plz %d", count--);
			paint_str(300,370,strBuf,strlen(strBuf)+4,3,0,makePixel(0,0,0),makePixel(255,255,255),0);
        }
        else break;

        send_tag();
        for (i = 0; i < 25; i++)
            receive_ACK[i] = 0;
        read_ACK(25);
        for (i = 0; i < 5; i++)
            ack_buff1[i] = receive_ACK[i + 19];
        usleep(100000);

        send_tag();
        for (i = 0; i < 25; i++)
            receive_ACK[i] = 0;
        read_ACK(25);
        for (i = 0; i < 5; i++)
            ack_buff2[i] = receive_ACK[i + 19];

        if (!strcmp(ack_buff1, check_arr)){
            for (i = 0; i < 5; i++){
                id_buff[i] = ack_buff2[i];
                id_buff[i + 5] = ack_buff1[i];
            }
        }
        else{
            for (i = 0; i < 5; i++){
                id_buff[i] = ack_buff1[i];
                id_buff[i + 5] = ack_buff2[i];
            }
        }
        usleep(100000);

        if (check_id())
        {
            while (!feof(user_check.user_file)){
                fgets(user_check.line, 100, user_check.user_file);
                sscanf(user_check.line, "%s\t\t\t%s\t\t\t%s\n", user_check_data.name, user_check_data.id, check_arr);
                if(strstr(user_check.line,id_buff) != NULL){
                    if (check_arr[0] == 'O'){
                        sprintf(strBuf,"Alread checked");
                        read_ACK(25);
                        for (i = 0; i < 100; i++){
                            id_buff[i] = 0;
                        }
                        fclose(user_check.user_file);
						paint_rect2(button_register.posX,button_register.posY,button_list.width,430,makePixel(255,255,255),1);
						paint_str(200,370,strBuf,strlen(strBuf),5,1,makePixel(0,0,0),makePixel(255,255,255),0);

						usleep(1500000);
                        return;
                    }

                    fseek(user_check.user_file,-2,SEEK_CUR);
                    fwrite("O", 1, 1, user_check.user_file);
                    // rewind(user_check.user_file);
                    exist_name_flag = 1;
                    break;
                }
            }
        }
        if(exist_name_flag){
            sprintf(strBuf,"%s checked!",user_check_data.name);
			paint_rect2(button_register.posX,button_register.posY,button_list.width,430,makePixel(255,255,255),1);
			paint_str(200,370,strBuf,strlen(strBuf),5,1,makePixel(0,0,0),makePixel(255,255,255),0);
			usleep(1500000);
            break;
        }
	}
    read_ACK(25);
    for (i = 0; i < 100; i++){
        id_buff[i] = 0;
    }
    fclose(user_check.user_file);
	exitTouch = 0;
}

void send_tag(void){
	unsigned char i;
	for(i=0;i<11;i++){
		if(write(hNFC,&tag[i],sizeof(tag[i])));
	}
}

char  check_id(void){
	char i;
	for(i=0;i<10;i++){
		if(id_buff[i] !=  0)
			return 1;
	}
	return 0;
}

void wake_card(){
	unsigned char i;
	for(i=0;i<24;i++)
		write(hNFC,&wake[i],sizeof(wake[i]));
}


void *pThreadFunc(void * arg)
{
	while(1){

		if(read(touch_fd,touch_iev,sizeof(struct input_event)*3) <0){
			perror("Error : could not read touch");
			continue;
		}
		if(touch_iev[0].type ==1 && touch_iev[1].type == 3 && touch_iev[2].type == 3){
			int touchX = touch_iev[1].value;
			int touchY = touch_iev[2].value;
			unsigned int ret = touch_scan(touchX,touchY);
			if(ret != 255){
				exitTouch= 0;
				if(ret == 0) 
					exitTouch = 1;
				if(state==0){
					state = ret;
				}
				else if(ret == 0) state = 0;
				paintOnce = 1;

				//if(state==0 && ret == 0) break;

			}
			printf("Touched %d,%d\n",touchX,touchY);
		}
	}

}
void *pThreadTimer(void * arg){

	time_t tt;
	char strBuf[100];
	while(1){
		// dateTime
		time(&tt);
		sprintf(strBuf,"Now :%s",ctime(&tt));
		//paint_str(100,50,strBuf,strlen(strBuf),3,3,makePixel(0,255,0),0,0);
	}
}





unsigned int key_read(char* buf){
	char arr[200];
	int idx=0;
	char c=0;

	while(c != '\n'){
		read(keyboard_fd,&arr,sizeof(arr));
		read(keyboard_fd,&arr,sizeof(arr));
		c = key_map(arr);
		if(c== 0){

			;
		}
		else if(c == '\b'){
			buf[idx-1] = '\0';
			idx--;
		}
		else if(c == '\n' || c=='\r') buf[idx] = '\0';
		else{
			buf[idx++] = c;
		}
		paint_str(560,370,buf,idx+1,3,1,makePixel(0,0,0),makePixel(255,255,255),0);
	}
}
char key_map(char* arr){
	char data =0;
	if(arr[12] == 30)
		data = '1';
	else if(arr[12] == 31)
		data = '2';	
	else if(arr[12] == 32)
		data = '3';
	else if(arr[12] == 33)
		data = '4';		
	else if(arr[12] == 34)
		data = '5';
	else if(arr[12] == 35)
		data = '6';	
	else if(arr[12] == 36)
		data = '7';
	else if(arr[12] == 37)
		data = '8';
	else if(arr[12] == 38)
		data = '9';
	else if(arr[12] == 39)
		data = '0';
	else if(arr[12] == 42)
		data = '\b';
	else if(arr[12] == 20)
		data = 'q';
	else if(arr[12] == 26)
		data = 'w';	
	else if(arr[12] == 8)
		data = 'e';
	else if(arr[12] == 21)
		data = 'r';		
	else if(arr[12] == 23)
		data = 't';
	else if(arr[12] == 28)
		data = 'y';	
	else if(arr[12] == 24)
		data = 'u';
	else if(arr[12] == 12)
		data = 'i';
	else if(arr[12] == 18)
		data = 'o';
	else if(arr[12] == 19)
		data = 'p';
	else if(arr[12] == 4)
		data = 'a';
	else if(arr[12] == 22)
		data = 's';
	else if(arr[12] == 7)
		data = 'd';	
	else if(arr[12] == 9)
		data = 'f';
	else if(arr[12] == 10)
		data = 'g';		
	else if(arr[12] == 11)
		data = 'h';
	else if(arr[12] == 13)
		data = 'j';	
	else if(arr[12] == 14)
		data = 'k';
	else if(arr[12] == 15)
		data = 'l';
	else if(arr[12] == 29)
		data = 'z';
	else if(arr[12] == 27)
		data = 'x';
	else if(arr[12] == 6)
		data = 'c';
	else if(arr[12] == 25)
		data = 'v';
	else if(arr[12] == 5)
		data = 'b';	
	else if(arr[12] == 17)
		data = 'n';
	else if(arr[12] == 16)
		data = 'm';		
	else if(arr[12] == 44)
		data = ' ';
	else if(arr[12] == 40)
		data = '\n';	
	else if(arr[12] == 41)
		;
	//	quit = 1;
	else
		data = ' ';
	return data;
}
