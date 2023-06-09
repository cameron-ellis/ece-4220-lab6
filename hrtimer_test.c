/* Based on code found at https://gist.github.com/maggocnx/5946907
   Modified and commented by: Luis Rivera

   Compile using the Makefile
*/

#ifndef MODULE
#define MODULE
#endif
#ifndef __KERNEL__
#define __KERNEL__
#endif



#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/err.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <asm/io.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/unistd.h>

MODULE_LICENSE("GPL");

unsigned long timer_interval_ns = 5e5;	// timer interval length (nano sec part)
static struct hrtimer hr_timer;			// timer structure
static int count = 0, dummy = 0;

unsigned long *ptr,*GPSET0, *GPFSEL0, *GPCLR0;




#define GPIO_OUT6 (6)

int flag=0;

// Timer callback function: this executes when the timer expires
enum hrtimer_restart timer_callback(struct hrtimer *timer_for_restart)
{
  	ktime_t currtime, interval;	// time type, in nanoseconds
	unsigned long overruns = 0;

	// Re-configure the timer parameters (if needed/desired)
  	currtime  = ktime_get();
  	interval = ktime_set(0, timer_interval_ns); // (long sec, long nano_sec)

	// Write a code to alternate the SPKR PIN from HIGH to LOW or vice versa (based on the previous state to generate the square signal)










	// Advance the expiration time to the next interval. This returns how many
	// intervals have passed. More than 1 may happen if the system load is too high.
  	overruns = hrtimer_forward(timer_for_restart, currtime, interval);


	// The following printk only executes once every 1000 cycles.
	if(dummy == 0){
		printk("Count: %d, overruns: %ld\n", ++count, overruns);
	}
	dummy = (dummy + 1)%1000;


	return HRTIMER_RESTART;	// Return this value to restart the timer.
							// If you don't want/need a recurring timer, return
							// HRTIMER_NORESTART (and don't forward the timer).
}

int timer_init(void)
{
	// Configure the SPKR PIN to be output 









   //Timer initialization 

	ktime_t ktime = ktime_set(0, timer_interval_ns); // (long sec, long nano_sec)

	// CLOCK_MONOTONIC: always move forward in time, even if system time changes
	// HRTIMER_MODE_REL: time relative to current time.
	hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

	// Attach callback function to the timer
	hr_timer.function = &timer_callback;

	// Start the timer
 	hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);

	return 0;
}

void timer_exit(void)
{
	int ret;
  	ret = hrtimer_cancel(&hr_timer);	// cancels the timer.
  	if(ret)
		printk("The timer was still in use...\n");
	else
		printk("The timer was already cancelled...\n");	// if not restarted or
														// cancelled before
	
	//FREE PIN 6
	gpio_free(GPIO_OUT6);


  	printk("HR Timer module uninstalling\n");

}

// Notice this alternative way to define your init_module()
// and cleanup_module(). "timer_init" will execute when you install your
// module. "timer_exit" will execute when you remove your module.
// You can give different names to those functions.
module_init(timer_init);
module_exit(timer_exit);

