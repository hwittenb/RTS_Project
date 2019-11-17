#include "pthread.h"
#include "unistd.h"
#include <cstdio>
#include <chrono>
#include <set>
#include <semaphore.h>
#include <stdlib.h>
#include <functional>

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

struct train_information{
    char train_char;
    int failure_rate;
    function<int(int)> col_movement_function;
    function<int(int)> row_movement_function;
};

//enum created for readability
enum train{
    X, Y, Z
};

//for storing the failure rates of each of the trains in order of X, Y, and Z
const int train_failure_rate[] = {50, 10, 1};

//used for associating row number with train
const char train_index[] = {'X', 'Y', 'Z'};

grid_buffer buffer_a[3];
grid_buffer buffer_b[3];

//each row is for each of the three trains. The first 2 columns are for row and column position of that train. The last column is 1 if the train is moving, and 0 if the train is stopped
position_buffer buffer_c;
position_buffer buffer_d;

train_information trains[3];

pthread_rwlock_t lock_a;
pthread_rwlock_t lock_b;
pthread_rwlock_t lock_c;
pthread_rwlock_t lock_d;

sem_t buffer_updated_sem;

pthread_barrier_t timing_barrier;

int north_move(int row){
    if(row == 0)
        return 7;
    return row-1;
}

int south_move(int row){
    return (row+1) % 8;
}

int east_move(int col){
    return (col+1) % 7;
}

int west_move(int col){
    if(col == 0)
        return 6;
    return col - 1;
}

int no_move(int pos){
    return pos;
}

string initialize_train_movement(function<int(int)>& x_func, function<int(int)>& y_func){
    int selected_movement = rand() % 8;
    switch(selected_movement){
        case 0:
            x_func = north_move;
            y_func = no_move;
            return "north";
        case 1:
            x_func = north_move;
            y_func = east_move;
            return "north-east";
        case 2:
            x_func = no_move;
            y_func = east_move;
            return "east";
        case 3:
            x_func = south_move;
            y_func = east_move;
            return "south-east";
        case 4:
            x_func = south_move;
            y_func = no_move;
            return "south";
        case 5:
            x_func = south_move;
            y_func = west_move;
            return "south-west";
        case 6:
            x_func = no_move;
            y_func = west_move;
            return "west";
        default:
            x_func = north_move;
            y_func = west_move;
            return "north-west";
    }
}

//this is the method for process 1
void *calculate_next_step(void *threadid) {
    //true=buffer_a
    //false=buffer_b
    bool which_buffer = true;
    while (true) {
        sem_wait(&buffer_updated_sem);

        //determines the current buffer (either A or B) and the next buffer (either A or B) based off the bool which_buffer
        grid_buffer* current_grid_buffer;
        pthread_rwlock_t* current_grid_lock;
        grid_buffer* next_grid_buffer;
        pthread_rwlock_t* next_grid_lock;
        if(which_buffer){
            current_grid_buffer = buffer_a;
            current_grid_lock = &lock_a;
            next_grid_buffer = buffer_b;
            next_grid_lock = &lock_b;
        }
        else{
            current_grid_buffer = buffer_b;
            current_grid_lock = &lock_b;
            next_grid_buffer = buffer_a;
            next_grid_lock = &lock_a;
        }

        //current buffer will be read from
        pthread_rwlock_rdlock(current_grid_lock);

        //x moves
        int next_row_x, next_col_x;
        if(current_grid_buffer[X].is_moving) {
            next_row_x = trains[X].row_movement_function(current_grid_buffer[X].row);
            next_col_x = trains[X].col_movement_function(current_grid_buffer[X].col);
        }
        else{
            next_row_x = current_grid_buffer[X].row;
            next_col_x = current_grid_buffer[X].col;
        }
        //y moves
        int next_row_y, next_col_y;
        if(current_grid_buffer[Y].is_moving) {
            next_row_y = trains[Y].row_movement_function(current_grid_buffer[Y].row);
            next_col_y = trains[Y].col_movement_function(current_grid_buffer[Y].col);
        }
        else{
            next_row_y = current_grid_buffer[Y].row;
            next_col_y = current_grid_buffer[Y].col;
        }
        //z moves with a velocity of 2
        int next_row_z, next_col_z;
        if(current_grid_buffer[Z].is_moving) {
            next_row_z = trains[Z].row_movement_function(current_grid_buffer[Z].row);
            next_row_z = trains[Z].row_movement_function(next_row_z);
            next_col_z = trains[Z].col_movement_function(current_grid_buffer[Z].col);
            next_col_z = trains[Z].col_movement_function(next_col_z);
        }
        else{
            next_row_z = current_grid_buffer[Z].row;
            next_col_z = current_grid_buffer[Z].col;
        }

        pthread_rwlock_unlock(current_grid_lock);

        //next buffer will be written to
        pthread_rwlock_wrlock(next_grid_lock);

        //saves the next position of train X in buffer b
        next_grid_buffer[X].buffer[next_grid_buffer[X].row][next_grid_buffer[X].col] = false;
        next_grid_buffer[X].buffer[next_row_x][next_col_x] = true;
        next_grid_buffer[X].row = next_row_x;
        next_grid_buffer[X].col = next_col_x;
        //saves the next position of train Y in buffer b
        next_grid_buffer[Y].buffer[next_grid_buffer[Y].row][next_grid_buffer[Y].col] = false;
        next_grid_buffer[Y].buffer[next_row_y][next_col_y] = true;
        next_grid_buffer[Y].row = next_row_y;
        next_grid_buffer[Y].col = next_col_y;
        //saves the next position of train Z in buffer b
        next_grid_buffer[Z].buffer[next_grid_buffer[Z].row][next_grid_buffer[Z].col] = false;
        next_grid_buffer[Z].buffer[next_row_z][next_col_z] = true;
        next_grid_buffer[Z].row = next_row_z;
        next_grid_buffer[Z].col = next_col_z;

        pthread_rwlock_unlock(next_grid_lock);

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
        sem_wait(&buffer_updated_sem);

        grid_buffer* current_grid_buffer;
        pthread_rwlock_t* current_grid_lock;
        position_buffer* current_position_buffer;
        pthread_rwlock_t* current_position_lock;
        if(which_buffer){
            current_grid_buffer = buffer_a;
            current_grid_lock = &lock_a;
            current_position_buffer = &buffer_c;
            current_position_lock = &lock_c;
        }
        else{
            current_grid_buffer = buffer_b;
            current_grid_lock = &lock_b;
            current_position_buffer = &buffer_d;
            current_position_lock = &lock_d;
        }

        //current_grid_buffer will be read from
        pthread_rwlock_rdlock(current_grid_lock);

        //stores the current position of each of the trains
        int current_row_x = current_grid_buffer[X].row;
        int current_col_x = current_grid_buffer[X].col;
        int is_moving_x = current_grid_buffer[X].is_moving;

        int current_row_y = current_grid_buffer[Y].row;
        int current_col_y = current_grid_buffer[Y].col;
        int is_moving_y = current_grid_buffer[Y].is_moving;

        int current_row_z = current_grid_buffer[Z].row;
        int current_col_z = current_grid_buffer[Z].col;
        int is_moving_z = current_grid_buffer[Z].is_moving;

        pthread_rwlock_unlock(current_grid_lock);

        //buffer c will be updated to current position values
        pthread_rwlock_wrlock(current_position_lock);

        //updates the appropriate buffer with current train positions and their movement status
        current_position_buffer->buffer[0][0] = current_row_x;
        current_position_buffer->buffer[0][1] = current_col_x;
        current_position_buffer->buffer[0][2] = is_moving_x;

        current_position_buffer->buffer[1][0] = current_row_y;
        current_position_buffer->buffer[1][1] = current_col_y;
        current_position_buffer->buffer[1][2] = is_moving_y;

        current_position_buffer->buffer[2][0] = current_row_z;
        current_position_buffer->buffer[2][1] = current_col_z;
        current_position_buffer->buffer[2][2] = is_moving_z;

        pthread_rwlock_unlock(current_position_lock);


        which_buffer = !which_buffer;
        //waits for each of the other threads to finish execution
        pthread_barrier_wait(&timing_barrier);
    }
}

void calculate_next_second(position_buffer* previous_second, position_buffer* current_second){
    //calculate movement for x
    int row_x = previous_second->buffer[0][0];
    int col_x = previous_second->buffer[0][1];
    if(previous_second->buffer[0][2] == 1){
        row_x = trains[X].row_movement_function(row_x);
        col_x = trains[X].col_movement_function(col_x);
    }
    current_second->buffer[0][0] = row_x;
    current_second->buffer[0][1] = col_x;
    current_second->buffer[0][2] = previous_second->buffer[0][2];

    //calculate movement for y
    int row_y = previous_second->buffer[1][0];
    int col_y = previous_second->buffer[1][1];
    if(previous_second->buffer[1][2] == 1) {
        row_y = trains[Y].row_movement_function(row_y);
        col_y = trains[Y].col_movement_function(col_y);
    }
    current_second->buffer[1][0] = row_y;
    current_second->buffer[1][1] = col_y;
    current_second->buffer[1][2] = previous_second->buffer[1][2];

    //calculate movement for z
    int row_z = previous_second->buffer[2][0];
    int col_z = previous_second->buffer[2][1];
    //z moves with a velocity of 2
    if(previous_second->buffer[2][2] == 1) {
        row_z = trains[Z].row_movement_function(row_z);
        row_z = trains[Z].row_movement_function(row_z);
        col_z = trains[Z].col_movement_function(col_z);
        col_z = trains[Z].col_movement_function(col_z);
    }
    current_second->buffer[2][0] = row_z;
    current_second->buffer[2][1] = col_z;
    current_second->buffer[2][2] = previous_second->buffer[2][2];
}

//this function initializes all positions of future_positions other than the position that will be written to on time time_zero. This allows the command center loop to fill indices starting at get_next_index(time_zero, future_seconds)
void initialize_future_positions(position_buffer* current_position, position_buffer* future_positions, int future_seconds){
    for(int current_time = 0; current_time < future_seconds; current_time++){
        position_buffer* previous_second;
        if(current_time == 0)
            previous_second = current_position;
        else
            previous_second = &future_positions[current_time-1];

        calculate_next_second(previous_second, &future_positions[current_time]);
    }
}

void get_trains_that_collide(position_buffer* position, set<int>* colliding_trains){
    for(int row = 0; row < 3; row++){
        for(int compare_row = 0; compare_row < 3; compare_row++){
            if(row != compare_row && position->buffer[row][0] == position->buffer[compare_row][0] && position->buffer[row][1] == position->buffer[compare_row][1]){
                colliding_trains->insert(row);
            }
        }
    }
}

//checks if there is a collision involving the train train_num returns either train_num or -1 if collision has not occured
int is_collision(position_buffer* position, int train_num){
    for(int compare_row = 0; compare_row < 3; compare_row++){
        if(compare_row != train_num && position->buffer[train_num][0] == position->buffer[compare_row][0] && position->buffer[train_num][1] == position->buffer[compare_row][1]){
            return compare_row;
        }
    }
    return -1;
}

//returns true if the train has been freed. Otherwise, returns false
bool free_train(position_buffer* current_positions, int train_num, int look_ahead_seconds, int time){
    //will store the future positions of the trains if the train train_num begins moving
    position_buffer test_buffer[look_ahead_seconds];
    //sets the train train_num to move
    current_positions->buffer[train_num][2] = 1;
    for(int i = 0; i < look_ahead_seconds; i++){
        if(i == 0)
            calculate_next_second(current_positions, &test_buffer[i]);
        else
            calculate_next_second(&test_buffer[i-1], &test_buffer[i]);
        //if this path has a collision involving train_num, this train cannot be freed
        if(is_collision(&test_buffer[i], train_num) != -1){
            //if the trains collide, the train train_num cannot start moving
            current_positions->buffer[train_num][2] = 0;
            return false;
        }
    }
    printf("Train %c has begun movement again at time %d.\n", train_index[train_num], time);
    return true;
}

//this method will update the current_positions buffer to resume movement of trains that no longer need to be stopped. Returns true if current_positions is modified.
bool update_stopped_trains(position_buffer* current_positions, int look_ahead_seconds, int time){
    //determines if any trains are currently stopped. If they are not, the current positions are returned
    for(int train = 0; train < 3; train++){
        if(current_positions->buffer[train][2] == 0){
            if(free_train(current_positions, train, look_ahead_seconds, time)){
                return true;
            }
        }
    }
    return false;
}

bool train_interfere(position_buffer* future_positions, int look_ahead_seconds, int train_num, int stopped_row, int stopped_col){
    for(int i = 0; i < look_ahead_seconds; i++){
        for(int train = 0; train < 3; train++){
            if(train != train_num && future_positions[i].buffer[train][0] == stopped_row && future_positions[i].buffer[train][1] == stopped_col){
                return true;
            }
        }
    }
    return false;
}

//finds the train with the highest chance of stopping and removes it from the collision set
int find_train_to_stop(set<int>& collisions){
    int min = *collisions.begin();
    for(auto it = ++collisions.begin(); it != collisions.end(); it++){
        if(train_failure_rate[*it] < train_failure_rate[min])
            min = *it;
    }
    collisions.erase(min);
    return min;
}

//returns true if the train has successfully stopped
bool try_stop(int train_num){
    int failure_rate = train_failure_rate[train_num];
    int random = rand() % 100 + 1;
    return random > failure_rate;
}

//function for the third process which will act as the central command center
void central_command_center(){
    //odd time=buffer_c, even time=buffer_d
    int time = 1;

    //this is an array for storing the future positions of the trains. time % look_ahead_amount will give the current index of the future_positions buffer
    int look_ahead_amount = 3;

    //time_point start and finish are used to track how long P3 takes to run on each iteration
    high_resolution_clock::time_point start, finish;
    //P3 will be for collision detection
    while(true){
        start = high_resolution_clock::now();

        //determines what the current buffer and lock are
        position_buffer* current_position_buffer;
        pthread_rwlock_t* current_position_lock;
        position_buffer* next_position_buffer;
        pthread_rwlock_t* next_position_lock;
        //the current grid is updated when it is determined if any of the trains need to stop before movement is continued
        grid_buffer* current_grid;
        pthread_rwlock_t* grid_lock;
        if(time%2 != 0){
            current_position_buffer = &buffer_c;
            current_position_lock = &lock_c;
            next_position_buffer = &buffer_d;
            next_position_lock = &lock_d;
            current_grid = buffer_b;
            grid_lock = &lock_b;
        }
        else{
            current_position_buffer = &buffer_d;
            current_position_lock = &lock_d;
            next_position_buffer = &buffer_c;
            next_position_lock = &lock_c;
            current_grid = buffer_a;
            grid_lock = &lock_a;
        }

        //checks if an unavoidable collision has occurred
        pthread_rwlock_rdlock(current_position_lock);
        bool collision_occured = false;
        int collision_train_one;
        int collision_train_two = -1;
        for(collision_train_one = 0; collision_train_one < 3; collision_train_one++){
            collision_train_two = is_collision(current_position_buffer, collision_train_one);
            if(collision_train_two != -1){
                collision_occured = true;
                break;
            }
        }
        pthread_rwlock_unlock(current_position_lock);
        if(collision_occured)
            printf("An unavoidable collision has occured between trains %c and %c at time %d.\n", train_index[collision_train_one], train_index[collision_train_two], time);

        //checks if any of the trains currently stopped should be freed
        pthread_rwlock_wrlock(current_position_lock);
        update_stopped_trains(current_position_buffer, look_ahead_amount, time);
        pthread_rwlock_unlock(current_position_lock);

        //checks where the trains will be in the future
        position_buffer* future_positions = new position_buffer[look_ahead_amount];
        pthread_rwlock_rdlock(current_position_lock);
        initialize_future_positions(current_position_buffer, future_positions, look_ahead_amount);
        pthread_rwlock_unlock(current_position_lock);

        set<int> collisions;
        for(int collision_check_time = 0; collision_check_time < look_ahead_amount; collision_check_time++) {
            //get_trains_that_collide(&future_positions[current_index], &collisions);
            get_trains_that_collide(&future_positions[collision_check_time], &collisions);
            if (collisions.size() == 3) {
                pthread_rwlock_wrlock(current_position_lock);
                int train_one = find_train_to_stop(collisions);
                int train_two = find_train_to_stop(collisions);
                int stopped_trains = 0;

                if(try_stop(train_one)){
                    current_position_buffer->buffer[train_one][2] = 0;
                    stopped_trains++;
                }
                else{
                    printf("Train %c failed to stop at time %d.\n", train_index[train_one], time);
                }

                if(try_stop(train_two)){
                    current_position_buffer->buffer[train_two][2] = 0;
                    stopped_trains++;
                }
                else{
                    printf("Train %c failed to stop at time %d.\n", train_index[train_two], time);
                }

                if(stopped_trains == 2){
                    printf("The trains X, Y, and Z were set to collide, so trains %c and %c have stopped at time %d.\n", train_index[train_one], train_index[train_two], time);
                }
                else{
                    int train_three = find_train_to_stop(collisions);
                    if(try_stop(train_three)){
                        current_position_buffer->buffer[train_three][2] = 0;
                        printf("Because %d/2 trains successfully stopped, train %c was additionally stopped at time %d", stopped_trains, train_index[train_three], time);
                    }
                    else{
                        printf("Train %c failed to stop at time %d.\n", train_index[train_three], time);
                    }
                }

                pthread_rwlock_unlock(current_position_lock);
                break;
            }
            else if (collisions.size() == 2) {
                int train_one = find_train_to_stop(collisions);
                int train_two = find_train_to_stop(collisions);
                int train_to_stop = train_one;
                bool is_train_stopped = false;
                pthread_rwlock_wrlock(current_position_lock);
                //output if there was interference?
                bool interference = train_interfere(future_positions, look_ahead_amount, train_one, current_position_buffer->buffer[train_one][0], current_position_buffer->buffer[train_one][1]);
                if(!interference){
                    if(try_stop(train_to_stop)){
                        current_position_buffer->buffer[train_to_stop][2] = 0;
                        is_train_stopped = true;
                    }
                    else{
                        printf("Train %c failed to stop at time %d.\n", train_index[train_to_stop], time);
                    }
                }

                if(!is_train_stopped){
                    train_to_stop = train_two;
                    if(try_stop(train_to_stop)){
                        current_position_buffer->buffer[train_to_stop][2] = 0;
                        is_train_stopped = true;
                    }
                    else{
                        printf("Train %c failed to stop at time %d.\n", train_index[train_to_stop], time);
                    }
                }
                pthread_rwlock_unlock(current_position_lock);
                if(is_train_stopped)
                    printf("The trains %c and %c were set to collide, so train %c was stopped at time %d.\n", train_index[train_one], train_index[train_two], train_index[train_to_stop], time);
                else
                    printf("The trains %c and %c were set to collide, and both trains failed to stop at time %d\n", train_index[train_one], train_index[train_two], time);
                break;
            }
        }

        int num_trains_moving = 0;

        //update either buffer a or b with new is_moving values
        pthread_rwlock_wrlock(grid_lock);
        pthread_rwlock_wrlock(next_position_lock);
        pthread_rwlock_rdlock(current_position_lock);

        //this will be updating the buffer that is currently being used to calculate the positions of the next time. That way the trains will be able to stop before a collision.
        for(int i = 0; i < 3; i++){
            num_trains_moving += current_position_buffer->buffer[i][2];
            current_grid[i].is_moving = current_position_buffer->buffer[i][2] == 1;
            next_position_buffer->buffer[i][2] = current_position_buffer->buffer[i][2];
        }

        printf("Time %d: %d trains are moving. Train X (%d, %d), train Y (%d, %d), train Z (%d, %d).\n", time, num_trains_moving,
               current_position_buffer->buffer[0][0], current_position_buffer->buffer[0][1],
               current_position_buffer->buffer[1][0], current_position_buffer->buffer[1][1],
               current_position_buffer->buffer[2][0], current_position_buffer->buffer[2][1]);

        pthread_rwlock_unlock(current_position_lock);
        pthread_rwlock_unlock(next_position_lock);
        pthread_rwlock_unlock(grid_lock);

        sem_post(&buffer_updated_sem);
        sem_post(&buffer_updated_sem);

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

    srand(time(NULL));

    pthread_rwlock_init(&lock_a, nullptr);
    pthread_rwlock_init(&lock_b, nullptr);
    pthread_rwlock_init(&lock_c, nullptr);
    pthread_rwlock_init(&lock_d, nullptr);

    sem_init(&buffer_updated_sem, 3, 2);

    pthread_barrier_init(&timing_barrier, nullptr, 3);

    //initializes buffers for starting values
    buffer_a[X].buffer[0][0] = true;
    buffer_a[X].row = rand() % 8;
    buffer_a[X].col = rand() % 7;
    buffer_a[X].is_moving = true;

    buffer_a[Y].buffer[0][2] = true;
    buffer_a[Y].row = rand() % 8;
    buffer_a[Y].col = rand() % 7;
    buffer_a[Y].is_moving = true;

    buffer_a[Z].buffer[3][6] = true;
    buffer_a[Z].row = rand() % 8;
    buffer_a[Z].col = rand() % 7;
    buffer_a[Z].is_moving = true;

    //initializes train information. this is readonly after initialization, so no protection is required
    trains[X].train_char = 'X';
    trains[X].failure_rate = 50;
    string x_direction = initialize_train_movement(trains[X].col_movement_function, trains[X].row_movement_function);
    printf("X is starting at (%d,%d) with initial velocity %s.\n", buffer_a[X].row, buffer_a[X].col, x_direction.c_str());

    trains[Y].train_char = 'Y';
    trains[Y].failure_rate = 10;
    string y_direction = initialize_train_movement(trains[Y].col_movement_function, trains[Y].row_movement_function);
    printf("Y is starting at (%d,%d) with initial velocity %s.\n", buffer_a[Y].row, buffer_a[Y].col, y_direction.c_str());

    trains[Z].train_char = 'Z';
    trains[Z].failure_rate = 1;
    string z_direction = initialize_train_movement(trains[Z].col_movement_function, trains[Z].row_movement_function);
    printf("Z is starting at (%d,%d) with initial velocity %s.\n\n", buffer_a[Z].row, buffer_a[Z].col, z_direction.c_str());

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