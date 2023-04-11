/* Based on example from: http://tuxthink.blogspot.com/2011/02/kernel-thread-creation-1.html
   Modified and commented by: Luis Rivera and Ramy Farag			
   
   Compile using the Makefile
*/


   
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>	// for kthreads
#include <linux/sched.h>	// for task_struct
#include <linux/time.h>		// for using jiffies 
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/gpio.h>


#define GPIO_OUT6 (6)
#define GPIO_OUT2 (2)

MODULE_LICENSE("GPL");

// structure for the kthread.
static struct task_struct *kthread1;


// Function to be associated with the kthread; what the kthread executes.
int kthread_fn(void *ptr)
{
	printk("In kthread_fn\n");

	// The ktrhead does not need to run forever. It can execute something
	// and then leave.
	while(1)
	{

		// Write code to generate Frequency on BCM 6 (SPKR) (GPIO_OUT6)
		gpio_set_value(GPIO_OUT6,1);
		//gpio_set_value(GPIO_OUT2,1);
		msleep(10);	// good for > 10 ms
		gpio_set_value(GPIO_OUT6,0);
		//gpio_set_value(GPIO_OUT2,0);
		msleep(10);   // good for > 10 ms
		//msleep_interruptible(1000); // good for > 10 ms
		//udelay(unsigned long usecs);	// good for a few us (micro s)
		//usleep_range(unsigned long min, unsigned long max); // good for 10us - 20 ms
		
		// In an infinite loop, you should check if the kthread_stop
		// function has been called (e.g. in clean up module). If so,
		// the kthread should exit. If this is not done, the thread
		// will persist even after removing the module.
		if(kthread_should_stop()) {
			do_exit(0);
		}
				
		// comment out if your loop is going "fast". You don't want to
		// printk too often. Sporadically or every second or so, it's okay.
		//printk("Count: %d\n", ++count);
	}
	
	return 0;
}

int thread_init(void)
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
	if (gpio_is_valid(GPIO_OUT2) == 1)
	{
		if(gpio_request(GPIO_OUT2, "RED LED") == 0) 
		{
			gpio_direction_output(GPIO_OUT2, 0);
		}
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

    return 0;
}

void thread_cleanup(void) {

	//Free the PIN
	gpio_free(GPIO_OUT6);
	gpio_free(GPIO_OUT2);

	int ret;
	// the following doesn't actually stop the thread, but signals that
	// the thread should stop itself (with do_exit above).
	// kthread should not be called if the thread has already stopped.
	ret = kthread_stop(kthread1);
								
	if(!ret)
		printk("Kthread stopped\n");
}

module_init(thread_init);
module_exit(thread_cleanup);
