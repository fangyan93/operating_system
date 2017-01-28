/* octuplets-threads-mutex.c */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define CHILDREN 8     /* based on a true story... */

/* function executed by each thread */
void * whattodo( void * arg );

/* function that contains critical section code */
void critical_section( int time );

/* global mutex variable */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;    /*****************/

int main()
{
    pthread_t tid[ CHILDREN ];   /* keep track of the thread IDs */
    int i, rc;
    
#if 0
    int t;   /* this is STATICALLY ALLOCATED memory.... */
    /* ....which means that the input to each thread
     will be the same pointer to this variable t,
     thus causing corruption in the threads */
#endif
    
    int * t;  /* this will be dynamically allocated memory... */
    
    /* create the threads */
    for ( i = 0 ; i < CHILDREN; i++ )
    {
        t = (int *)malloc( sizeof( int ) );
        *t = 2 + i * 2;   /* 2, 4, 6, 8, 10, ... */
        
        printf( "MAIN: Next thread will nap for %d seconds.\n", *t );
        
        rc = pthread_create( &tid[i], NULL, whattodo, t );
        
        if ( rc != 0 )
        {
            fprintf( stderr, "MAIN: Could not create child thread (%d)\n", rc );
        }
    }
    
    /* wait for threads to terminate */
    for ( i = 0 ; i < CHILDREN ; i++ )
    {
        unsigned int * x;
        pthread_join( tid[i], (void **)&x );    /* BLOCKING CALL */
        /* ^^^^^^^^^^^
         |
         this is the return value from pthread_exit() */
        
        printf( "MAIN: Joined a child thread that returned %u.\n", *x );
        free( x );
    }
    
    printf( "MAIN: All threads successfully joined.\n" );
    
    return EXIT_SUCCESS;
}

void critical_section( int time )   /*****************************/
{
    printf( "THREAD %u: Entering my critical section.\n",
           (unsigned int)pthread_self() );
    sleep( time );
    printf( "THREAD %u: Leaving my critical section.\n",
           (unsigned int)pthread_self() );
}

/* function executed by each thread */
void * whattodo( void * arg )
{
    int t = *(int *)arg;
    free( arg );
    
    printf( "THREAD %u: I'm gonna nap for %d seconds.\n",
           (unsigned int)pthread_self(), t );
    
    pthread_mutex_lock( &mutex );     /******************************/
    critical_section( t );
    pthread_mutex_unlock( &mutex );
    
    /* dynamically allocate space to hold a return value */
    unsigned int * x = (unsigned int *)malloc( sizeof( unsigned int ) );
    
    /* return value is simply the thread ID */
    *x = pthread_self();
    pthread_exit( x );   /* terminate the thread, returning x to
                          a waiting pthread_join() call */
    return NULL;
}