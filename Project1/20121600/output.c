#include"header.h"

void output_process(void){
	struct msgbuf_output rcv_msg;
	int msgbuf_size=sizeof(struct msgbuf)-sizeof(long);
	int msgbuf_counter_size=sizeof(struct msgbuf_output)-sizeof(long);
	int key_id=msgget((key_t)MAIN_TO_OUTPUT,IPC_CREAT|0666);
	const int zero=0,one=1;
	unsigned char text_lcd[33],fnd_buffer[4];
	unsigned char fpga_number[5][10]={
		{0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e},//1
		{0x1c,0x36,0x63,0x63,0x63,0x7f,0x7f,0x63,0x63,0x63},//A
		{0x00,0x00,0x1c,0x22,0x41,0x41,0x22,0x1c,0x00,0x00},//Sun symbol
		{0x00,0x38,0x14,0x12,0x09,0x09,0x12,0x14,0x38,0x00},//moon symbol
		{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
	};
	unsigned char dot_matrix_buffer[10]={0};
	unsigned char dot_buffer;

	memset(text_lcd,'\0',sizeof(text_lcd));
	dev_led=open("/dev/fpga_led",O_WRONLY);
	dev_fnd=open("/dev/fpga_fnd",O_WRONLY);
	dev_text=open("/dev/fpga_text_lcd",O_WRONLY);	
	dev_dot=open("/dev/fpga_dot",O_WRONLY);
	dev_buzzer=open("/dev/fpga_buzzer",O_RDWR);			//device open

	while(1){
		usleep(100000);
		if(msgrcv(key_id,&rcv_msg,msgbuf_counter_size,FPGA_SWITCH,IPC_NOWAIT)>0){//When SW input
			switch(rcv_msg.mode){
				case 0:			//when clock mode0
					write(dev_text,"                                    ",32);
					write(dev_led,&rcv_msg.data,1);
					write(dev_fnd,&rcv_msg.buffer,4);
					memset(dot_matrix_buffer,0,sizeof(dot_matrix_buffer));
					write(dev_dot,dot_matrix_buffer,sizeof(dot_matrix_buffer));
					break;
				case 1:			//when counter mode1
					memset(text_lcd,'\0',32);
					write(dev_led,&rcv_msg.data,1);
					write(dev_text,text_lcd,32);
					write(dev_fnd,&rcv_msg.buffer,4);
					break;
				case 2:			//when text editor mode2
					write(dev_led,&zero,1);
					if(rcv_msg.buffer[3])				//Draw 1 in dot
						write(dev_dot,fpga_number[0],sizeof(fpga_number[0]));
					else								//Draw A in dot
						write(dev_dot,fpga_number[1],sizeof(fpga_number[1]));
					fnd_buffer[3]=rcv_msg.data%10;
					fnd_buffer[2]=rcv_msg.data/10%10;
					fnd_buffer[1]=rcv_msg.data/100%10;
					fnd_buffer[0]=rcv_msg.data/1000;
					write(dev_fnd,&fnd_buffer,4);		//write count on fnd device

					if(rcv_msg.buffer[2])				//when clear command input
						memset(text_lcd,' ',32);
					else{
						if(rcv_msg.buffer[0]>31){
							strcpy(text_lcd,text_lcd+1);
							rcv_msg.buffer[0]=31;
						}
						text_lcd[rcv_msg.buffer[0]]=rcv_msg.buffer[1];
					}
					write(dev_text,text_lcd,32);
					break;
				case 3:			//when draw board mode3
					write(dev_led,0,1);
					fnd_buffer[3]=rcv_msg.data%10;
					fnd_buffer[2]=rcv_msg.data/10%10;
					fnd_buffer[1]=rcv_msg.data/100%10;
					fnd_buffer[0]=rcv_msg.data/1000;
					write(dev_fnd,&fnd_buffer,4);

					if(rcv_msg.buffer[3]&2){				//blinking every seconds signal
						if(rcv_msg.buffer[3]&4){
							dot_buffer=dot_matrix_buffer[rcv_msg.buffer[1]];
							dot_matrix_buffer[rcv_msg.buffer[1]]^=rcv_msg.buffer[0];
							write(dev_dot,dot_matrix_buffer,sizeof(dot_matrix_buffer));
							dot_matrix_buffer[rcv_msg.buffer[1]]=dot_buffer;
						}
						else
							write(dev_dot,dot_matrix_buffer,sizeof(dot_matrix_buffer));
					}
					else if(rcv_msg.buffer[2]&1){							//clear command input
						memset(dot_matrix_buffer,0,sizeof(dot_matrix_buffer));
						write(dev_dot,dot_matrix_buffer,sizeof(dot_matrix_buffer));
					}
					else if(rcv_msg.buffer[2]&2){							//convert command input
						dot_matrix_buffer[0]=~dot_matrix_buffer[0];
						dot_matrix_buffer[1]=~dot_matrix_buffer[1];
						dot_matrix_buffer[2]=~dot_matrix_buffer[2];
						dot_matrix_buffer[3]=~dot_matrix_buffer[3];
						dot_matrix_buffer[4]=~dot_matrix_buffer[4];
						dot_matrix_buffer[5]=~dot_matrix_buffer[5];
						dot_matrix_buffer[6]=~dot_matrix_buffer[6];
						dot_matrix_buffer[7]=~dot_matrix_buffer[7];
						dot_matrix_buffer[8]=~dot_matrix_buffer[8];
						dot_matrix_buffer[9]=~dot_matrix_buffer[9];
						write(dev_dot,dot_matrix_buffer,sizeof(dot_matrix_buffer));
					}
					else if(rcv_msg.buffer[3]&1){							//select command input
						dot_matrix_buffer[rcv_msg.buffer[1]]^=rcv_msg.buffer[0];
						write(dev_dot,dot_matrix_buffer,sizeof(dot_matrix_buffer));
					}
					break;
				case 4:
					write(dev_led,&zero,1);
					if(rcv_msg.data&ALARM_WAITING){							//When waiting mode
						strncpy(text_lcd,"Alarm is  ",sizeof(text_lcd));
						text_lcd[10]=rcv_msg.buffer[0]+'0';					//printing alarm time
						text_lcd[11]=rcv_msg.buffer[1]+'0';
						text_lcd[12]=':';
						text_lcd[13]=rcv_msg.buffer[2]+'0';
						text_lcd[14]=rcv_msg.buffer[3]+'0';
						write(dev_text,text_lcd,32);
						write(dev_dot,fpga_number[3],sizeof(dot_matrix_buffer));
					}
					else if(rcv_msg.data&ALARM_SETTING){					//When alarm setting mode
						write(dev_buzzer,&zero,1);
						write(dev_fnd,rcv_msg.buffer,4);
						write(dev_text,"Alarm Setting...                 ",32);
						write(dev_dot,fpga_number[4],sizeof(dot_matrix_buffer));
					}
					else{//ALARMING
						write(dev_buzzer,&one,1);							//turn on buzzer
						write(dev_text,"Press SW5 to stop alarm!!!       ",32);
						write(dev_dot,fpga_number[2],sizeof(dot_matrix_buffer));
					}
					break;
			}
		}
		else if(msgrcv(key_id,&rcv_msg,msgbuf_size,READKEY,IPC_NOWAIT)>0){//When Gpio input
			memset(dot_matrix_buffer,0,sizeof(dot_matrix_buffer));
			write(dev_dot,dot_matrix_buffer,sizeof(dot_matrix_buffer));
			write(dev_led,0,1);
			memset(fnd_buffer,0,4);
			write(dev_fnd,fnd_buffer,4);
			memset(text_lcd,' ',32);
			write(dev_text,text_lcd,32);
			if(!rcv_msg.data){
				close(dev_fnd);	close(dev_led);	close(dev_text);	close(dev_dot);	close(dev_buzzer);
				break;
			}
		}
	}	
}
