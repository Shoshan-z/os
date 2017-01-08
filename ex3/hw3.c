#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

#define MAX_READER_THREADS 1000 
#define MAX_WRITER_THREADS 100
//in the forum it was said that RNUM,WNUM<1000

//global
int stop_readers = 0; 
int stop_writers = 0; 
int stop_gc = 0; 

/*****************************INTLIST FUNCTIONS******************/
struct list_node{
  int data;
  struct list_node* prev;
  struct list_node* next; 
};

typedef struct list_node* node; 

typedef struct list{
  node head;
  node tail;
  int size;
  pthread_mutex_t lock;
  pthread_mutexattr_t attr;
  pthread_cond_t  empty_list;
} intlist; 

node create_node(int data) {
  node new_node = NULL;

  new_node = (struct list_node*) malloc(sizeof(node)); 
  if (new_node == NULL){
    printf("error in malloc: %s\n", strerror(errno));
    return NULL; 
  }

  new_node->prev = NULL;
  new_node->next = NULL;
  new_node->data = data;
  
  return new_node; 
}

void destroy_node(node n){
  if (n != NULL) {
    free(n); 
  }
}

void intlist_init(intlist* list) {
  int rc = 0;

  if (list == NULL) {
    printf("error - cannot init list, a null pointer was recieved\n");
    exit(EXIT_FAILURE); 
  }

  //initialize the mutex
  rc = pthread_mutexattr_init(&list->attr);
  if (rc != 0) {
    printf("error in pthread_mutexattr_init(): %s\n", strerror(rc));
    exit(rc);  
   }

  rc = pthread_mutexattr_settype(&list->attr,PTHREAD_MUTEX_RECURSIVE);
  if (rc != 0) {
    printf("error in pthread_mutex_settype(): %s\n", strerror(rc));
    exit(rc);  
   }

  rc = pthread_mutex_init(&list->lock, &list->attr);
  if (rc != 0) {
    printf("error in pthread_mutex_init(): %s\n", strerror(rc));
    exit(rc); 
   }

  //initialize the list  
  list->head = NULL;
  list->tail = NULL;
  list->size =0;

}

void intlist_destroy(intlist* list){
  int rc =0; 

  //pointer is null, or the list is empty, or it contains 1 item
  if (list == NULL) return;

  rc =  pthread_mutexattr_destroy(&list->attr);
  //in case of error, print it and continue freeing resources
  if (rc != 0) {
    printf("error in pthread_mutexattr_destroy(): %s\n", strerror(rc));
   }

  rc =  pthread_mutex_destroy(&list->lock);
  //in case of error, print it and continue freeing resources
  if (rc != 0) {
    printf("error in pthread_mutex_destroy(): %s\n", strerror(rc));
   }
  
  rc =  pthread_cond_destroy(&list->empty_list);
  //in case of error, print it and continue freeing resources
  if (rc != 0) {
    printf("error in pthread_cond_destroy(): %s\n", strerror(rc));
   }
  
  if (list->size == 0){
    free(list); 
    return;
  }
  if (list->size == 1) {
    destroy_node(list->head);
    free(list);
    return; 
  }

  //the list has more than 1 item
  node curr = list->head; 
  node next = list->head->next;

  while (next != NULL) {
    destroy_node(curr);
    curr = next;
    next = next->next;
     
  }
  free(list); 
}

pthread_mutex_t* intlist_get_mutex(intlist* list) {
  if (list == NULL) {
    printf("error - list is NULL\n");
    return NULL; 
  }
  return &list->lock; 
}


void intlist_push_head(intlist* list, int value){
  int rc = 0; 
  node new_node = NULL; 
  pthread_mutex_t* lock_p = NULL;
  
  if (list == NULL) {
    printf("error - list is NULL\n");
    exit(EXIT_FAILURE); 
  }

  new_node = create_node(value);
  if (new_node == NULL) {
    exit(EXIT_FAILURE); 
  }

  //get the lock
  lock_p = intlist_get_mutex(list); 

  rc = pthread_mutex_lock(lock_p);
  if (rc != 0) {
    printf("error in pthread_mutex_lock(): %s\n", strerror(rc));
    exit(rc); 
  }

  //critical section - changing the list 
  if (list->size == 0) {
    list->head = new_node;
    list->tail = new_node;
    list->size++;
    
    
  }
  else {
    list->head->prev = new_node; 
    new_node->next = list->head;
    list->head = new_node;
    list->size++; 
  }

  rc = pthread_cond_signal(&list->empty_list);
  if (rc != 0) {
      printf("error in pthread_cond_signal(): %s\n", strerror(rc));
      exit(rc); 
    }
  rc = pthread_mutex_unlock(lock_p);
  if (rc != 0) {
    printf("error in pthread_mutex_lock(): %s\n", strerror(rc));
    exit(rc); 
  }
  
}

//a utility function, used by pop_tail and remove_last_k
int do_pop_tail(intlist* list) {
  int value = 0;
  node temp_node = NULL; 
  
  if (list->size == 1) {
      value = list->head->data;
      temp_node = list->head;
      list->head = NULL;
      list->tail = NULL;
      list->size--; 
  }
  else {
    temp_node = list->tail; 
    list->tail = list->tail->prev;  
    list->tail->next = NULL;
    value = temp_node->data;
    list->size--;
    }

  if (temp_node != NULL) {
    value = temp_node->data;
    destroy_node(temp_node);   
  }
  
  return value; 
}

int intlist_pop_tail(intlist* list) {
  int rc = 0; 
  pthread_mutex_t* lock_p = NULL; 
  int value = 0;
  node temp_node = NULL; 
  
  if (list == NULL) {
    printf("error - list is NULL\n");
    exit(EXIT_FAILURE); 
  }
  
  //get the lock
  lock_p = intlist_get_mutex(list); 

  rc = pthread_mutex_lock(lock_p);
  if (rc != 0) {
    printf("error in pthread_mutex_lock(): %s\n", strerror(rc));
    exit(rc); 
  }

  //if the list is empty - wait for a signal
  while (list->size == 0) {
    rc = pthread_cond_wait(&list->empty_list, lock_p); 
    if (rc != 0) {
      printf("error in pthread_cond_wait(): %s\n", strerror(rc));
      exit(rc); 
    }
  }

  value = do_pop_tail(list);
  
  rc = pthread_mutex_unlock(lock_p);
  if (rc != 0) {
    printf("error in pthread_mutex_lock(): %s\n", strerror(rc));
    exit(rc); 
  }
  
  return value; 
}


void intlist_remove_last_k(intlist* list, int k) {
  int rc = 0;
  pthread_mutex_t* lock_p = NULL; 
  int items_to_remove = 0;
  int i =0; 

  if (list == NULL) {
    printf("error - list is NULL\n");
    exit(EXIT_FAILURE); 
  }

  if (k<=0) {
    return; 
  }

  lock_p = &list->lock; 
  
  rc = pthread_mutex_lock(lock_p);
  if (rc != 0) {
    printf("error in pthread_mutex_lock(): %s\n", strerror(rc));
    exit(rc); 
  } 
  
  //check how many itmes need to be removed
  if (list->size<k) {
    items_to_remove = list->size; 
  }
  else {
    items_to_remove = k; 
  }
 
  for (i=0; i<items_to_remove; i++) {
    do_pop_tail(list); 
  }
 
  rc = pthread_mutex_unlock(lock_p);
  if (rc != 0) {
    printf("error in pthread_mutex_lock(): %s\n", strerror(rc));
    exit(rc); 
  }
  
}

int intlist_size(intlist* list){
  return list->size; 
}

/*********************SIMULATOR FUNCTIONS*********************************/

//information for the writer and gc threads
typedef struct threads_args{
  intlist* list;
  pthread_mutex_t* gc_lock;
  pthread_cond_t* reached_max;
  int max;
} threads_args;


void* writer(void* t) {
  threads_args* context_args = (threads_args*) t; 
  intlist* list = context_args->list;
  int max_items = context_args->max; 
  pthread_cond_t* reached_max = context_args->reached_max; 
  int rand_num = 0; 
  int rc = 0;
  
  srand(time(NULL));
  
  while (stop_writers != 1) {  
    rand_num = rand();
    intlist_push_head(list, rand_num); 
   
    //if there are more then MAX items - signal the garbage collector
    if (intlist_size(list) > max_items) {
      rc = pthread_cond_signal(reached_max);
      if (rc != 0) {
    	printf("error in pthread_cond_signal(): %s\n", strerror(rc));
    	exit(rc); 
      }
    }
  } 
}

void* reader(void* t){
  threads_args* context_args = (threads_args*) t; 
  intlist* list = context_args->list;
  int max_items = context_args->max; 
  pthread_cond_t* reached_max = context_args->reached_max;
  int rc = 0; 
  
  while (stop_readers != 1) {
    intlist_pop_tail(list);

    //if there are more then MAX items - signal the garbage collector
    if (intlist_size(list) > max_items) {
      rc = pthread_cond_signal(reached_max);
      if (rc != 0) {
	printf("error in pthread_cond_signal(): %s\n", strerror(rc));
	exit(rc); 
      }
    }
  }
}

void* garbage_collector(void *t) {
  threads_args* context_args = (threads_args*) t; 
  intlist* list = context_args->list;
  int max_items = context_args->max; 
  pthread_cond_t* reached_max = context_args->reached_max; 
  pthread_mutex_t* lock = context_args->gc_lock; 
  int rand_num = 0; 
  int rc = 0;
  int items_to_remove = 0; 
  int size = 0; 

  while (stop_gc != 1) {
    rc = pthread_mutex_lock(lock);
    if (rc != 0) {
      printf("error in pthread_mutex_lock(): %s\n", strerror(rc));
      exit(rc); 
    } 

    rc = pthread_cond_wait(reached_max,lock); 
    if (rc != 0) {
      printf("error in pthread_cond_wait(): %s\n", strerror(rc));
      exit(rc); 
    }

    size = intlist_size(list); 
    
    if (size >= max_items) {
      items_to_remove = size/2 + size%2; 
      intlist_remove_last_k(list, items_to_remove);   
    }
    
    rc = pthread_mutex_unlock(lock);
    if (rc != 0) {
      printf("error in pthread_mutex_lock(): %s\n", strerror(rc));
      exit(rc); 
    }

    printf("GC – %d items removed from the list\n", items_to_remove);
  }
}

/***************************UTILITY FUNCTIONS***************************/ 

void parse_cmd_arguments(char** argv, int* WNUM_p, int* RNUM_p, int* MAX_p, int* TIME_p) {

  errno = 0; 
  *WNUM_p = strtol(argv[1], NULL, 10);
  if (errno != 0) {
    printf("error converting WNUM parameter to int: %s\n",strerror(errno));
    exit(errno);  
  }

  errno = 0; 
  *RNUM_p = strtol(argv[2], NULL, 10);
  if (errno != 0) {
    printf("error converting RNUM parameter to int: %s\n",strerror(errno));
    exit(errno);  
  }
  
  errno = 0; 
  *MAX_p = strtol(argv[3], NULL, 10);
  if (errno != 0) {
    printf("error converting MAX parameter to int: %s\n",strerror(errno));
    exit(errno); 
  }
  
  errno = 0; 
  *TIME_p = strtol(argv[4], NULL, 10);
  if (errno != 0) {
    printf("error converting TIME parameter to int: %s\n",strerror(errno));
    exit(errno); 
  }

}

//this function prepars a struct with the needed information for the threads
void init_args(threads_args* args_p, intlist* list, pthread_mutex_t* lock, pthread_cond_t* reached_max, int MAX){

  args_p->list = list;
  args_p->gc_lock = lock;
  args_p->reached_max = reached_max; 
  args_p->max = MAX; 
}


int main(int argc, char* argv[]){
  pthread_mutex_t gc_lock;
  pthread_cond_t reached_max;
  pthread_t reader_threads[MAX_READER_THREADS];
  pthread_t writer_threads[MAX_WRITER_THREADS];
  pthread_t gc_thread; 
  intlist* list = NULL;
  void* status; 
  int rc = 0; 
  threads_args context_args; 
  int MAX = 0;
  int WNUM = 0;
  int RNUM = 0;
  int TIME = 0; 
  int i = 0;
  int final_list_size = 0; 
  
  if (argc != 5) {
    printf("usage: %s <MAX> <WNUM> <RNUM> <TIME>\n", argv[0]);
    exit(EXIT_FAILURE); 
  }
  
  //get cmd arguments
  parse_cmd_arguments(argv, &MAX, &WNUM, &RNUM, &TIME);
  
  //create and initialize the list
  list = (intlist*) malloc(sizeof(intlist));
  if (list == NULL) {
    printf("error in malloc: %s\n", strerror(errno)); 
    exit(errno); 
  }

  intlist_init(list);

  //initialize the mutex lock to be used by the garbage collector
  rc = pthread_mutex_init(&gc_lock, NULL);
  if (rc != 0) {
    printf("error in pthread_mutex_init(): %s\n", strerror(rc));
    exit(rc); 
   }

  //prepare the thread args struct
  init_args(&context_args, list, &gc_lock, &reached_max, MAX);  

  //create the gc thread
  rc = pthread_create(&gc_thread, NULL, garbage_collector, (void*) &context_args); 
  if (rc != 0) {
    printf("error in pthread_create(): %s\n", strerror(rc));
    exit(rc);
   }

  //create writers threads
  for (i=0; i<WNUM; ++i){
      rc = pthread_create(&writer_threads[i], NULL, writer, (void*) &context_args); 
      if (rc != 0) {
	printf("error in pthread_create(): %s\n", strerror(rc));
	exit(rc);
      }
  }
     
  //create readers threads
  for (i=0; i<RNUM; ++i){
      rc = pthread_create(&reader_threads[i], NULL, reader, (void*) &context_args); 
      if (rc != 0) {
	printf("error in pthread_create(): %s\n", strerror(rc));
	exit(rc);
      }
  }

  sleep(TIME);

  //signal the reader threads to stop
  stop_readers = 1;
  for (i=0; i<RNUM; ++i){
    rc = pthread_join(reader_threads[i], &status); 
    if (rc !=0 ){
      printf("error in pthread_join(): %s\n", strerror(rc));
      exit(rc); 
    }
  }

  //signal the writer threads to stop
  stop_writers = 1;
  for (i=1; i<WNUM; ++i){
    rc = pthread_join(writer_threads[i], &status); 
    if (rc !=0 ){
      printf("error in pthread_join(): %s\n", strerror(rc));
      exit(rc); 
    }
  }

  //signal the gc
  stop_gc = 1;
  rc = pthread_cond_signal(&reached_max);
  if (rc != 0) {
    printf("error in pthread_cond_signal(): %s\n", strerror(rc));
    exit(rc); 
  }

  rc = pthread_join(gc_thread, &status); 
  if (rc !=0 ){
    printf("error in pthread_join(): %s\n", strerror(rc));
    exit(rc); 
  }

  final_list_size = intlist_size(list); 
  printf("list size: %d\n", final_list_size);

  for (i=0; i<final_list_size; ++i){
    //while (list != NULL) {
    printf("%d\n", intlist_pop_tail(list)); 
  }
  printf("\n"); 
  
  //cleanup
  intlist_destroy(list); 

  rc =  pthread_mutex_destroy(&gc_lock);
  //in case of error, print it and continue freeing resources
  if (rc != 0) {
    printf("error in pthread_mutex_destroy(): %s\n", strerror(rc));
   }
  
  rc =  pthread_cond_destroy(&reached_max);
  if (rc != 0) {
    printf("error in pthread_cond_destroy(): %s\n", strerror(rc));
   }
  
  return 0;

}
