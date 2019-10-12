#include "pthread.h"
#include "unistd.h"
#include <cstdio>
#include <chrono>

//compile with g++ main.cpp -lpthread

using namespace std;
using namespace std::chrono;

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

pthread_rwlock_t lock_a;
pthread_rwlock_t lock_b;
pthread_rwlock_t lock_c;
pthread_rwlock_t lock_d;

pthread_barrier_t timing_barrier;

//this is the method for process 1
void *calculate_next_step(void *threadid) {
    //true=buffer_a
    //false=buffer_b
    bool which_buffer = true;
    while (true) {
        if (which_buffer) {
            //buffer a will be read from
            pthread_rwlock_rdlock(&lock_a);
            //x moves diagonally
            int next_row_x = (buffer_a[X].row + 1) % 8;
            int next_col_x = (buffer_a[X].col + 1) % 7;

            //y moves vertically
            int next_row_y = (buffer_a[Y].row + 1) % 8;
			int next_col_y = 2;

            //z moves horizontally
			int next_row_z = 3;
            int next_col_z = (buffer_a[Z].col + 1) % 7;
            pthread_rwlock_unlock(&lock_a);

            //buffer b will be written to
            pthread_rwlock_wrlock(&lock_b);
            //saves the next position of train X in buffer b
            buffer_b[X].buffer[buffer_b[X].row][buffer_b[X].col] = false;
            buffer_b[X].buffer[next_row_x][next_col_x] = true;
            buffer_b[X].row = next_row_x;
            buffer_b[X].col = next_col_x;

            //saves the next position of train Y in buffer b
            buffer_b[Y].buffer[buffer_b[Y].row][buffer_b[Y].col] = false;
            buffer_b[Y].buffer[next_row_y][next_col_y] = true;
            buffer_b[Y].row = next_row_y;
			buffer_b[Y].col = next_col_y;

            //saves the next position of train Z in buffer b
            buffer_b[Z].buffer[buffer_b[Z].row][buffer_b[Z].col] = false;
            buffer_b[Z].buffer[next_row_z][next_col_z] = true;
			buffer_b[Z].row = next_row_z;
            buffer_b[Z].col = next_col_z;
            pthread_rwlock_unlock(&lock_b);
        } else {
            //buffer b will be read from
            pthread_rwlock_rdlock(&lock_b);
            //x moves diagonally
            int next_row_x = (buffer_b[X].row + 1) % 8;
            int next_col_x = (buffer_b[X].col + 1) % 7;

            //y moves vertically
            int next_row_y = (buffer_b[Y].row + 1) % 8;
			int next_col_y = 2;

            //z moves horizontally
			int next_row_z = 3;
            int next_col_z = (buffer_b[Z].col + 1) % 7;
            pthread_rwlock_unlock(&lock_b);

            //buffer a will be written to
            pthread_rwlock_wrlock(&lock_a);
            //saves the next position of train X in buffer a
            buffer_a[X].buffer[buffer_a[X].row][buffer_a[X].col] = false;
            buffer_a[X].buffer[next_row_x][next_col_x] = true;
            buffer_a[X].row = next_row_x;
            buffer_a[X].col = next_col_x;

            //saves the next position of train Y in buffer b
            buffer_a[Y].buffer[buffer_a[Y].row][buffer_a[Y].col] = false;
            buffer_a[Y].buffer[next_row_y][next_col_y] = true;
            buffer_a[Y].row = next_row_y;
			buffer_a[Y].col = next_col_y;

            //saves the next position of train Z in buffer b
            buffer_a[Z].buffer[buffer_a[Z].row][buffer_a[Z].col] = false;
            buffer_a[Z].buffer[next_row_z][next_col_z] = true;
			buffer_a[Z].row = next_row_z;
            buffer_a[Z].col = next_col_z;
            pthread_rwlock_unlock(&lock_a);
        }
        which_buffer = !which_buffer;
        //waits for each of the other threads to finish execution
        pthread_barrier_wait(&timing_barrier);
    }
}

//this is the method for process 2
void *determine_current_positions(void *threadid){
    //true=buffer_a and buffer_c
    //false=buffer_b and buffer_d
    bool which_buffer = true;
    while(true){
        if(which_buffer){
            //buffer_a will be read from
            pthread_rwlock_rdlock(&lock_a);
            //stores the current position of each of the trains
            int current_row_x = buffer_a[X].row;
            int current_col_x = buffer_a[X].col;

            int current_row_y = buffer_a[Y].row;
            int current_col_y = buffer_a[Y].col;

            int current_row_z = buffer_a[Z].row;
            int current_col_z = buffer_a[Z].col;
            pthread_rwlock_unlock(&lock_a);

            //buffer c will be updated to current position values
            pthread_rwlock_wrlock(&lock_c);
            //updates the appropriate buffer with current train positions. Stored as a character
            buffer_c[0][1] = '0' + current_row_x;
            buffer_c[0][2] = '0' + current_col_x;

            buffer_c[1][1] = '0' + current_row_y;
            buffer_c[1][2] = '0' + current_col_y;

            buffer_c[2][1] = '0' + current_row_z;
            buffer_c[2][2] = '0' + current_col_z;
            pthread_rwlock_unlock(&lock_c);
        }
        else{
            //buffer_b will be read from
            pthread_rwlock_rdlock(&lock_b);
            //stores the current position of each of the trains
            int current_row_x = buffer_b[X].row;
            int current_col_x = buffer_b[X].col;

            int current_row_y = buffer_b[Y].row;
            int current_col_y = buffer_b[Y].col;

            int current_row_z = buffer_b[Z].row;
            int current_col_z = buffer_b[Z].col;
            pthread_rwlock_unlock(&lock_b);

            //buffer d will be updated to current position values
            pthread_rwlock_wrlock(&lock_d);
            //updates the appropriate buffer with current train positions. Stored as a character
            buffer_d[0][1] = '0' + current_row_x;
            buffer_d[0][2] = '0' + current_col_x;

            buffer_d[1][1] = '0' + current_row_y;
            buffer_d[1][2] = '0' + current_col_y;

            buffer_d[2][1] = '0' + current_row_z;
            buffer_d[2][2] = '0' + current_col_z;
            pthread_rwlock_unlock(&lock_d);
        }
        which_buffer = !which_buffer;
        //waits for each of the other threads to finish execution
        pthread_barrier_wait(&timing_barrier);
    }
}

//main thread is process 3
int main() {
    pthread_t process1;
    pthread_t process2;

    pthread_rwlock_init(&lock_a, nullptr);
    pthread_rwlock_init(&lock_b, nullptr);
    pthread_rwlock_init(&lock_c, nullptr);
    pthread_rwlock_init(&lock_d, nullptr);

    pthread_barrier_init(&timing_barrier, nullptr, 3);

    //initializes buffers for starting values
    buffer_a[X].buffer[0][0] = true;
    buffer_a[X].row = 0;
    buffer_a[X].col = 0;

    buffer_a[Y].buffer[0][2] = true;
    buffer_a[Y].row = 0;
    buffer_a[Y].col = 2;

    buffer_a[Z].buffer[3][6] = true;
    buffer_a[Z].row = 3;
    buffer_a[Z].col = 6;

    buffer_c[0][0] = 'X';
    buffer_c[1][0] = 'Y';
    buffer_c[2][0] = 'Z';

    buffer_d[0][0] = 'X';
    buffer_d[1][0] = 'Y';
    buffer_d[2][0] = 'Z';

    //creates the two additional threads
    pthread_create(&process1, nullptr, calculate_next_step, nullptr);
    pthread_create(&process2, nullptr, determine_current_positions, nullptr);

    //allows time for buffer c to be filled to be checked by P3
    sleep(1);
    pthread_barrier_wait(&timing_barrier);

    //odd time=buffer_c
    //even time=buffer_d
	int time = 1;
	//time_point start and finish are used to track how long P3 takes to run on each iteration
    high_resolution_clock::time_point start, finish;
    //P3 will be for collision detection
    while(true){
		bool collision = false;
		start = high_resolution_clock::now();
		//the current time will determine which buffer to read from
		if(time%2 != 0){
			pthread_rwlock_rdlock(&lock_c);
			//checks the buffer to see if any of the trains are at the same point
			for(int row = 0; row < 3; row++){
				for(int compare_row = row+1; compare_row < 3; compare_row++){
					if(buffer_c[row][1] == buffer_c[compare_row][1] && buffer_c[row][2] == buffer_c[compare_row][2]){
						collision = true;
						printf("Collision between %c and %c at second %d, location (%c,%c).\n", buffer_c[row][0], buffer_c[compare_row][0], time, buffer_c[row][1], buffer_c[row][2]);
						break;
					}
				}
			}
			pthread_rwlock_unlock(&lock_c);
		}
		else{
			pthread_rwlock_rdlock(&lock_d);
			//checks the buffer to see if any of the trains are at the same point
			for(int row = 0; row < 3; row++){
				for(int compare_row = row+1; compare_row < 3; compare_row++){
					if(buffer_d[row][1] == buffer_d[compare_row][1] && buffer_d[row][2] == buffer_d[compare_row][2]){
						collision = true;
						printf("Collision between %c and %c at second %d, location (%c,%c).\n", buffer_d[row][0], buffer_d[compare_row][0], time, buffer_d[row][1], buffer_d[row][2]);
						break;
					}
				}
			}
			pthread_rwlock_unlock(&lock_d);
		}
		if(!collision){
			printf("No collision at time %d.\n", time);
		}
		time++;
        finish = high_resolution_clock::now();

        //sleeps for (1 second) - (execution time)
        timespec tim;
        tim.tv_sec = 0;
		tim.tv_nsec = 1000000000L - duration_cast<nanoseconds>(finish-start).count();
		nanosleep(&tim, nullptr);

		//barrier is used to ensure that each of the 3 processes are synchronized
		pthread_barrier_wait(&timing_barrier);
    }

    pthread_join(process1, nullptr);
    pthread_join(process2, nullptr);
    return 0;
}