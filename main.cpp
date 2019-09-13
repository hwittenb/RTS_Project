#include <shared_mutex>
#include <mutex>
#include "pthread.h"
#include "unistd.h"
#include "stdio.h"

using namespace std;

struct grid_buffer{
    bool buffer[8][7];
    int row;
    int col;
};

//enum created for readability
enum train{
    X, Y, Z
};

grid_buffer buffer_a[3];
grid_buffer buffer_b[3];
char buffer_c[3][3];
char buffer_d[3][3];

shared_timed_mutex mutex_a;
shared_timed_mutex mutex_b;
shared_timed_mutex mutex_c;
shared_timed_mutex mutex_d;

void *test(void *threadid){
    mutex_a.lock_shared();
    long tid = (long)threadid;
    for(int i = 0; i < 10; i++)
    printf("hello from thread %d\n", tid);
    mutex_a.unlock_shared();
    pthread_exit(NULL);
}

//main thread is process 3
int main() {
    pthread_t process1;
    pthread_t process2;

    buffer_a[X].buffer[0][0] = true;
    buffer_a[X].row = 0;
    buffer_a[X].col = 0;

    buffer_a[Y].buffer[0][2] = true;
    buffer_a[Y].row = 0;
    buffer_a[Y].col = 2;

    buffer_a[Z].buffer[3][6] = true;
    buffer_a[Z].row = 3;
    buffer_a[Z].col = 6;

    pthread_create(&process1, NULL, test, (void*)1);
    pthread_create(&process2, NULL, test, (void*)2);

    pthread_join(process1, NULL);
    pthread_join(process2, NULL);

/*    pthread_t *threads = new pthread_t[10];
    for(long i = 0; i < 10; i ++){
        pthread_create(&threads[i], NULL, test, (void *)i);
    }

    for(long i = 0; i < 10; i++){
        pthread_join(threads[i], NULL);
    }

    delete [] threads;*/
    return 0;
}