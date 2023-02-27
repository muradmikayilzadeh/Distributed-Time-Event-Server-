/* 
 *
 * Author: Murad Mikayilzade
 * Login: muradm
 * Project: CIS 415 P3
 * This is my own work except extractWords function provided by Joe Sventek. And I received help from TAs and GE in terms of pseudocode.
 *  
*/
#include <valgrind/valgrind.h>
#include "BXP/bxp.h" 
#include "ADTs/queue.h"
#include "ADTs/heapprioqueue.h"
#include "ADTs/hashmap.h"
#include <assert.h> 
#include <stdlib.h>
#include <stdio.h> 
#include <string.h> 
#include <pthread.h>
#include <sys/time.h>
#include <stdint.h>
#include <unistd.h>

#define UNUSED __attribute__((unused)) 
 
#define PORT 19999
#define SERVICE "DTS"
#define USECS (10 * 1000)


typedef struct request {
    unsigned long clid;
    unsigned long secs;
    unsigned long usecs;
    char* host;
    char* service;
    unsigned short port;
    unsigned long msecs;
    int repeats;
    unsigned long svid;
    int isCancelled;
} Request;



unsigned long svid = 0;
const PrioQueue *pq = NULL;
const Map *hm;


int cmpPrior(void *p1, void *p2) {
    
    struct timeval *t1 = (struct timeval*)p1;
    struct timeval *t2 = (struct timeval*)p2;

    int c;

    if(t1->tv_sec < t2->tv_sec){
        c = -1;
    }else if(t1->tv_sec > t2->tv_sec){
        c= 1;
    }else{
        if(t1->tv_usec < t2->tv_usec){
            c = -1;
        }else if(t1->tv_usec > t2->tv_usec){
            c= 1;
        }else{
            c= 0;
        }
    }

    return c;

}


int hashCmp(void* v1, void* v2){
    int val1 = (long)v1;
    int val2 = (long)v2;

    int c;
    if(val1 < val2){
        c = -1;
    }else if(val1 > val2){
        c = 1;
    }else{
        c = 0;
    }

    return c;  
}


long hash(void *key, long N){
    return (((unsigned long)key)%N);
}

// Helper function for OneShot command
void oneShot(char* args[]){

    Request *request = (Request *)malloc(sizeof(Request));
    struct timeval *time  = (struct timeval*)malloc(sizeof(struct timeval));
    gettimeofday(time, NULL);

    // Initing struct members
    sscanf(args[1], "%lu", &(request->clid));
    sscanf(args[2], "%lu", &(request->secs));
    sscanf(args[3], "%lu", &(request->usecs));

    request->host = strdup(args[4]);
    request->service = strdup(args[5]);

    sscanf(args[6], "%hu", &(request->port));

    request->msecs = 0;
    request->svid = svid;
    request->isCancelled = 0;

    time->tv_sec = request->secs;
    time->tv_usec = request->usecs;

    // Insert to the PQ and HashMap
    pq->insert(pq, (void *)time, (void *)request);
    hm->put(hm, (void*)svid, (void*)request);

    svid++;

}

// Helper function for Repeat command
void repeat(char* args[]){

    Request *request = (Request *)malloc(sizeof(Request));
    struct timeval *time  = (struct timeval*)malloc(sizeof(struct timeval));

    gettimeofday(time, NULL);


    // Initing struct members
    sscanf(args[1], "%lu", &(request->clid));
    sscanf(args[2], "%lu", &(request->msecs));
    sscanf(args[3], "%d", &(request->repeats));

    request->host = strdup(args[4]);
    request->service = strdup(args[5]);

    sscanf(args[6], "%hu", &(request->port));

    request->svid = svid;
    request->isCancelled = 0;
    
    time->tv_usec = request->msecs * 1000;
    while(time->tv_usec > 1000000){
        time->tv_usec = time->tv_usec - 1000000;
        time->tv_sec++;
    }

    // Insert to the PQ and HashMap
    pq->insert(pq, time, (void *)request);
    hm->put(hm, (void*)svid, (void*)request);


    svid++;

}

// Helper function for Cancel command
void cancel(char* args[]){

    unsigned long id;
    sscanf(args[1], "%lu", &id);
    id--;
    Request *request;

    if(hm->get(hm, (void*)id, (void**)&request)){
        request->isCancelled = 1;
    }

}

// Helper function for extracting words from request
int extractWords(char *buf, char *sep, char *words[]) {
    int i;
    char *p;

    for (p = strtok(buf, sep), i = 0; p != NULL; p = strtok(NULL, sep),i++)
        words[i] = p;
    words[i] = NULL;
    return i;
}

// Routine function for thread 1
void *svcRoutine(void *args) {

    BXPService svc = (BXPService *)args;
    BXPEndpoint ep; 

    char* query = (char *)malloc(BUFSIZ);
    char* response = (char *)malloc(BUFSIZ);

    unsigned qlen, rlen; 
    char*w[100];
    int numArgs;

    // read lines
    while ((qlen = bxp_query(svc, &ep, query, BUFSIZ)) > 0) { 

        char originalQuery[1000];
        strcpy(originalQuery, query);

        numArgs = extractWords(query, "|", w);

        //OneShot command
        if((strcmp(w[0], "OneShot") == 0) && numArgs == 7){

            oneShot(w);
            sprintf(response, "1%08lu", svid);
            VALGRIND_MONITOR_COMMAND("leak_check summary"); 

        }
        // Repeat command
        else if((strcmp(w[0], "Repeat") == 0) && numArgs == 7){

            repeat(w);
            sprintf(response, "1%08lu", svid);
            VALGRIND_MONITOR_COMMAND("leak_check summary"); 

        }
        // Cancel command
        else if((strcmp(w[0], "Cancel") == 0) && numArgs == 2){

            cancel(w);
            sprintf(response, "1%08lu", svid);
            VALGRIND_MONITOR_COMMAND("leak_check summary"); 

        }
        // Illegal command
        else{
            sprintf(response, "0");

        }

        rlen = strlen(response) + 1; 
        assert(bxp_response(svc, &ep, response, rlen));
    } 
    free(query);
    free(response);
    pthread_exit(NULL);

}

// Ruotine function for Timer thread
void *timer(){
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    struct timeval *priority;
    struct timeval *repeat_time = (struct timeval *)malloc(sizeof(struct timeval));
    struct timeval now;

    Request *request;


    for(;;){ 

        // Creating ready and repeat queues
        const Queue *ready_q = NULL;
        const Queue *repeat_q = NULL;   
        ready_q = Queue_create(doNothing);
        repeat_q = Queue_create(doNothing);    

        usleep(USECS);
        gettimeofday(&now, NULL);

        // Removing requests from PQ
        pthread_mutex_lock(&mutex);
        while(pq->min(pq, (void**)&priority, (void**)&request)){
            if((cmpPrior((void*)priority, (void*)&now)) != 1){
                pq->removeMin(pq, (void**)&priority, (void**)&request);
                ready_q->enqueue(ready_q, (void*)request);
            }else{
                break;
            }
        }
        pthread_mutex_unlock(&mutex);

        // Removing from ready queue
        while(ready_q->dequeue(ready_q, (void**)&request)){
                // If the request is cancelled, then remove from HM and free
                if(request->isCancelled == 1){
                    hm->remove(hm, (void*)request->svid);
                    free(request->host);
                    free(request->service);
                    free(request);
                    break;
                }

                // Fire event
                printf("Event fired: %lu|%s|%s|%u\n", request->clid, request->host, request->service, request->port);
                if((--request->repeats > 0)){
                    repeat_q->enqueue(repeat_q, (void*)request);
                }else{
                    hm->remove(hm, (void*)request->svid);
                    free(request->host);
                    free(request->service);
                    free(request);
                }

        }

        pthread_mutex_lock(&mutex);
        while(repeat_q->dequeue(repeat_q, (void**)&request)){
            repeat_time->tv_usec = (now.tv_usec+(request->msecs*1000));
            repeat_time->tv_sec = now.tv_sec;
            while(repeat_time->tv_usec > 1000000){
                repeat_time->tv_usec = repeat_time->tv_usec - 1000000;
                repeat_time->tv_sec++;

            }

            if(request->isCancelled == 0){
                pq->insert(pq, (void*)repeat_time, (void*)request);
            }
        }
        pthread_mutex_unlock(&mutex);

        repeat_q->destroy(repeat_q);
        ready_q->destroy(ready_q);

    }
    free(repeat_time);
    free(priority);   
    pthread_exit(NULL);
}

int main(UNUSED int argc, UNUSED char *argv[]) { 

    BXPService svc;

    if ((pq = HeapPrioQueue(cmpPrior, doNothing, doNothing)) == NULL) {
        fprintf(stderr, "Unable to create Priority Queue\n");
        return EXIT_FAILURE;
    }

    if ((hm = HashMap(1024L, 2.0, hash, hashCmp, doNothing, doNothing)) == NULL) {
        fprintf(stderr, "Unable to create HashMap\n");
        return EXIT_FAILURE;
    }


    char* service;
    unsigned short port;
    
    pthread_t svcThread;
    pthread_t timerThread;

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


    if(pthread_create(&timerThread, NULL, timer, NULL) != 0) {
        fprintf(stderr, "Can't start timer thread\n");
    }

    pthread_join(svcThread, NULL);
    pthread_join(timerThread, NULL);

    hm->destroy(hm);
    pq->destroy(pq);

    free(service);

    return 0; 
} 