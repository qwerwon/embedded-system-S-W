#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <mach/gpio.h>
#include <linux/platform_device.h>
#include <asm/gpio.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/timer.h>
#include <linux/errno.h>

#define FND_ADDRESS 0x08000004
#define MAJOR_NUBMER 242
#define DEVICE_NAME "/dev/stopwatch"

static int device_usage=0;
//static dev_t inter_dev;
//static struct cdev inter_cdev;
static struct timer_list my_timer,quit_timer;
static int current_min=0,current_sec=0;
static unsigned char*fnd_addr;
static unsigned long saved_jiffies=HZ;



static int inter_open(struct inode *, struct file *);
static int inter_release(struct inode *, struct file *);
static int inter_write(struct file *filp, const unsigned long *buf, size_t count, loff_t *f_pos);

irqreturn_t inter_handler1(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler2(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler3(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler4(int irq, void* dev_id, struct pt_regs* reg);

void quit_handler(unsigned long arg);
void timer_handler(unsigned long arg);

wait_queue_head_t wq_write;
DECLARE_WAIT_QUEUE_HEAD(wq_write);

static struct file_operations inter_fops =
{
	.open = inter_open,
	.write = inter_write,
	.release = inter_release,
};

irqreturn_t inter_handler1(int irq, void* dev_id, struct pt_regs* reg) {	//home button(start)
	printk(KERN_ALERT "interrupt1!!! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 11)));
	printk("wake up\n");
	my_timer.function=timer_handler;
	my_timer.expires=get_jiffies_64()+saved_jiffies;
	add_timer(&my_timer);
	return IRQ_HANDLED;
}

irqreturn_t inter_handler2(int irq, void* dev_id, struct pt_regs* reg) {	//back(pause)
        printk(KERN_ALERT "interrupt2!!! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 12)));
	saved_jiffies=my_timer.expires - get_jiffies_64();
	del_timer(&my_timer);
        return IRQ_HANDLED;
}

irqreturn_t inter_handler3(int irq, void* dev_id,struct pt_regs* reg) {		//volup(reset)
	current_sec=current_min=0;
        printk(KERN_ALERT "interrupt3!!! = %x\n", gpio_get_value(IMX_GPIO_NR(2, 15)));
	my_timer.expires=get_jiffies_64() + HZ;
	outw((unsigned short int)0,(unsigned int)fnd_addr);

        return IRQ_HANDLED;
}


irqreturn_t inter_handler4(int irq, void* dev_id, struct pt_regs* reg) {	//voldown(quit)
	int value=gpio_get_value(IMX_GPIO_NR(5, 14));
        printk(KERN_ALERT "interrupt4!!! = %x\n", value);
	if(value){
		del_timer(&quit_timer);
	}
	else{
		quit_timer.function=quit_handler;
		quit_timer.expires=get_jiffies_64() + HZ*3;
		add_timer(&quit_timer);
	}	
        return IRQ_HANDLED;
}

void timer_handler(unsigned long arg){
	unsigned short int value_short;
	current_sec++;
	if(current_sec >= 60)	current_min++;
	current_sec%=60;
	current_min%=60;

	value_short=(current_min/10<<12)+(current_min%10<<8)+(current_sec/10<<4)+current_sec%10;
	outw(value_short,(unsigned int)fnd_addr);

	my_timer.function=timer_handler;
	my_timer.expires=get_jiffies_64() + HZ;
	add_timer(&my_timer);
}


void quit_handler(unsigned long arg){
	outw(0,(unsigned int)fnd_addr);
 	__wake_up(&wq_write, 1, 1, NULL);
	del_timer(&my_timer);
	del_timer(&quit_timer);
}

static int inter_open(struct inode *minode, struct file *mfile){
	int ret;
	int irq;

	if(device_usage)	return -EBUSY;
	device_usage=1;

	printk(KERN_ALERT "Open Module\n");

	// int1
	gpio_direction_input(IMX_GPIO_NR(1,11));
	irq = gpio_to_irq(IMX_GPIO_NR(1,11));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler1, IRQF_TRIGGER_FALLING, "home", 0);

	// int2
	gpio_direction_input(IMX_GPIO_NR(1,12));
	irq = gpio_to_irq(IMX_GPIO_NR(1,12));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler2, IRQF_TRIGGER_FALLING, "back", 0);

	// int3
	gpio_direction_input(IMX_GPIO_NR(2,15));
	irq = gpio_to_irq(IMX_GPIO_NR(2,15));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler3, IRQF_TRIGGER_FALLING, "volup", 0);

	// int4
	gpio_direction_input(IMX_GPIO_NR(5,14));
	irq = gpio_to_irq(IMX_GPIO_NR(5,14));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler4, IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, "voldown", 0);

	return 0;
}

static int inter_release(struct inode *minode, struct file *mfile){
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);

	device_usage=0;	current_min=current_sec=0;
	
	printk(KERN_ALERT "Release Module\n");
	return 0;
}

static int inter_write(struct file *filp, const unsigned long *buf, size_t count, loff_t *f_pos ){
	init_timer(&my_timer);
	init_timer(&quit_timer);

	//my_timer.function=timer_handler;
	//my_timer.expires=get_jiffies_64() + HZ;
	//add_timer(&my_timer);
        interruptible_sleep_on(&wq_write);
	printk("write\n");
	return 0;
}


static int __init inter_init(void) {
	int result;
	result = register_chrdev(MAJOR_NUBMER,DEVICE_NAME,&inter_fops);
	if(result < 0 ){
		printk(KERN_WARNING"Can't get any major\n");
		return result;
	}
	printk(KERN_ALERT "Init Module Success \n");
	printk(KERN_ALERT "Device : /dev/inter, Major Num : 242 \n");

	fnd_addr=ioremap(FND_ADDRESS,0x4);
	return 0;
}

static void __exit inter_exit(void) {
	iounmap(fnd_addr);
	//cdev_del(&inter_cdev);
	//unregister_chrdev_region(inter_dev, 1);
	unregister_chrdev(MAJOR_NUBMER,DEVICE_NAME);
	printk(KERN_ALERT "Remove Module Success \n");
}

module_init(inter_init);
module_exit(inter_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HOU");
