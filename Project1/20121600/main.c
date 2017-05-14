#include"header.h"

void(*func_ptr[5])()={clock_mode0,counter_mode1,text_editor_mode2,draw_board_mode3,alarm_mode4};	//Fucntion pointer of 5 modes
int mode,quit_state;
int key_id_input,key_id_output;
	
int main(void){
	pid_t pid_input,pid_output;
	mode=0;	quit_state=1;
	key_id_input=msgget((key_t)INPUT_TO_MAIN,IPC_CREAT|0666);	//Create message queue input process -> main process
	key_id_output=msgget((key_t)MAIN_TO_OUTPUT,IPC_CREAT|0666);	//Create message queue main process  -> output process

	dev_fpga=open("/dev/fpga_push_switch",O_RDWR|O_NONBLOCK);
	dev_readkey=open("/dev/input/event0",O_RDONLY|O_NONBLOCK);	//Device file open
	
	pid_input=fork();
	if(pid_input){
		pid_output=fork();
		if(!pid_output)
			output_process();	
	}
	if(!pid_input){
		input_process();
		wait();				//input process waits until output process die
	}
	else{
		main_process();
		wait();				//main waits until input process die
	}
	close(dev_fpga);	close(dev_readkey);
	return 0;
}

void main_process(){
	struct msgbuf snd_msg,rcv_msg;
	int msgbuf_size=sizeof(struct msgbuf)-sizeof(long);
	key_t key_id_output=msgget((key_t)MAIN_TO_OUTPUT,IPC_CREAT|0666);
	
	while(quit_state)	//Keep loop until BACK key input
		func_ptr[mode]();
	
	snd_msg.msgtype=READKEY;		snd_msg.data=0;			//Add output initialize code
	msgsnd(key_id_output,&snd_msg,msgbuf_size,IPC_NOWAIT);	//When Back key pressed
}


/************************************************************************/
/*																		*/
/*					clock mode0 function								*/
/*rcv_msg : message from input process and it contains which sw has		*/
/*			input. (msgtype = READKEY)									*/
/*snd_msg : message for send to output process. snd_msg.data represents	*/
/*			which led should be lighten up. And snd_msg.buffer[4] 		*/
/*			contains information of FND device output.					*/
/*			snd_msg.mode=0												*/
/*																		*/
/************************************************************************/


void clock_mode0(){
	struct msgbuf rcv_msg;
	struct msgbuf_output snd_msg;
	int msgbuf_size=sizeof(struct msgbuf)-sizeof(long);
	int msgbuf_output_size=sizeof(struct msgbuf_output)-sizeof(long);
	int modify=0;
	time_t before=0,after=0,current_time=time(NULL);
	snd_msg.mode=0;	snd_msg.msgtype=FPGA_SWITCH;	snd_msg.data=128;
	TIME_CALCULATION_MACRO
	while(1){
		current_time%=86400;	
		after=time(NULL)&1;		//variable 'after' represents the time is even or odd
		usleep(100000);
		if(!(time(NULL)%60)){	//when real time changed by minute
			TIME_CALCULATION_MACRO
		}
		if(before^after){		//Every seconds it activated
			before=after;
			if(modify){			//modify is flag of point blinking or not
				snd_msg.data=(after<<5)+(!after<<4);	//send 32 or 16 to led device
				msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
			}
		}
		if(msgrcv(key_id_input,&rcv_msg,msgbuf_size,READKEY,IPC_NOWAIT)>0){	//Gpio input
			READKEY_MACRO
			break;
		}
		else if(msgrcv(key_id_input,&rcv_msg,msgbuf_size,FPGA_SWITCH,IPC_NOWAIT)>0){
			if((rcv_msg.data==SW_1)&&!modify){	//SW_1
				modify=1;	snd_msg.data=16;
				continue;
			}
			if(modify){
				switch(rcv_msg.data){
					case 1:	modify=0;
							snd_msg.data=128;
							TIME_CALCULATION_MACRO
							break;
					case 2:	current_time=time(NULL);	//SW_2
							TIME_CALCULATION_MACRO
							break;
					case 4: current_time+=3600;			//SW_3 (plus 1hour)
							TIME_CALCULATION_MACRO
							break;
					case 8:	current_time+=60;			//SW_4 (plus 1minute)
							TIME_CALCULATION_MACRO
							break;
				}
			}
		}
	}	
}


/************************************************************************/
/*																		*/
/*					counter mode1 function								*/
/*rcv_msg : message from input process and it contains which sw has		*/
/*			input. (msgtype = READKEY)									*/
/*snd_msg : message for send to output process. snd_msg.data represents	*/
/*			which led should be lighten up.(when decimal, second led	*/
/*			will be lighten) snd_msg.buffer contains information of FND	*/
/*			device output.												*/
/*			snd_msg.mode=1												*/
/*																		*/
/************************************************************************/



void counter_mode1(){
	int count=0,decimal[4]={10,8,4,2},square[4]={100,64,16,4},triple[4]={1000,512,64,8},led[4]={64,32,16,128},index=0;
	int msgbuf_size=sizeof(struct msgbuf)-sizeof(long),temp,final_value;
	int msgbuf_output_size=sizeof(struct msgbuf_output)-sizeof(long);
	struct msgbuf rcv_msg;
	struct msgbuf_output snd_msg;
	memset(snd_msg.buffer,0,4);	snd_msg.msgtype=FPGA_SWITCH; snd_msg.data=64;	snd_msg.mode=1;
	msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
	while(1){
		usleep(100000);
		if(msgrcv(key_id_input,&rcv_msg,msgbuf_size,READKEY,IPC_NOWAIT)>0){
			READKEY_MACRO
			break;
		}
		else if(msgrcv(key_id_input,&rcv_msg,msgbuf_size,FPGA_SWITCH,IPC_NOWAIT)>0){
			switch(rcv_msg.data){
				case SW_1 : index++; index&=3;	break;		// 10 -> 8 -> 4 -> 2 -> 10
				case SW_2 : count+=square[index];	break;	// Add third digit(10^2,8^2,4^2,2^2)
				case SW_3 : count+=decimal[index];	break;	// Add second digit
				case SW_4 : count++;	break;				// Add first digit
			}
			count%=1000;	snd_msg.data=led[index];	
			snd_msg.buffer[0]=count/triple[index]%decimal[index];
			snd_msg.buffer[1]=count%triple[index]/square[index];
			snd_msg.buffer[2]=count%square[index]/decimal[index];
			snd_msg.buffer[3]=count%decimal[index];		//Assign buffer for fnd output device
			msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
		}
	}
}


/************************************************************************/
/*																		*/
/*					tesxt editor mode2 function							*/
/*rcv_msg : message from input process and it contains which sw has		*/
/*			input. (msgtype = READKEY)									*/
/*snd_msg : snd_msg.buffer[0];	cursor position							*/
/*			snd_msg.buffer[1];	character that display in text lcd device*/
/*			snd_msg.buffer[2];	clear(1) or not(0)						*/
/*			snd_msg.buffer[3];	integer input mode(1) or character		*/
/*								input mode(0)							*/
/*			snd_msg.data;	count										*/
/*			snd_msg.mode=2												*/
/*																		*/
/************************************************************************/


void text_editor_mode2(){
	int msgbuf_size=sizeof(struct msgbuf)-sizeof(long);
	int msgbuf_output_size=sizeof(struct msgbuf_output)-sizeof(long);
	struct msgbuf rcv_msg;	struct msgbuf_output snd_msg;
	const char key_pad[9][3]={{'.','Q','Z'},{'A','B','C'},{'D','E','F'},{'G','H','I'},{'J','K','L'},{'M','N','O'},{'P','R','S'},{'T','U','V'},{'W','X','Y'}};							//key pad value
	int cur_x=-1,cur_y,cursor=-1,count=0,char_or_int=0;
	snd_msg.msgtype=FPGA_SWITCH;
	snd_msg.mode=2;	snd_msg.data=0;	snd_msg.buffer[0]=0;	//cursor position initialize to 0
									snd_msg.buffer[1]=0;	//
									snd_msg.buffer[2]=1;	//clear to initialize lcd text buffer
									snd_msg.buffer[3]=0;	//integer(1) or character(0)

	msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
	while(1){
		usleep(100000);
		if(msgrcv(key_id_input,&rcv_msg,msgbuf_size,READKEY,IPC_NOWAIT)>0){
			READKEY_MACRO
			break;
		}
		else if(msgrcv(key_id_input,&rcv_msg,msgbuf_size,FPGA_SWITCH,IPC_NOWAIT)>0){
			count++;	snd_msg.buffer[2]=0;	count%=10000;
			if(cursor>31)
				cursor=31;
			if(!char_or_int){			//When input is alphabet
				switch(rcv_msg.data){
					case SW_1:	TEXT_EDITOR_MACRO(0)	
					case SW_2: TEXT_EDITOR_MACRO(1)
					case SW_3: TEXT_EDITOR_MACRO(2)
					case SW_4: TEXT_EDITOR_MACRO(3)
					case SW_5:TEXT_EDITOR_MACRO(4)
					case SW_6:TEXT_EDITOR_MACRO(5)
					case SW_7:TEXT_EDITOR_MACRO(6)
					case SW_8:TEXT_EDITOR_MACRO(7)
					case SW_9:TEXT_EDITOR_MACRO(8)
					case SW_2AND3: 	//clear key
						snd_msg.buffer[2]=1;	cursor=-1;	cur_x=-1;	break;
					case SW_5AND6: 	//convert ascii to int
						cur_x=-1;	snd_msg.buffer[3]=char_or_int=1;	break;
					case SW_8AND9:	//spacebar
						cursor++;	snd_msg.buffer[1]=' ';			break;
				}
				snd_msg.buffer[0]=cursor;	
			}
			else{						//When input is integer
				switch(rcv_msg.data){
					case SW_1:		snd_msg.buffer[1]='1';	break;
					case SW_2:		snd_msg.buffer[1]='2';	break;
					case SW_3:		snd_msg.buffer[1]='3';	break;
					case SW_4:		snd_msg.buffer[1]='4';	break;
					case SW_5:	snd_msg.buffer[1]='5';	break;
					case SW_6:	snd_msg.buffer[1]='6';	break;
					case SW_7:	snd_msg.buffer[1]='7';	break;
					case SW_8:	snd_msg.buffer[1]='8';	break;
					case SW_9:	snd_msg.buffer[1]='9';	break;
					case SW_2AND3:		snd_msg.buffer[2]=1;	cursor=-2;	cur_x=-2;	break;
					case SW_5AND6:	cur_x=-1;	snd_msg.buffer[3]=char_or_int=0;	break;
					case SW_8AND9:	snd_msg.buffer[1]=' ';	break;
				}
				snd_msg.buffer[0]=++cursor;
			}
			snd_msg.data=count;	
			msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
		}
	}
}


/************************************************************************/
/*																		*/
/*					draw board mode3 function							*/
/*rcv_msg : message from input process and it contains which sw has		*/
/*			input. (msgtype = READKEY)									*/
/*snd_msg : snd_msg.buffer[0];	x coordinate(64,32,16,8,4,2,1)			*/
/*			snd_msg.buffer[1];	y coordinate							*/
/*			snd_msg.buffer[2];	2 = convert		1 = clear				*/
/*			snd_msg.buffer[3];	4,2 = blinking value  1 = when selected	*/
/*			snd_msg.data;		count									*/
/*			snd_msg.mode=3												*/
/*																		*/
/************************************************************************/


void draw_board_mode3(){
	int msgbuf_size=sizeof(struct msgbuf)-sizeof(long);
	int msgbuf_output_size=sizeof(struct msgbuf_output)-sizeof(long);
	struct msgbuf rcv_msg;	struct msgbuf_output snd_msg;
	time_t before=0,after=0;
	int cur_x=0,cur_y=0,current_val=0,blink_flag=1,count=0;
	int x_val[7]={64,32,16,8,4,2,1};	snd_msg.data=0;					//count
	snd_msg.mode=3;	snd_msg.msgtype=FPGA_SWITCH;	snd_msg.buffer[0]=64;	//x coordinate
													snd_msg.buffer[1]=0;	//y coordinate
													snd_msg.buffer[2]=1;	//2:convert (0 or 1), 1:clear (0 or 1)
													snd_msg.buffer[3]=0;	//odd or even 4: fake value 2 , real value  1
	msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
	while(1){
		usleep(100000);	after=time(NULL)&1;							//after = odd or even
		count%10000;	snd_msg.data=count;							//counting
		snd_msg.buffer[0]=x_val[cur_x];	snd_msg.buffer[1]=cur_y;	//initialize its position to (0,0)
		snd_msg.buffer[2]=0;										
		snd_msg.buffer[3]=0;
		if(blink_flag){			//when blinking
			if(before^after){
				before=after;
				snd_msg.buffer[3]=2+(before<<2);
				msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
			}
		}
		if(msgrcv(key_id_input,&rcv_msg,msgbuf_size,READKEY,IPC_NOWAIT)>0){
			READKEY_MACRO
			break;
		}
		else if(msgrcv(key_id_input,&rcv_msg,msgbuf_size,FPGA_SWITCH,IPC_NOWAIT)>0){
			count++;
			switch(rcv_msg.data){
				case SW_1:	blink_flag=1;						//Initialize all(coordinates, value and blingking_flag)
							snd_msg.buffer[0]=64;	cur_x=0;
							cur_y=snd_msg.buffer[1]=0;	
							snd_msg.buffer[2]=1;	
							msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
							break;
				case SW_2:	if(!cur_y)					//Upper arrow
								break;				
							else{	
								cur_y--;	
								snd_msg.buffer[1]=cur_y;
							}
							msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
							break;
				case SW_3:	blink_flag^=1;				//Blink option
							snd_msg.buffer[3]=2;
							msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
							break;			
				case SW_4:	if(!cur_x)					//Left arrow
								break;				
							else{	
								cur_x--;	
								snd_msg.buffer[0]=x_val[cur_x];	
								msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
								break;
							}
				case SW_5:		//Select
							snd_msg.buffer[3]=1;			
							msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
							break;
				case SW_6:	if(cur_x==6)			//Right arrow
								break;		
							else{	
								cur_x++;	
								snd_msg.buffer[0]=x_val[cur_x];	
								msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
								break;
							}
				case SW_7:	snd_msg.buffer[2]=1;			//Clear
							msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
							break;
				case SW_8:	if(cur_y==9)			//Down arrow
								break;		
							else{	
								cur_y++;	
								snd_msg.buffer[1]=cur_y;	
								msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
								break;
							}
				case SW_9:	snd_msg.buffer[2]=2;			//Convert
							msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
							break;	
			}
		}
	}
}
void alarm_mode4(){
	int msgbuf_size=sizeof(struct msgbuf)-sizeof(long);
	int msgbuf_output_size=sizeof(struct msgbuf_output)-sizeof(long);
	struct msgbuf rcv_msg;	struct msgbuf_output snd_msg;
	int status=ALARM_SETTING;	time_t current_time,alarm_time=0;
	int hour=12,min=0;
	snd_msg.buffer[0]=0;	snd_msg.buffer[1]=0;	snd_msg.buffer[2]=0;	snd_msg.buffer[3]=0;
	//FND output device
	snd_msg.msgtype=FPGA_SWITCH;	snd_msg.data=ALARM_SETTING;	snd_msg.mode=4;
	msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
	while(1){
		usleep(100000);	current_time=(time(NULL)%86400)/60;	alarm_time%=1440;
		if(alarm_time==current_time&&status&ALARM_WAITING){//When alarm time
			snd_msg.data=status=ALARMING;
			msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
		}
		if(msgrcv(key_id_input,&rcv_msg,msgbuf_size,READKEY,IPC_NOWAIT)>0){
			READKEY_MACRO
			break;
		}
		else if(msgrcv(key_id_input,&rcv_msg,msgbuf_size,FPGA_SWITCH,IPC_NOWAIT)>0){
			if(status&ALARM_SETTING){
				switch(rcv_msg.data){
					case SW_1:	//press switch 1 (SETTING -> WAITING)
						snd_msg.data=status=ALARM_WAITING;
						msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
						break;
					case SW_2:	alarm_time+=60;
						ALARM_MACRO
						msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
						break;
					case SW_3:	alarm_time-=60;
						if(alarm_time<0)
							alarm_time+=1440;
						ALARM_MACRO
						msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
						break;
					case SW_5:	alarm_time++;
						ALARM_MACRO
						msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
						break;
					case SW_6:	alarm_time--;
						if(alarm_time<0)
							alarm_time=1439;
						ALARM_MACRO
						msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
						break;
				}
			}
			else if(status&ALARM_WAITING){
				if(rcv_msg.data&SW_5){//press switch 5 (WAITING -> SETTING)
					snd_msg.data=status=ALARM_SETTING;
					alarm_time=720;
					ALARM_MACRO
					msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
				}
			}
			else if(status=ALARMING&&rcv_msg.data&SW_5){//press switch 5 (ALARMING -> WAITING)
				snd_msg.data=status=ALARM_SETTING;
				msgsnd(key_id_output,&snd_msg,msgbuf_output_size,IPC_NOWAIT);
			}
		}
	}
}
