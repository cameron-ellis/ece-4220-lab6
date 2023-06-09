/* Lab6_cdev_kmod.c
 * ECE4220/7220
 * Based on code from: https://github.com/blue119/kernel_user_space_interfaces_example/blob/master/cdev.c
 * Modified and commented by: Luis Alberto Rivera
 
 You can compile the module using the Makefile provided. Just add
 obj-m += Lab6_cdev_kmod.o

 This Kernel module prints its "MajorNumber" to the system log. The "MinorNumber"
 can be chosen to be 0.
 
 -----------------------------------------------------------------------------------
 Broadly speaking: The Major Number refers to a type of device/driver, and the
 Minor Number specifies a particular device of that type or sometimes the operation
 mode of that device type. On a terminal, try:
    ls -l /dev/
 You'll see a list of devices, with two numbers (among other pieces of info). For
 example, you'll see tty0, tty1, etc., which represent the terminals. All those have
 the same Major number, but they will have different Minor numbers: 0, 1, etc.
 -----------------------------------------------------------------------------------
 
 After installing the module, 
 
 1) Check the MajorNumber using dmesg
 
 2) You can then create a Character device using the MajorNumber returned:
	  sudo mknod /dev/YourDevName c MajorNumber 0
    You need to create the device every time the Pi is rebooted.
	
 3) Change writing permissions, so that everybody can write to the Character Device:
	  sudo chmod go+w /dev/YourDevName
    Reading permissions should be enabled by default. You can check using
      ls -l /dev/YourDevName
    You should see: crw-rw-rw-

 After the steps above, you will be able to use the Character Device.
 If you uninstall your module, you won't be able to access your Character Device.
 If you install it again (without having shutdown the Pi), you don't need to
 create the device again --steps 2 and 3--, unless you manually delete it.
 
 Note: In this implementation, there is no buffer associated to writing to the
 Character Device. Every new string written to it will overwrite the previous one.
*/
  
#ifndef MODULE
#define MODULE
#endif
#ifndef __KERNEL__
#define __KERNEL__
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/kthread.h>	// for kthreads
#include <linux/timer.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/unistd.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>

#define MSG_SIZE 40
#define CDEV_NAME "Lab6"	// "YourDevName"
#define GPIO_OUT6 (6)		// Speaker Pin
#define BTN_1 (16)
#define BTN_2 (17)
#define BTN_3 (18)
#define BTN_4 (19)
#define BTN_5 (20)

MODULE_LICENSE("GPL");
int DELAY = 300;
int btn1_irq;
int btn2_irq;
int btn3_irq;
int btn4_irq;
int btn5_irq;
static int major; 
static char msg[MSG_SIZE];

// structure for the kthread.
static struct task_struct *kthread1;

// Routine to handle button irqs
static irqreturn_t btn_irq_handler(int irq, void *dev_id)
{
	printk(KERN_INFO "INTERRUPT OCCURRED\n");

    // Check IRQ and turn certain tones accordingly
	if (irq == btn1_irq)
    {
        msg[0] = 'A';
		DELAY = 400;
    }
    else if (irq == btn2_irq)
    {
        msg[0] = 'B';
		DELAY = 500;
    }
    else if (irq == btn3_irq)
    {
        msg[0] = 'C';
		DELAY = 600;
    }
    else if (irq == btn4_irq)
    {
        msg[0] = 'D';
		DELAY = 700;
    }
    else if (irq == btn5_irq)
    {
        msg[0] = 'E';
		DELAY = 800;
    }
    else
    {
        printk(KERN_INFO "ERROR: UKNOWN IRQ\n");
    }
    
	return IRQ_HANDLED;
}

// Function called when the user space program reads the character device.
// Some arguments not used here.
// buffer: data will be placed there, so it can go to user space
// The global variable msg is used. Whatever is in that string will be sent to userspace.
// Notice that the variable may be changed anywhere in the module...
static ssize_t device_read(struct file *filp, char __user *read_buffer, size_t length, loff_t *offset)
{
	// Whatever is in msg will be placed into buffer, which will be copied into user space
	ssize_t dummy = copy_to_user(read_buffer, msg, length);	// dummy will be 0 if successful

	// msg should be protected (e.g. semaphore). Not implemented here, but you can add it.
	msg[0] = '\0';	// "Clear" the message, in case the device is read again.
					// This way, the same message will not be read twice.
					// Also convenient for checking if there is nothing new, in user space.
	// Check if read error
	if (dummy != 0)
	{
		printk("Device Read Fail!\n");
	}
	
	return length;
}

// Function called when the user space program writes to the Character Device.
// Some arguments not used here.
// buff: data that was written to the Character Device will be there, so it can be used
//       in Kernel space.
// In this example, the data is placed in the same global variable msg used above.
// That is not needed. You could place the data coming from user space in a different
// string, and use it as needed...
static ssize_t device_write(struct file *filp, const char __user *buff, size_t len, loff_t *off)
{
	ssize_t dummy;
	
	if(len > MSG_SIZE)
		return -EINVAL;
	
	// unsigned long copy_from_user(void *to, const void __user *from, unsigned long n);
	dummy = copy_from_user(msg, buff, len);	// Transfers the data from user space to kernel space
	if(len == MSG_SIZE)
		msg[len-1] = '\0';	// will ignore the last character received.
	else
		msg[len] = '\0';
	
	// You may want to remove the following printk in your final version.
	printk("Message from user space: %s\n", msg);

	// Check if read error
	if (dummy != 0)
	{
		printk("Device Read Fail!\n");
	}
	
	return len;		// the number of bytes that were written to the Character Device.
}

// structure needed when registering the Character Device. Members are the callback
// functions when the device is read from or written to.
static struct file_operations fops = {
	.read = device_read, 
	.write = device_write,
};

// Function to be associated with the kthread; what the kthread executes.
int kthread_fn(void *ptr)
{
	printk("In kthread_fn\n");
	// The ktrhead does not need to run forever. It can execute something
	// and then leave.
	while(1)
	{
		gpio_set_value(GPIO_OUT6, 1);
		udelay(DELAY);	// good for a few us (micro s)
		gpio_set_value(GPIO_OUT6, 0);
		udelay(DELAY);	// good for a few us (micro s)

		if(kthread_should_stop()) {
			printk("exiting kthread\n");
			do_exit(0);
		}
	}
	
	return 0;
}

int cdev_module_init(void)
{
	// Check if SPKR PIN is Valid and available to be allocated 
	// Set it to be output
	if (gpio_is_valid(GPIO_OUT6) == 1)
	{
		if(gpio_request(GPIO_OUT6, "SPEAKER") == 0) 
		{
			gpio_direction_output(GPIO_OUT6, 0);
		}
	}

	// Set Buttons to input
    for (int i = 16; i < 20; i++)
    {
    	if (gpio_direction_input(i) == 0)
    	{
    		printk(KERN_INFO "GPIO %d set to input!\n", i);
    	}
    	else
    	{
    		printk(KERN_INFO "GPIO %d input set failed!\n", i);
    	}
    }
    
    // Setting GPIO Buttons to IRQ Numbers
    btn1_irq = gpio_to_irq(BTN_1);
    btn2_irq = gpio_to_irq(BTN_2);
    btn3_irq = gpio_to_irq(BTN_3);
    btn4_irq = gpio_to_irq(BTN_4);
    btn5_irq = gpio_to_irq(BTN_5);
    
    // Btn1 IRQ Request
    if (request_irq(btn1_irq,(void *)btn_irq_handler, IRQF_TRIGGER_RISING,"BTN_1_INTERRUPT",NULL) == 0)
    {
        printk(KERN_INFO "Request Btn1 IRQ Success!\n");
    }
    else
    {
        printk(KERN_INFO "Request Btn1 IRQ Failed!\n");
    }

    // Btn2 IRQ Request
    if (request_irq(btn2_irq,(void *)btn_irq_handler, IRQF_TRIGGER_RISING,"BTN_2_INTERRUPT",NULL) == 0)
    {
        printk(KERN_INFO "Request Btn2 IRQ Success!\n");
    }
    else
    {
        printk(KERN_INFO "Request Btn2 IRQ Failed!\n");
    }

    // Btn3 IRQ Request
    if (request_irq(btn3_irq,(void *)btn_irq_handler, IRQF_TRIGGER_RISING,"BTN_3_INTERRUPT",NULL) == 0)
    {
        printk(KERN_INFO "Request Btn3 IRQ Success!\n");
    }
    else
    {
        printk(KERN_INFO "Request Btn3 IRQ Failed!\n");
    }

    // Btn4 IRQ Request
    if (request_irq(btn4_irq,(void *)btn_irq_handler, IRQF_TRIGGER_RISING,"BTN_4_INTERRUPT",NULL) == 0)
    {
        printk(KERN_INFO "Request Btn4 IRQ Success!\n");
    }
    else
    {
        printk(KERN_INFO "Request Btn4 IRQ Failed!\n");
    }

    // Btn5 IRQ Request
    if (request_irq(btn5_irq,(void *)btn_irq_handler, IRQF_TRIGGER_RISING,"BTN_5_INTERRUPT",NULL) == 0)
    {
        printk(KERN_INFO "Request Btn5 IRQ Success!\n");
    }
    else
    {
        printk(KERN_INFO "Request Btn5 IRQ Failed!\n");
    }
	
	char kthread_name[]="SoundGeneration_kthread";	// try running  ps -ef | grep SoundGeneration
													// when the thread is active.
	printk("In init module\n");
    	    
    kthread1 = kthread_create(kthread_fn, NULL, kthread_name);
	
    if((kthread1))	// true if kthread creation is successful
    {
        printk("Inside if\n");
		// kthread is dormant after creation. Needs to be woken up
        wake_up_process(kthread1);
    }
	// register the Characted Device and obtain the major (assigned by the system)
	major = register_chrdev(0, CDEV_NAME, &fops);
	if (major < 0) {
     		printk("Registering the character device failed with %d\n", major);
	     	return major;
	}
	printk("Lab6_cdev_kmod example, assigned major: %d\n", major);
	printk("Create Char Device (node) with: sudo mknod /dev/%s c %d 0\n", CDEV_NAME, major);
 	return 0;
}

void cdev_module_exit(void)
{
	// Once unregistered, the Character Device won't be able to be accessed,
	// even if the file /dev/YourDevName still exists. Give that a try...
	unregister_chrdev(major, CDEV_NAME);
	printk("Char Device /dev/%s unregistered.\n", CDEV_NAME);
	//Free the PIN
	gpio_free(GPIO_OUT6);

	// Free irqs for Buttons
    free_irq(btn1_irq,NULL);
    printk(KERN_INFO "Freed Btn1 IRQ\n");
    free_irq(btn2_irq,NULL);
    printk(KERN_INFO "Freed Btn2 IRQ\n");
    free_irq(btn3_irq,NULL);
    printk(KERN_INFO "Freed Btn3 IRQ\n");
    free_irq(btn4_irq,NULL);
    printk(KERN_INFO "Freed Btn4 IRQ\n");
    free_irq(btn5_irq,NULL);
    printk(KERN_INFO "Freed Btn5 IRQ\n");

	// Free Button GPIOs
    for (int i = 16; i < 21; i++)
    {
    	gpio_free(i);
    	printk(KERN_INFO "GPIO %d freed!\n", i);
    }

	int ret;
	// the following doesn't actually stop the thread, but signals that
	// the thread should stop itself (with do_exit above).
	// kthread should not be called if the thread has already stopped.
	ret = kthread_stop(kthread1);
								
	if(!ret)
		printk("Kthread stopped\n");
}

module_init(cdev_module_init);
module_exit(cdev_module_exit);
