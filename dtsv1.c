/* 
 *
 * Author: Murad Mikayilzade
 * Login: muradm
 * Project: CIS 415 P3
 * This is my own work.
 *  
*/
#include "BXP/bxp.h" 
#include <assert.h> 
#include <stdlib.h>
#include <stdio.h> 
#include <string.h> 
#include <pthread.h>

#define UNUSED __attribute__((unused)) 
 
#define PORT 19999
#define SERVICE "DTS"


void *svcRoutine(void *args) {

    BXPService svc = (BXPService *)args;
	BXPEndpoint ep; 
	char* query = (char *)malloc(BUFSIZ);
    char* response = (char *)malloc(BUFSIZ);
    unsigned qlen, rlen; 

    while ((qlen = bxp_query(svc, &ep, query, BUFSIZ)) > 0) { 
        sprintf(response, "1%s", query); 
        rlen = strlen(response) + 1; 
 		assert(bxp_response(svc, &ep, response, rlen)); 
    } 
    free(query);
    free(response);

    pthread_exit(NULL);
}

int main(UNUSED int argc, UNUSED char *argv[]) { 

    
    BXPService svc; 


    char* service;
    unsigned short port;
    
    pthread_t svcThread;

    service = SERVICE;
    port = PORT;
 
    assert(bxp_init(port, 1)); 

    if ((svc = bxp_offer(service)) == NULL) {
        fprintf(stderr, "Unable to create BXP service\n");
		exit(EXIT_FAILURE);
    }

    if (pthread_create(&svcThread, NULL, svcRoutine, (void *)svc)) {
        fprintf(stderr, "Unable to create thread\n");
		exit(EXIT_FAILURE);
    }

    pthread_join(svcThread, NULL);

    free(service);
    

    return 0; 
} 