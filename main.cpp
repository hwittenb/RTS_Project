#include "pthread.h"
#include "unistd.h"
#include "stdio.h"
#include <boost/thread/shared_mutex.hpp>

//compile with g++ main.cpp -lpthread -lboost_system -lboost_thread

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

boost::shared_mutex mutex_a;
boost::shared_mutex mutex_b;
boost::shared_mutex mutex_c;
boost::shared_mutex mutex_d;

//this is the method for process 1
void *calculate_next_step(void *threadid) {
    //true=buffer_a
    //false=buffer_b
    bool which_buffer = true;
    while (true) {
        if (which_buffer) {
            //buffer a will be read from
            mutex_a.lock_shared();
            //x moves diagonally
            int next_row_x = (buffer_a[X].row + 1) % 8;
            int next_col_x = (buffer_a[X].col + 1) % 7;

            //y moves vertically
            int next_row_y = (buffer_a[Y].row + 1) % 8;
			int next_col_y = 2;

            //z moves horizontally
			int next_row_z = 3;
            int next_col_z = (buffer_a[Z].col + 1) % 7;
            mutex_a.unlock_shared();

            //buffer b will be written to
            mutex_b.lock();
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
            mutex_b.unlock();
        } else {
            //buffer b will be read from
            mutex_b.lock_shared();
            //x moves diagonally
            int next_row_x = (buffer_b[X].row + 1) % 8;
            int next_col_x = (buffer_b[X].col + 1) % 7;

            //y moves vertically
            int next_row_y = (buffer_b[Y].row + 1) % 8;
			int next_col_y = 2;

            //z moves horizontally
			int next_row_z = 3;
            int next_col_z = (buffer_b[Z].col + 1) % 7;
            mutex_b.unlock_shared();

            //buffer a will be written to
            mutex_a.lock();
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
            mutex_a.unlock();
        }
        which_buffer = !which_buffer;
        sleep(1);
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
            mutex_a.lock_shared();
            int current_row_x = buffer_a[X].row;
            int current_col_x = buffer_a[X].col;

            int current_row_y = buffer_a[Y].row;
            int current_col_y = buffer_a[Y].col;

            int current_row_z = buffer_a[Z].row;
            int current_col_z = buffer_a[Z].col;
            mutex_a.unlock_shared();

            //buffer c will be updated to current position values
            mutex_c.lock();
            buffer_c[0][1] = '0' + current_row_x;
            buffer_c[0][2] = '0' + current_col_x;

            buffer_c[1][1] = '0' + current_row_y;
            buffer_c[1][2] = '0' + current_col_y;

            buffer_c[2][1] = '0' + current_row_z;
            buffer_c[2][2] = '0' + current_col_z;
            mutex_c.unlock();
        }
        else{
            //buffer_b will be read from
            mutex_b.lock_shared();
            int current_row_x = buffer_b[X].row;
            int current_col_x = buffer_b[X].col;

            int current_row_y = buffer_b[Y].row;
            int current_col_y = buffer_b[Y].col;

            int current_row_z = buffer_b[Z].row;
            int current_col_z = buffer_b[Z].col;
            mutex_b.unlock_shared();

            //buffer d will be updated to current position values
            mutex_d.lock();
            buffer_d[0][1] = '0' + current_row_x;
            buffer_d[0][2] = '0' + current_col_x;

            buffer_d[1][1] = '0' + current_row_y;
            buffer_d[1][2] = '0' + current_col_y;

            buffer_d[2][1] = '0' + current_row_z;
            buffer_d[2][2] = '0' + current_col_z;
            mutex_d.unlock();
        }
        which_buffer = !which_buffer;
        sleep(1);
    }
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

    buffer_c[0][0] = 'X';
    buffer_c[1][0] = 'Y';
    buffer_c[2][0] = 'Z';

    buffer_d[0][0] = 'X';
    buffer_d[1][0] = 'Y';
    buffer_d[2][0] = 'Z';

    pthread_create(&process1, NULL, calculate_next_step, (void*)1);
    pthread_create(&process2, NULL, determine_current_positions, (void*)2);

    //allows time for buffer c to be filled to be checked by P3
    sleep(1);

    //true=buffer_c
    //false=buffer_d
	char train_name[] = {'X', 'Y', 'Z'};
	int time = 1;
    while(true){
		bool collision = false;
		if(time%2 != 0){
			mutex_c.lock_shared();
			for(int row = 0; row < 3; row++){
				for(int compare_row = row+1; compare_row < 3; compare_row++){
					if(buffer_c[row][1] == buffer_c[compare_row][1] && buffer_c[row][2] == buffer_c[compare_row][2]){
						collision = true;
						printf("Collision between %c and %c at second %d, location (%c,%c).\n", train_name[row], train_name[compare_row], time, buffer_c[row][1], buffer_c[row][2]);
						break;
					}
				}
			}
			mutex_c.unlock_shared();
		}
		else{
			mutex_d.lock_shared();
			for(int row = 0; row < 3; row++){
				for(int compare_row = row+1; compare_row < 3; compare_row++){
					if(buffer_d[row][1] == buffer_d[compare_row][1] && buffer_d[row][2] == buffer_d[compare_row][2]){
						collision = true;
						printf("Collision between %c and %c at second %d, location (%c,%c).\n", train_name[row], train_name[compare_row], time, buffer_d[row][1], buffer_d[row][2]);
						break;
					}
				}
			}
			mutex_d.unlock_shared();
		}
		if(!collision){
			printf("No collision at time %d.\n", time);
		}
		time++;
		sleep(1);
    }

    pthread_join(process1, NULL);
    pthread_join(process2, NULL);
    return 0;
}