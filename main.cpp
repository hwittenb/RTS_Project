#include "pthread.h"
#include "unistd.h"
#include <cstdio>
#include <chrono>
#include <semaphore.h>

//compile with g++ main.cpp -lpthread

using namespace std;
using namespace std::chrono;

struct grid_buffer{
    bool buffer[8][7];
    int row;
    int col;
    bool is_moving;
};

struct position_buffer{
    int buffer[3][3];
};

//enum created for readability
enum train{
    X, Y, Z
};

grid_buffer buffer_a[3];
grid_buffer buffer_b[3];

//each row is for each of the three trains. The first 2 columns are for row and column position of that train. The last column is 1 if the train is moving, and 0 if the train is stopped
position_buffer buffer_c;
position_buffer buffer_d;

pthread_rwlock_t lock_a;
pthread_rwlock_t lock_b;
pthread_rwlock_t lock_c;
pthread_rwlock_t lock_d;

pthread_barrier_t timing_barrier;

void print_buffer(grid_buffer* buffer){
    for(int plane = 0; plane<3; plane++){
        for(int row = 0; row < 8; row++){
            for(int col = 0; col < 7; col++){
                printf("%d | ", buffer[plane].buffer[row][col]);
            }
            printf("\n");
        }
        printf("___________________________________________\n");
    }
    printf("\n");
}

int calculate_next_row_position(int row){
    return (row+1) % 8;
}

int calculate_next_col_position(int col){
    return (col+1) % 7;
}

//this is the method for process 1
void *calculate_next_step(void *threadid) {
    //true=buffer_a
    //false=buffer_b
    bool which_buffer = true;
    while (true) {
        if (which_buffer) {
            //buffer a will be read from
            pthread_rwlock_rdlock(&lock_a);

            //x moves diagonally if it is moving
            int next_row_x, next_col_x;
            if(buffer_a[X].is_moving) {
                next_row_x = calculate_next_row_position(buffer_a[X].row);
                next_col_x = calculate_next_col_position(buffer_a[X].col);
            }
            else{
                next_row_x = buffer_a[X].row;
                next_col_x = buffer_a[X].col;
            }

            //y moves vertically if it is moving
            int next_row_y, next_col_y;
            if(buffer_a[Y].is_moving) {
                next_row_y = calculate_next_row_position(buffer_a[Y].row);
                next_col_y = 2;
            }
            else{
                next_row_y = buffer_a[Y].row;
                next_col_y = buffer_a[Y].col;
            }

            //z moves horizontally if it is moving
            int next_row_z, next_col_z;
            if(buffer_a[Z].is_moving) {
                next_row_z = 3;
                next_col_z = calculate_next_col_position(buffer_a[Z].col);
            }
            else{
                next_row_z = buffer_a[Z].row;
                next_col_z = buffer_a[Z].col;
            }

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
        }
        else {
            //buffer b will be read from
            pthread_rwlock_rdlock(&lock_b);

            //x moves diagonally
            int next_row_x = calculate_next_row_position(buffer_b[X].row);
            int next_col_x = calculate_next_col_position(buffer_b[X].col);

            //y moves vertically
            int next_row_y = calculate_next_row_position(buffer_b[Y].row);
			int next_col_y = 2;

            //z moves horizontally
			int next_row_z = 3;
            int next_col_z = calculate_next_col_position(buffer_b[Z].col);

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
            int is_moving_x = buffer_a[X].is_moving;

            int current_row_y = buffer_a[Y].row;
            int current_col_y = buffer_a[Y].col;
            int is_moving_y = buffer_a[Y].is_moving;

            int current_row_z = buffer_a[Z].row;
            int current_col_z = buffer_a[Z].col;
            int is_moving_z = buffer_a[Z].is_moving;

            pthread_rwlock_unlock(&lock_a);

            //buffer c will be updated to current position values
            pthread_rwlock_wrlock(&lock_c);

            //updates the appropriate buffer with current train positions and their movement status
            buffer_c.buffer[0][0] = current_row_x;
            buffer_c.buffer[0][1] = current_col_x;
            buffer_c.buffer[0][2] = is_moving_x;

            buffer_c.buffer[1][0] = current_row_y;
            buffer_c.buffer[1][1] = current_col_y;
            buffer_c.buffer[1][2] = is_moving_y;

            buffer_c.buffer[2][0] = current_row_z;
            buffer_c.buffer[2][1] = current_col_z;
            buffer_c.buffer[2][2] = is_moving_z;

            pthread_rwlock_unlock(&lock_c);
        }
        else{
            //buffer_b will be read from
            pthread_rwlock_rdlock(&lock_b);

            //stores the current position of each of the trains
            int current_row_x = buffer_b[X].row;
            int current_col_x = buffer_b[X].col;
            int is_moving_x = buffer_b[X].is_moving;

            int current_row_y = buffer_b[Y].row;
            int current_col_y = buffer_b[Y].col;
            int is_moving_y = buffer_b[Y].is_moving;

            int current_row_z = buffer_b[Z].row;
            int current_col_z = buffer_b[Z].col;
            int is_moving_z = buffer_b[Z].is_moving;

            pthread_rwlock_unlock(&lock_b);

            //buffer d will be updated to current position values
            pthread_rwlock_wrlock(&lock_d);

            //updates the appropriate buffer with current train positions. Stored as a character
            buffer_d.buffer[0][0] = current_row_x;
            buffer_d.buffer[0][1] = current_col_x;
            buffer_d.buffer[0][2] = is_moving_x;

            buffer_d.buffer[1][0] = current_row_y;
            buffer_d.buffer[1][1] = current_col_y;
            buffer_d.buffer[1][2] = is_moving_y;

            buffer_d.buffer[2][0] = current_row_z;
            buffer_d.buffer[2][1] = current_col_z;
            buffer_d.buffer[2][2] = is_moving_z;

            pthread_rwlock_unlock(&lock_d);
        }
        which_buffer = !which_buffer;
        //waits for each of the other threads to finish execution
        pthread_barrier_wait(&timing_barrier);
    }
}

//this function will initialize future_positions to have values filled from [1, future_seconds). This will allow the command center loop to being filling positions at index 0
void initialize_future_positions(position_buffer* current_position, position_buffer* future_positions, size_t future_seconds){
    for(size_t current_time = 1; current_time < future_seconds; current_time++){
        position_buffer* previous_second;
        if(current_time == 1)
            previous_second = current_position;
        else
            previous_second = &future_positions[current_time - 1];

        //calculate movement for x
        int row_x = previous_second->buffer[0][0];
        int col_x = previous_second->buffer[0][1];
        if(previous_second->buffer[0][2] == 1){
            row_x = calculate_next_row_position(row_x);
            col_x = calculate_next_col_position(col_x);
        }
        future_positions[current_time].buffer[0][0] = row_x;
        future_positions[current_time].buffer[0][1] = col_x;
        future_positions[current_time].buffer[0][2] = previous_second->buffer[0][2];

        //calculate movement for y
        int row_y = previous_second->buffer[1][0];
        int col_y = previous_second->buffer[1][1];
        if(previous_second->buffer[1][2] == 1)
            row_y = calculate_next_col_position(row_y);
        future_positions[current_time].buffer[1][0] = row_y;
        future_positions[current_time].buffer[1][1] = col_y;
        future_positions[current_time].buffer[1][2] = previous_second->buffer[1][2];

        //calculate movement for z
        int row_z = previous_second->buffer[2][0];
        int col_z = previous_second->buffer[2][1];
        if(previous_second->buffer[2][2] == 1)
            row_z = calculate_next_row_position(row_z);
        future_positions[current_time].buffer[2][0] = row_z;
        future_positions[current_time].buffer[2][1] = col_z;
        future_positions[current_time].buffer[2][2] = previous_second->buffer[2][2];
    }
}

int next_index(int time, size_t size){
    return time % (int)size;
}

int previous_index(int index, size_t size){
    int previous = (index-1) % (int)size;
    if(previous < 0){
        return (int)size-1;
    }
    return previous;
}

//function for the third process which will act as the central command center
void central_command_center(){
    //used for associating row number with train
    char train_index[] = {'X', 'Y', 'Z'};

    //odd time=buffer_c, even time=buffer_d
    int time = 1;

    //this is an array for storing the future positions of the trains. time % look_ahead_amount will give the current index of the future_positions buffer
    size_t look_ahead_amount = 2;
    position_buffer* future_positions = new position_buffer[look_ahead_amount];
    pthread_rwlock_rdlock(&lock_c);
    initialize_future_positions(&buffer_c, future_positions, look_ahead_amount);
    pthread_rwlock_unlock(&lock_c);

    //time_point start and finish are used to track how long P3 takes to run on each iteration
    high_resolution_clock::time_point start, finish;
    //P3 will be for collision detection
    while(true){
        bool collision = false;
        start = high_resolution_clock::now();
        //the current time will determine which buffer to read from
/*        if(time%2 != 0){
            pthread_rwlock_rdlock(&lock_c);
            //checks the buffer to see if any of the trains are at the same point
            for(int row = 0; row < 3; row++){
                for(int compare_row = row+1; compare_row < 3; compare_row++){
                    if(buffer_c.buffer[row][0] == buffer_c.buffer[compare_row][0] && buffer_c.buffer[row][1] == buffer_c.buffer[compare_row][1]){
                        collision = true;
                        printf("Collision between %c and %c at second %d, location (%d,%d).\n", train_index[row], train_index[compare_row], time, buffer_c.buffer[row][0], buffer_c.buffer[row][1]);
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
                    if(buffer_d.buffer[row][0] == buffer_d.buffer[compare_row][0] && buffer_d.buffer[row][1] == buffer_d.buffer[compare_row][1]){
                        collision = true;
                        printf("Collision between %c and %c at second %d, location (%d,%d).\n", train_index[row], train_index[compare_row], time, buffer_d.buffer[row][0], buffer_d.buffer[row][1]);
                        break;
                    }
                }
            }
            pthread_rwlock_unlock(&lock_d);
        }
        if(!collision){
            printf("No collision at time %d.\n", time);
        }*/
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
    buffer_a[X].is_moving = true;

    buffer_a[Y].buffer[0][2] = true;
    buffer_a[Y].row = 0;
    buffer_a[Y].col = 2;
    buffer_a[Y].is_moving = true;

    buffer_a[Z].buffer[3][6] = true;
    buffer_a[Z].row = 3;
    buffer_a[Z].col = 6;
    buffer_a[Z].is_moving = true;

    //creates the two additional threads
    pthread_create(&process1, nullptr, calculate_next_step, nullptr);
    pthread_create(&process2, nullptr, determine_current_positions, nullptr);

    //allows time for buffer c to be filled to be checked by P3
    sleep(1);
    pthread_barrier_wait(&timing_barrier);

    //main thread begins acting as the central command center
    central_command_center();

    pthread_join(process1, nullptr);
    pthread_join(process2, nullptr);
    return 0;
}