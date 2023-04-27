#include <stdio.h>
#include <mosquitto.h>

int main(){
	int rc;
	struct mosquitto * mosq;

	mosquitto_lib_init();

	char pubname[]="TAPublisher";

	mosq = mosquitto_new(pubname, true, NULL);

    // char *host="128.206.19.11";     
    // rc = mosquitto_connect(mosq, host, 1883, 60);

    printf("%d\n",rc);
	rc = mosquitto_connect(mosq, "localhost", 1883, 60);
	if(rc != 0){
		printf("Client could not connect to broker! Error Code: %d\n", rc);
		mosquitto_destroy(mosq);
		return -1;
	}
	printf("We are now connected to the broker!\n");
	char buffer[40];
	while(1){
		scanf("%s",buffer);
		mosquitto_publish(mosq, NULL, "Election", sizeof(buffer),buffer, 0, false);
	}
	
	mosquitto_disconnect(mosq);
	mosquitto_destroy(mosq);

	mosquitto_lib_cleanup();
	return 0;
}
