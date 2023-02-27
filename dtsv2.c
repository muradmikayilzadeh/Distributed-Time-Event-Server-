/* 
 *
 * Author: Murad Mikayilzade
 * Login: muradm
 * Project: CIS 415 P3
 * This is my own work except extractWords function provided by Joe Sventek.
 *  
*/
#include <valgrind/valgrind.h>
#include "BXP/bxp.h" 
#include "ADTs/queue.h"
#include <assert.h> 
#include <stdlib.h>
#include <stdio.h> 
#include <string.h> 
#include <pthread.h>

#define UNUSED __attribute__((unused)) 
 
#define PORT 19999
#define SERVICE "DTS"


int extractWords(char *buf, char *sep, char *words[]) {
    int i;
    char *p;

    for (p = strtok(buf, sep), i = 0; p != NULL; p = strtok(NULL, sep),i++)
        words[i] = p;
    words[i] = NULL;
    return i;
}

void *svcRoutine(void *args) {

    BXPService svc = (BXPService *)args;
    BXPEndpoint ep; 

    char* query = (char *)malloc(BUFSIZ);
    char* response = (char *)malloc(BUFSIZ);

    unsigned qlen, rlen; 
    char*w[100];
    int numArgs;

    while ((qlen = bxp_query(svc, &ep, query, BUFSIZ)) > 0) { 
        char originalQuery[1000];
        strcpy(originalQuery, query);

        numArgs = extractWords(query, "|", w);


        if((strcmp(w[0], "OneShot") == 0) && numArgs == 7){
            sprintf(response, "1%s", originalQuery);
            VALGRIND_MONITOR_COMMAND("leak_check summary"); 

        }else if((strcmp(w[0], "Repeat") == 0) && numArgs == 7){
            sprintf(response, "1%s", originalQuery);
            VALGRIND_MONITOR_COMMAND("leak_check summary"); 

        }else if((strcmp(w[0], "Cancel") == 0) && numArgs == 2){
            sprintf(response, "1%s", originalQuery);
            VALGRIND_MONITOR_COMMAND("leak_check summary"); 

        }else{
            sprintf(response, "0");

        }

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