#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/memory.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/version.h>

#include <linux/timer.h>
#include <linux/errno.h>

#define IOCTL_SUCCESS 1

#define MAJOR_NUMBER 242		// ioboard device major number
#define DEVICE_NAME "/dev/dev_driver"

#define LED_ADDRESS 0x08000016 
#define FND_ADDRESS 0x08000004  
#define DOT_MATRIX_ADDRESS 0x08000210
#define TEXT_LCD_ADDRESS 0x08000090

#define EXIT_DEVICE 25

static unsigned char fpga_number[9][10] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // blank
	{0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e}, // 1
	{0x7e,0x7f,0x03,0x03,0x3f,0x7e,0x60,0x60,0x7f,0x7f}, // 2
	{0xfe,0x7f,0x03,0x03,0x7f,0x7f,0x03,0x03,0x7f,0x7e}, // 3
	{0x66,0x66,0x66,0x66,0x66,0x66,0x7f,0x7f,0x06,0x06}, // 4
	{0x7f,0x7f,0x60,0x60,0x7e,0x7f,0x03,0x03,0x7f,0x7e}, // 5
	{0x60,0x60,0x60,0x60,0x7e,0x7f,0x63,0x63,0x7f,0x3e}, // 6
	{0x7f,0x7f,0x63,0x63,0x03,0x03,0x03,0x03,0x03,0x03}, // 7
	{0x3e,0x7f,0x63,0x63,0x7f,0x7f,0x63,0x63,0x7f,0x3e}, // 8
};


//Global variable
static int device_usage = 0;
static unsigned char *led_addr,*fnd_addr,*text_lcd_addr,*dot_matrix_addr;
static struct timer_list my_timer;

static int initial_num,repeat_count,repeat_interval,start_position;
static int current_num,current_position,text_lcd_my_name_position=0,text_lcd_my_id_position=0;
static int my_name_factor=1,my_id_factor=1;

// define functions...
int dev_open(struct inode *minode, struct file *mfile);
int dev_release(struct inode *minode, struct file *mfile);
long dev_ioctl(struct file*flip,unsigned int cmd,unsigned long arg);
void timer_handler(unsigned long arg);

static void led_write(int num);
static void fnd_write(int position,int num);
static void dot_matrix_write(int num);
static void text_lcd_write(int my_name_start,int my_id_start);

// define file_operations structure 
struct file_operations dev_fops =
{
	.owner		=	THIS_MODULE,
	.open		=	dev_open,
	.release	=	dev_release,
	.unlocked_ioctl	=	dev_ioctl,
};

// when device open ,call this function
int dev_open(struct inode *minode, struct file *mfile) 
{	
	if(device_usage) return -EBUSY;
	device_usage = 1;
	return 0;
}

// when device close ,call this function
int dev_release(struct inode *minode, struct file *mfile) 
{
	device_usage = 0;
	return 0;
}


/****************************************************************************************/
/*	arg:										*/
/*	first byte indicates which digits got value(ex.1000 or 0100 or 0010 or 0001	*/
/*	second byte indicates the number range 1~8					*/
/*	third byte indicates how many iterations implemneted and it decreases every loop*/
/*	The last byte indicates the interval [1~100] [0.1sec ~ 10sec]			*/
/*											*/
/*	dev_ioctl calls every I/O device write function call and set timer exprire value*/
/****************************************************************************************/

long dev_ioctl(struct file*flip,unsigned int cmd,unsigned long arg)
{
	current_position=start_position=arg&0xf;
	current_num=initial_num=(arg&0xf00)>>8;
	repeat_count=(arg&0xff0000)>>16;
	repeat_interval=(arg&0xff000000)>>24;

	init_timer(&my_timer);
	my_timer.function=timer_handler;
	my_timer.expires=get_jiffies_64() + HZ*repeat_interval/10;
       	add_timer(&my_timer);

	led_write(initial_num);
	fnd_write(start_position,initial_num);
	dot_matrix_write(initial_num);
	text_lcd_write(0,0);

	return IOCTL_SUCCESS;
}

/****************************************************************************************/
/*	Timer interrupts call this function every expire time[0.1 ~ 10sec]		*/
/*	and updates expire value and attatch expire function in every interval time	*/
/*	Updates expire value until the repeat_count decreases under 1			*/
/****************************************************************************************/

void timer_handler(unsigned long arg){

	if(!--repeat_count){
		led_write(0);
		fnd_write(0,0);
		dot_matrix_write(0);
		text_lcd_write(EXIT_DEVICE,EXIT_DEVICE);
		my_name_factor=my_id_factor=1;
		text_lcd_my_name_position=text_lcd_my_id_position=0;
		return ;
	}

	current_num=current_num%8+1;			// 1~8

	text_lcd_my_name_position+=my_name_factor;	
	text_lcd_my_id_position+=my_id_factor;
							//name
	if(text_lcd_my_name_position>6){		//When reach end of buffer,
		text_lcd_my_name_position=5;		//Moving left side
		my_name_factor=-1;
	}
	else if(text_lcd_my_name_position<0){		
		text_lcd_my_name_position=1;		//Moving right side
		my_name_factor=1;
	}
							//id
	if(text_lcd_my_id_position>8){			//""
		text_lcd_my_id_position=7;
		my_id_factor=-1;
	}
	else if(text_lcd_my_id_position<0){		//""
		text_lcd_my_id_position=1;
		my_id_factor=1;
	}

	if(current_num==initial_num){				//Shift digits when current number and initial number are same
	       	current_num=initial_num;
		current_position=current_position>>1;
		if(!current_position)	current_position=8;
	}
	led_write(current_num);
	fnd_write(current_position,current_num);
	dot_matrix_write(current_num);
	text_lcd_write(text_lcd_my_name_position,text_lcd_my_id_position);

	my_timer.function=timer_handler;
	my_timer.expires=get_jiffies_64() + HZ*repeat_interval/10;
       	add_timer(&my_timer);
}

static void led_write(int num){
	unsigned short int _s_value=256>>num;
	outw(_s_value,(unsigned int)led_addr);
}

static void fnd_write(int position,int num){
	unsigned short int value_short;
	switch(position){
		case 8:
			value_short=num<<12;
			break;
		case 4:
			value_short=num<<8;
			break;
		case 2:
			value_short=num<<4;
			break;
		case 1:
			value_short=num;
			break;
		default:
			value_short=0;
	}
	outw(value_short,(unsigned int)fnd_addr);
}

static void dot_matrix_write(int num){
	unsigned short int _s_value;
	int i=0;
	for(i=0;i<10;i++){
		_s_value=fpga_number[num][i]&0x7F;
		outw(_s_value,(unsigned int)dot_matrix_addr+i*2);
	}
}
static void text_lcd_write(int my_name_start,int my_id_start){
	unsigned short int _s_value=' ';
	unsigned char buffer[33]={"                                "};
	int i;
	buffer[33]=0;
	for(i=0;i<32;i++)
		outw(_s_value,(unsigned int)text_lcd_addr+i);

	if(my_name_start==EXIT_DEVICE)	return;

	buffer[my_id_start]='2';
	buffer[my_id_start+1]='0';
	buffer[my_id_start+2]='1';
	buffer[my_id_start+3]='2';
	buffer[my_id_start+4]='1';
	buffer[my_id_start+5]='6';
	buffer[my_id_start+6]='0';
	buffer[my_id_start+7]='0';

	buffer[my_name_start+16]='J';
	buffer[my_name_start+17]='e';
	buffer[my_name_start+18]='-';
	buffer[my_name_start+19]='H';
	buffer[my_name_start+20]='o';
	buffer[my_name_start+21]=' ';
	buffer[my_name_start+22]='S';
	buffer[my_name_start+23]='o';
	buffer[my_name_start+24]='n';
	buffer[my_name_start+25]='g';

	for(i=0;i<32;i+=2){
		_s_value=(buffer[i]&0xFF)<<8 | buffer[i+1] & 0xFF;
		outw(_s_value,(unsigned int)text_lcd_addr+i);
	}
}
int __init dev_init(void)
{
	int result;
	result = register_chrdev(MAJOR_NUMBER, DEVICE_NAME, &dev_fops);
	if(result < 0) {
		printk(KERN_WARNING"Can't get any major\n");
		return result;
	}
	led_addr = ioremap(LED_ADDRESS, 0x1);
	fnd_addr = ioremap(FND_ADDRESS, 0x4);
	text_lcd_addr = ioremap(TEXT_LCD_ADDRESS,0x10);
	dot_matrix_addr = ioremap(DOT_MATRIX_ADDRESS,0x32);

	printk("init module, %s major number : %d\n", DEVICE_NAME, MAJOR_NUMBER);
	return 0;
}

void __exit dev_exit(void) 
{
	iounmap(led_addr);	iounmap(fnd_addr);
	iounmap(text_lcd_addr);	iounmap(dot_matrix_addr);
	unregister_chrdev(MAJOR_NUMBER,DEVICE_NAME);
}

module_init(dev_init);
module_exit(dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huins");
