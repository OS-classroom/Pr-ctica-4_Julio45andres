/**
 *   \file psinfo-l.c
 *   \brief base code for the program psinfo-l
 *          
 *  This program prints some basic information for a given 
 *  list of processes.
 *  You can use this code as a basis for implementing parallelization 
 *  through the pthreads library. 
 *
 *   \author: Danny Munera - Sistemas Operativos UdeA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_BUFFER 100
//#define DEBUG

typedef struct p_ {
  int pid;
  char name[MAX_BUFFER];
  char state[MAX_BUFFER];
  char vmsize[MAX_BUFFER];
  char vmdata[MAX_BUFFER];
  char vmexe[MAX_BUFFER];
  char vmstk[MAX_BUFFER];
  int voluntary_ctxt_switches;
  int nonvoluntary_ctxt_switches;
} proc_info;

typedef struct t_p{
  int pid;
  int pos;
} t_params;

pthread_mutex_t mutex;
sem_t empty;
sem_t full;
//sem_t mutex;
pthread_mutex_t lock;

proc_info* proc_buff;
int n_procs;

void* put_info(void* _params);  // Productor
void* consume_info(void* nope); // Consumidor

proc_info* load_info(int pid);
void print_info(proc_info* pi);
  
int main(int argc, char *argv[]){
  int i;
  // number of process ids passed as command line parameters
  // (first parameter is the program name) 
  //proc_info item;
  n_procs = argc - 1;
  
  if(argc < 2){
    printf("Error\n");
    exit(1);
  }

  /* mutex init*/
  if (pthread_mutex_init(&lock, NULL) != 0)
  {
   printf("\n mutex init failed\n");
   return 1;
  }
  /* mutex init*/
  if (pthread_mutex_init(&mutex, NULL) != 0)
  {
   printf("\n mutex init failed\n");
   return 1;
  }

 // sem_init(&mutex, 0, 1);
  sem_init(&empty, 0, n_procs);
  sem_init(&full, 0, 0);

  pthread_t* threads = malloc(sizeof(pthread_t)*n_procs);
  pthread_t print_tread;
  assert(threads!=NULL);

  /*Allocate memory for each process info*/
  proc_buff = malloc(sizeof(proc_info)*n_procs);
  assert(proc_buff!=NULL);
  
  t_params* params = malloc(sizeof(t_params)*n_procs);
  assert(params!=NULL);
  
  // = malloc(sizeof(t_params));
  //assert(params!=NULL);
  // Get information from status file
  for(i = 0; i < n_procs; i++){
    int pid = atoi(argv[i+1]);
    //printf("%d\n", pid);
    params[i].pid = pid;
    params[i].pos = i;
    pthread_create(&threads[i], NULL, &put_info, &params[i]);
  }
  pthread_create(&print_tread, NULL, &consume_info, NULL);

  for(i = 0; i < n_procs; i++){
    pthread_join(threads[i], NULL);
  }
  pthread_join(print_tread, NULL);

  //print_info(&proc_buff[0]);
  /*Machete debugger*/
  //printf("pppp\n");
  //exit(1);
  
  // free heap memory
  free(proc_buff);
  /* mutex destroy*/
  pthread_mutex_destroy(&lock);
  //sem_destroy(&mutex);
  pthread_mutex_destroy(&mutex);
  sem_destroy(&empty);
  sem_destroy(&full);
  return 0;
}

// Productor
void* put_info(void* _params){
  int pos;
  t_params* params = (t_params*) _params;
  // Inicio región crítica
  pthread_mutex_lock(&lock);
  sem_wait(&empty);
  //sem_wait(&mutex);
  pthread_mutex_lock(&mutex);
  proc_info* myinfo = load_info(params->pid);
  if(sem_getvalue(&full, &pos) != 0){
    printf("\nCan't get value for full semaphore from producer\n");
    exit(1);
  }
  proc_buff[pos] = *myinfo; // Produce
  // /*Machete debugger*/
  // if(pos == 1){
  //   printf("pppp %d\n", pos);
  //   exit(1);
  // }
  free(myinfo);
  pthread_mutex_unlock(&mutex);
  //sem_post(&mutex);
  sem_post(&full);
  pthread_mutex_unlock(&lock);
  // Fin región crítica
  return NULL;
}

// Consumidor
void* consume_info(void* nope){
  int pos, i;
  for(i = 0; i < n_procs; i++){
    // Inicio región crítica
    sem_wait(&full);
    //sem_wait(&mutex);
    pthread_mutex_lock(&mutex);
    if(sem_getvalue(&full, &pos) != 0){
      printf("\nCan't get value for full semaphore from consumer\n");
      exit(1);
    }
    print_info(&proc_buff[pos]); //Consume
    pthread_mutex_unlock(&mutex);
    //sem_post(&mutex);
    sem_post(&empty);
    // Fin región crítica
  }
  return NULL;
}

/**
 *  \brief load_info
 *
 *  Load process information from status file in proc fs
 *
 *  \param pid    (in)  process id 
 *  \param myinfo (out) process info struct to be filled
 */
proc_info* load_info(int pid){
  proc_info* myinfo = malloc(sizeof(proc_info));
  FILE *fpstatus;
  char buffer[MAX_BUFFER]; 
  char path[MAX_BUFFER];
  char* token;
  
  //printf("%d\n", pid);
  sprintf(path, "/proc/%d/status", pid);
  fpstatus = fopen(path, "r");
  assert(fpstatus != NULL);
#ifdef DEBUG
  printf("%s\n", path);
#endif // DEBUG
  myinfo->pid = pid;
  while (fgets(buffer, MAX_BUFFER, fpstatus)) {
    token = strtok(buffer, ":\t");
    if (strstr(token, "Name")){
      token = strtok(NULL, ":\t");
#ifdef  DEBUG
      printf("%s\n", token);
#endif // DEBUG
      strcpy(myinfo->name, token);
    }else if (strstr(token, "State")){
      token = strtok(NULL, ":\t");
      strcpy(myinfo->state, token);
    }else if (strstr(token, "VmSize")){
      token = strtok(NULL, ":\t");
      strcpy(myinfo->vmsize, token);
    }else if (strstr(token, "VmData")){
      token = strtok(NULL, ":\t");
      strcpy(myinfo->vmdata, token);
    }else if (strstr(token, "VmStk")){
      token = strtok(NULL, ":\t");
      strcpy(myinfo->vmstk, token);
    }else if (strstr(token, "VmExe")){
      token = strtok(NULL, ":\t");
      strcpy(myinfo->vmexe, token);
    }else if (strstr(token, "nonvoluntary_ctxt_switches")){
      token = strtok(NULL, ":\t");
      myinfo->nonvoluntary_ctxt_switches = atoi(token);
    }else if (strstr(token, "voluntary_ctxt_switches")){
      token = strtok(NULL, ":\t");
      myinfo->voluntary_ctxt_switches = atoi(token);
    }
#ifdef  DEBUG
    printf("%s\n", token);
#endif
  }
  fclose(fpstatus);
  return myinfo;
}
/**
 *  \brief print_info
 *
 *  Print process information to stdout stream
 *
 *  \param pi (in) process info struct
 */ 
void print_info(proc_info* pi){
  printf("PID: %d \n", pi->pid);
  printf("Nombre del proceso: %s", pi->name);
  printf("Estado: %s", pi->state);
  printf("Tamaño total de la imagen de memoria: %s", pi->vmsize);
  printf("Tamaño de la memoria en la región TEXT: %s", pi->vmexe);
  printf("Tamaño de la memoria en la región DATA: %s", pi->vmdata);
  printf("Tamaño de la memoria en la región STACK: %s", pi->vmstk);
  printf("Número de cambios de contexto realizados (voluntarios"
	 "- no voluntarios): %d  -  %d\n\n", pi->voluntary_ctxt_switches,  pi->nonvoluntary_ctxt_switches);
}
