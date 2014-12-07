#include<linux/interrupt.h>
#include<mach/regs-gpio.h>
#include<linux/poll.h>
#include<linux/device.h>
#include<asm/io.h>
#include<linux/irq.h>

#define DEVICE_NAME  "key"
#define DEVICE_MAJOR  232
#define KEY_TIMER_DELAY1  (HZ/50)
#define KEY_TIMER_DELAY2  (HZ/10)
#define KEY_DOWN  0
#define KEY_UP  1
#define KEY_UNCERTAIN 2
#define KEY_COUNT  4

static volatile int ev_press = 0;
static volatile int key_status[KEY_COUNT];
static struct timer_list key_timers[KEY_LINE];
static DECLARE_WAIT_QUEUE_HEAD(button_waitq);

struct key_irq_desc{
	int irq;
	int pin;
	int pin_setting;
	char *name;
};

static struct key_irq_desc key_irqs[] = 
{
	{IRQ_EINT1,S3C2410_GPF1,S3C2410_GPF1_EINT1,"KEY0"},
	{IRQ_EINT4,S3C2410_GPF4,S3C2410_GPF4_EINT4,"KEY1"},
	{IRQ_EINT2,S3C2410_GPF2,S3C2410_GPF2_EINT2,"KEY2"},
	{IRQ_EINT0,S3C2410_GPF0,S3C2410_GPF0_EINT0,"KEY4"},
};

static struct file_operations key_fops =
{
	.owner  = THIS_MODULE,
	.open  = key_open,
	.release = key_close,
	.read = key_read,
	.poll = key_poll,
};

static int key_open(struct inode *inode,struct file *file)
{
	int i;
	int ret;

	for(i=0;i<KEY_COUNT;i++)
	{
		s3c2410_gpio_cfgpin(key_irq[i].pin,key_irq[i].pin_setting);
		set_irq_type(key_irq[i].irq,IRQ_TYPE_EDGE_FAILING);
		ret = request_irq(key_irqs[i].irq,key_interrupt,IRQF_DISABLED,key_irqs[i].name,(void *)i);

		if(ret)
		{
			break;
		}

		key_status[i] = KEY_UP;

		key_timers[i].function = key_timer;
		key_timers[i].data = i;
		init_timer(&key_timers[i]);
	}

	if(ret)
	{
		i--;
		for(;i>=0;i--)
		{
			disable_irq(key_irqs[i].irq);
			free_irq(key_irqs[i],(void *)i);
		}
		return -EBUSY;
	}

	return 0;
}

static int key_close(struct inode *inode,struct file *file)
{
	int i;

	for(i=0;i<KEY_COUNT;i++)
	{
		del_timer(&key_timers[i]);
		disable_irq(key_irqs[i].irq);
		free_irq(key_irqs[i].irq,(void *)i);
	}
	return 0;
}

static int key_read(struct file *filp,char __user *buf,size_t count,loff_t *ops)
{
	unsigned long ret;
	int i;

	if(!ev_press)
	{
		if(filp->f_flags&O_NONBLOCK)
		{
			return -EAGAIN;
		}
		else
		{
			wait_event_intruptible(key_waiq,ev_press);
		}
	}

ev_press = 0;

ret = copy_to_user(buf,(void *)key_status,min(sizeof(key_status),count));

s3c2410_gpio_setpin(S3C2410_GPB5,0);
s3c2410_gpio_setpin(S3C2410_GPB6,0);
s3c2410_gpio_setpin(S3C2410_GPB7,0);
s3c2410_gpio_setpin(S3C2410_GPB8,0);

return ret? -EFAULT:min(sizeof(key_status),count);
}

static unsigned int key_poll(struct file *file,struct poll_table_struct *wait)
{
	unsigned int mask = 0;
	poll_wait(file,&key_waitq,wait);

	if(ev_press)
	{
		mask|=POLLIN|POLLRDNORM;
	}
	return mask; 
}

static irqreturn_t key_interrupt(int irq,void *dev_id,struct pt_regs *regs)
{
	int line = (int)dev_id;
	if(key_status[line] == KEY_UP)
	{
		key_status[line]  = KEY_UNCERTAIN;
		key_timers[line].expires = jiffies + KEY_TIMER_DELAY1;
		add_timer(&key_timers[line]);
	}

	return IRQ_RETVAL(IRQ_HANDLED);
}

static void key_timer(unsigned long arg)
{
	int line = arg;

	int up = s3c2410_gpio_getpin(key_irqs[line].pin);
	if(!up)
	{
		if(key_status[line] == KEY_UNCERTAIN)
		{
			key_status[line] = KEY_DOWN;
			
			switch(line)
			{
				case 0:
					s3c2410_gpio_setpin(S3C2410_GPB5,1);
					break;
				case 1:
					s3c2410_gpio_setpin(S3C2410_GPB6,1);
					break;
				case 2:
					s3c2410_gpio_setpin(S3C2410_GPB7,1);
					break;
				case 3:
					s3c2410_gpio_setpin(S3C2410_GPB8,1);
					break;
			}
			ev_press = 1;

			wake_up_interruptible(&key_waitq);
		}

		key_timers[line].expires = jiffies + KEY_TIMER_DELAY2;

		add_timer(&key_timers[line]);
	}
	else
	{
		key_status[line] = KEY_UP;
	}
}

static int __init buttons_init(void)
{
	int ret;

	//设置扫描输出口
	s3c2410_gpio_cfgpin(S3C2410_GPB5,S3C2410_GPB5_OUTP);
	s3c2410_gpio_cfgpin(S3C2410_GPB6,S3C2410_GPB6_OUTP);
	s3c2410_gpio_cfgpin(S3C2410_GPB7,S3C2410_GPB7_OUTP);
	s3c2410_gpio_cfgpin(S3C2410_GPB8,S3C2410_GPB8_OUTP);
	//输出口的初始值设为0
	s3c2410_gpio_setpin(S3C2410_GPB5,0);
	s3c2410_gpio_setpin(S3C2410_GPB6,0);
	s3c2410_gpio_setpin(S3C2410_GPB7,0);
	s3c2410_gpio_setpin(S3C2410_GPB8,0);
	//注册字符设备
	ret = register_chrdev(DEVICE_MAJOR,DEVICE_NAME,&key_fops);

	if(ret < 0)
	{
		printk(DEVICE_NAME"register failed!\n");
		return ret;
	}
	else
	{
		printk(DEVICE_NAME"register successfully!\n");

	}
	return 0;
}

static void __exit buttons_exit(void)
{
	unregister_chrdev(DEVICE_MAJOR,DEVICE_NAME);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("vnail");
MODULE_DESCRIPTION("S3C2440 button driver");




























