#include<mpi.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>

#define APEX_FUNC_TIMER_START(fn)                                              \
    apex_profiler_handle profiler0 = apex_start(APEX_FUNCTION_ADDRESS, &fn);
#define APEX_NAME_TIMER_START(num, name)                                       \
    apex_profiler_handle profiler##num = apex_start(APEX_NAME_STRING, name);
#define APEX_TIMER_STOP(num) apex_stop(profiler##num);

int main(int argc, char **argv)
{
    int rank, size;
    char hostname[MPI_MAX_PROCESSOR_NAME];
    char root_hostname[MPI_MAX_PROCESSOR_NAME];
    int len;
    int node_shared, node_shared_sum;
    int num_tgts = 0;
    int mpr;
    MPI_Aint win_size;
    int *rank_shared;
    int *send_buf, *recv_buf;
    MPI_Win win;
    MPI_Request *reqs;
    int i;
    long j;
    struct timespec start, end;
    int is_recv = 0;
    int is_send = 0;
    int send_count, recv_count;
    MPI_Comm xfer_comm;
    int xrank, xsize;
    int color;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);


    if(argc != 2) {
        if(!rank) {
            fprintf(stderr, "Usage: %s <MPR>\n", argv[0]);
        }
        exit(1);
    }
    mpr = atoi(argv[1]);
    MPI_Get_processor_name(hostname, &len);
    memcpy(root_hostname, hostname, len + 1);
    MPI_Bcast(root_hostname, len + 1, MPI_CHAR, 0, MPI_COMM_WORLD);
    node_shared = 0;
    if(strcmp(hostname, root_hostname) == 0) {
        node_shared = 1;
    }
    rank_shared = (int *)malloc(sizeof(*rank_shared) * size);
    MPI_Allgather(&node_shared, 1, MPI_INT, rank_shared, 1, MPI_INT, MPI_COMM_WORLD);
    color = 9;
    send_count = recv_count = 0;
    for(i = 0; i < size; i++) {
        if(i == rank) {
            if(node_shared) {
                if(recv_count < 9) {
                    is_recv = 1;
                    color = recv_count;
                }
            } else {
                is_send = 1;
                color = send_count / 14;
            }
        }
        if(rank_shared[i]) {
            recv_count++;
        } else {
            send_count++;
        }
    }

    if(is_recv) {
        fprintf(stderr, "rank %i on %s is receiving for color %i\n", rank, hostname, color);
    } else if(is_send) {
        fprintf(stderr, "rank %i on %s is sending for color %i\n", rank, hostname, color);
    } else {
        fprintf(stderr, "rank %i on %s is idle (default color %i)\n", rank, hostname, color);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Comm_split(MPI_COMM_WORLD, color, is_recv ? 0 : rank + 1, &xfer_comm);
    MPI_Comm_rank(xfer_comm, &xrank);
    MPI_Comm_size(xfer_comm, &xsize);

    win_size = mpr * 1024 * 1024;
    //MPI_Reduce(&node_shared, &node_shared_sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    if(!rank) {
        node_shared_sum = size;
        for(i = 0; i < size; i++) {
            node_shared_sum -= rank_shared[i];
        }
        if(node_shared_sum * 4 != 3 * size) { 
            fprintf(stderr, "ranks not distributed correctly. Found %i on-node ranks, but size is %i.\n", node_shared_sum, size);
            exit(1);
        }
        fprintf(stderr, "found %i off-node ranks.\n", node_shared_sum);
    }
    
    if(!rank) {
        printf("Window size is %li bytes.\n", win_size);  
        //reqs = (MPI_Request *)malloc(sizeof(*reqs) * node_shared_sum);
    }

    if(is_send) {
        send_buf = malloc(win_size * sizeof(int));
        memset(send_buf, xrank, 4 * win_size);
        fprintf(stderr, "sending %li bytes to %i\n", 4 * win_size, xrank); 
        MPI_Win_create(send_buf, win_size*sizeof(int), 1, MPI_INFO_NULL, xfer_comm, &win); 
        MPI_Win_fence(0, win);
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Win_fence(0, win); 
        MPI_Barrier(MPI_COMM_WORLD);
    } else if(is_recv) {
        fprintf(stderr, "rank %i receiving from %i\n", rank, xsize-1);
        recv_buf = malloc(14 * win_size * sizeof(int));
        MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, xfer_comm, &win); 
 
        MPI_Win_fence(0, win); 
        
        MPI_Barrier(MPI_COMM_WORLD);
        timespec_get(&start, TIME_UTC); 
        //APEX_NAME_TIMER_START(1, "get_time");
        for(i = 1; i < xsize; i++) {
            MPI_Get(&recv_buf[win_size * (i-1)], 4 * win_size, MPI_BYTE, i, 0, 4 * win_size, MPI_BYTE, win);
            //fprintf(stderr, "get %i into %p\n", i, (void *)&recv_buf[win_size * (i-1)]);
        }
        //MPI_Waitall(node_shared_sum, reqs, MPI_STATUS_IGNORE);
        //APEX_TIMER_STOP(1);
        MPI_Win_fence(0, win);
        MPI_Barrier(MPI_COMM_WORLD);
        timespec_get(&end, TIME_UTC);
        fprintf(stderr, "did MPI_Get %i times\n", xsize-1);
        float xfer_time = (float)(end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;

        for(int i = 0; i < xsize - 1; i++) {
            for(j = 0; j < 4 * win_size; j++) {
                if(((char *)recv_buf)[i * 4 * win_size + j] != i+1) {
                        fprintf(stderr, "recv check failed. Rank %i, byte %li was %i\n",
                                    i+1, j, ((char *)recv_buf)[i * 4 * win_size + j]);
                        exit(1);    
                 }
            }
        }
        free(recv_buf);
        fprintf(stderr, "rank %i recv elapsed %f sec\n", rank, xfer_time);
    } else {
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if(is_recv || is_send)
        MPI_Win_free(&win);

    MPI_Finalize();
}
