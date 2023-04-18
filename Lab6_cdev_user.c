/* Lab6_cdev_user.c
 * ECE4220/7220
 * Author: Luis Alberto Rivera
 
 This program allows you to enter a message on the terminal, and then it writes the
 message to a Character Device. The Device should be created beforehand, as described
 in the Lab6_cdev_kmod.c file.
 Try this example together with the Lab6_cdev_kmod module. Once you enter a message, run
	dmesg | tail     ( | tail  is not needed. With that, you'll only see the last few entries).
 on a different terminal. You should see the message in the system log (printk'd in the
 module). That would mean that the message is getting to kernel space.
 Use elements from this example and the Lab6_cdev_user module in your Lab 6 programs. You may
 need to modify a bit the callback functions in the module, according to your needs.
 
 For the extra credit part of lab 6, you'll need to think about how to read messages coming
 from kernel space, and how to create those messages in the module...
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <mosquitto.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netdb.h>

#define CHAR_DEV "/dev/Lab6" // "/dev/YourDevName"
#define MSG_SIZE 40
#define IP_MAX_SIZE 20      // max size of IP address
#define MOSQ_ID "Cameron"

// get_wlan0_ip_addr
/*  Function for getting wlan0 IP address of raspberry pi 
    and returning as a char pointer 
*/
char * get_wlan0_ip_addr() {
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    
    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;
    
    /* I want IP address attached to "wlan0" */
    strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ-1);
    
    ioctl(fd, SIOCGIFADDR, &ifr);
    
    close(fd);
    
    return (inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
}

// ip_to_int_arr
/* Function to convert raspberry pi IP address into
   integer array containing all the parts of the IP
*/
void ip_to_int_arr(char * IP_addr, int UQ1[4]) {
    sscanf(IP_addr, "%d.%d.%d.%d\n", &UQ1[0], &UQ1[1], &UQ1[2], &UQ1[3]);
    return;
}

void on_connect(struct mosquitto *mosq, void *obj, int rc) {
	printf("ID: %d\n", * (int *) obj);
	if (rc) {
		printf("Error with result code: %d\n", rc);
		exit(-1);
	}
	mosquitto_subscribe(mosq, NULL, "Election", 0);
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
	printf("New message with topic %s: %s\n", msg->topic, (char *) msg->payload);
}

void* char_dev_thread() {
	int cdev_id, dummy;
	char buffer[MSG_SIZE];
	
	// Open the Character Device for writing
	if((cdev_id = open(CHAR_DEV,O_RDWR)) == -1) {
		printf("Cannot open device %s\n", CHAR_DEV);
		exit(1);
	}

	// while(1)
	// {
	// 	printf("Enter message (! to exit): ");
	// 	fflush(stdout);
	// 	gets(buffer);	// deprecated function, but it servers the purpose here...
	// 					// One must be careful about message sizes on both sides.
	// 	if(buffer[0] == '!')	// If the first character is '!', get out
	// 		break;
		
	// 	dummy = write(cdev_id, buffer, sizeof(buffer));
	// 	if(dummy != sizeof(buffer)) {
	// 		printf("Write failed, leaving...\n");
	// 		break;
	// 	}
	// }

	close(cdev_id);	// close the device.
}

int main(void)
{
	// Vars for IP checking
	char buffer[MSG_SIZE];	// to store received messages or messages to be sent.
   	char rasp_ip_addr[IP_MAX_SIZE]; // to store IP address of raspberry pi
   	char copy_rasp_ip_addr[IP_MAX_SIZE]; // to store IP address of raspberry pi
   	int UQ1[4]; // integer array to hold parts of IP address for raspberry pi
   	int UQ2[4]; // integer array to hold parts of IP address read from socket
   	int rand_num = 0;
   	int VOTED = 0;
   	int FLAG = 0;
   	int recv_num = 0;
   	int max_num = -1;
   	int max_ip = -1;

	// Get IP address for rapsberry pi
	// Determine the IP of the raspberry pi
   	strncpy(rasp_ip_addr, get_wlan0_ip_addr(), MSG_SIZE);
	strncpy(copy_rasp_ip_addr, get_wlan0_ip_addr(), MSG_SIZE);
   	// Split up IP addr of raspberry pi into integers
   	ip_to_int_arr(rasp_ip_addr, UQ1);
	
	// ID for mosquitto object
	int rc, id=12;

	// Initialize and create new mosquitto object
	mosquitto_lib_init();
	struct mosquitto *mosq;
	
	mosq = mosquitto_new(MOSQ_ID, true, &id);
	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_message_callback_set(mosq, on_message);

	rc = mosquitto_connect(mosq, "128.206.19.11", 1883, 10);
	if (rc)
	{
		printf("Could not connect to Broker with return code %d\n", rc);
		return -1;
	}
	
	mosquitto_loop_start(mosq);

	mosquitto_disconnect(mosq); // disconnect from Broker
	mosquitto_destroy(mosq); // destroy mosquitto object
	mosquitto_lib_cleanup(); // clean up mosquitto resources
	return 0;  		// dummy used to prevent warning messages...
}
