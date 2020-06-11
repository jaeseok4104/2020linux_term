#include "/root/work/achroimx6q/fpga_driver/include/fpga_driver.h"

static int push_switch_port_usage = 0;
static unsigned char *iom_fpga_push_switch_addr;

//proto
//static ssize_t iom_fnd_write(struct file *file, const char *buf,size_t count,loff_t *f_pos);
static ssize_t iom_push_switch_read(struct file *file,char *buf,size_t count,loff_t *f_pos);
static int iom_push_switch_open(struct inode *inode,struct file *file);
static int iom_push_switch_release(struct inode *inode,struct file *file);

struct file_operations iom_push_switch_fops =
{
	.owner	=	THIS_MODULE,
	.open	=	iom_push_switch_open,
	//.write	=	iom_fnd_write,
	.read	=	iom_push_switch_read,
	.release=	iom_push_switch_release
};

static int iom_push_switch_open(struct inode *inode,struct file *file)
{
	if(push_switch_port_usage)
	return -EBUSY;

	push_switch_port_usage = 1;
	return 0;
}

static int iom_push_switch_release(struct inode *inode,struct file *file)
{
	push_switch_port_usage = 0;
	return 0;
}

/*
static ssize_t iom_fnd_write(struct file *file,const char *buf,size_t count,loff_t *f_pos)
{
	unsigned char value[4];
	unsigned short _s_value;

	if (count > 4)
	count = 4;
	if (copy_from_user(value, buf, 4))
	return -EFAULT;

	_s_value = value[0] << 12 | value[1] << 8 | value[2] << 4 | value[3];
	outw(_s_value, (unsigned int)iom_fpga_fnd_addr);
	return count;
}
*/

static ssize_t iom_push_switch_read(struct file *file,char *buf,size_t count,loff_t *f_pos)
{
	unsigned char value[IOM_PUSH_SWITCH_MAX_BUTTON];
	unsigned short _s_value;
	int i;

	if(count > IOM_PUSH_SWITCH_MAX_BUTTON)
	count = IOM_PUSH_SWITCH_MAX_BUTTON;

	for(i = 0; i<count; i++)
	{
		_s_value = inw((unsigned int)iom_fpga_push_switch_addr +i * 2);
		value[i] = _s_value && 0xFF;
	}
	if(copy_to_user(buf,value,count))
	return -EFAULT;
	return count;
}

// __init : <linux/init.h>
int __init iom_push_switch_init(void)
{
	int result = register_chrdev(IOM_PUSH_SWITCH_MAJOR,IOM_PUSH_SWITCH_NAME,&iom_push_switch_fops);

	if(result < 0 )
	{
		printk(KERN_WARNING "Can't get any major number\n");
		return result;
	}

	iom_fpga_push_switch_addr = ioremap(IOM_PUSH_SWITCH_ADDRESS, 0x12);
	printk("init module %s,major number:%d\n",IOM_PUSH_SWITCH_NAME,IOM_PUSH_SWITCH_MAJOR);
	return 0;
}

void __exit iom_push_switch_exit(void)
{
	iounmap(iom_fpga_push_switch_addr);
	unregister_chrdev(IOM_PUSH_SWITCH_MAJOR,IOM_PUSH_SWITCH_NAME);
}

module_init(iom_push_switch_init);	
module_exit(iom_push_switch_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("InYeong");
MODULE_DESCRIPTION("FPGA FND driver");
