#define MARIO_SIZE_X 12
#define MARIO_SIZE_Y 16

#define MARIO_COLOR_RED		1	//(0xFF0000)
#define MARIO_COLOR_WHITE	2	//(0xFFFFFF)	
#define MARIO_COLOR_BLUE	3	//(0x0000FF)
#define MARIO_COLOR_YELLOW	4	//(0xFFFF00)
#define MARIO_COLOR_BROWN	5	//(0x996611)

const unsigned int Mario_Color[] ={
	0x000000,
	//0xFF0000,
	0xE52521,
	//0xFFFFFF,
	0x000000,
	//0x0000FF,
	0x045CFF,
	//0xFFFF00,
	0xFBD000,
	0x996611
};


const unsigned char Mario_map[MARIO_SIZE_Y][MARIO_SIZE_X] = {
		{2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2},
		{2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2},
		{2, 2, 3, 3, 3, 4, 4, 3, 4, 2, 2, 2}, 
		{2, 3, 4, 3, 4, 4, 4, 3, 4, 4, 4, 2}, 
		{2, 3, 4, 3, 3, 4, 4, 4, 3, 4, 4, 4}, 
		{2, 3, 3, 4, 4, 4, 4, 3, 3, 3, 3, 2}, 
		{2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 2, 2}, 
		{2, 2, 3, 3, 1, 3, 3, 3, 2, 2, 2, 2}, 
		{2, 3, 3, 3, 1, 3, 3, 1, 3, 3, 3, 2}, 
		{3, 3, 3, 3, 1, 1, 1, 1, 3, 3, 3, 3}, 
		{4, 4, 3, 1, 4, 1, 1, 4, 1, 3, 4, 4}, 
		{4, 4, 4, 1, 1, 1, 1, 1, 1, 4, 4, 4}, 
		{4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4}, 
		{2, 2, 1, 1, 1, 2, 2, 1, 1, 1, 2, 2}, 
		{2, 3, 3, 3, 2, 2, 2, 2, 3, 3, 3, 2}, 
		{3, 3, 3, 3, 2, 2, 2, 2, 3, 3, 3, 3}
};


