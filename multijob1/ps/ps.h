#ifndef PS_H
#define PS_H

// #include <iostream>
// #include <ctime>
// #include <cmath>
// #include <random>
// #include <arpa/inet.h>
// #include <chrono>
// #include <map>

// #include <stdio.h>
// #include <stdbool.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <signal.h>
// #include <time.h>
// #include <memory.h>
// #include <zlib.h>
// #include <sys/queue.h>



#define MAX_TENSOR_SIZE 100000000 * 10
#define MAX_APP_PER_THREAD 10
#define MAX_STORAGE_PER_APP_PER_THREAD 1
#define MAX_WORKER 16
#define OVERFLOW_HANDLE false

#define MAX_ENTRIES_PER_PACKET 64

struct data_t {
    int32_t *data_int;
    float *data_float;
};

struct window_manager
{
	bool *isACKed;
	// bool *isSent;
	uint32_t total_ACK;
	uint32_t last_ACK;
};


struct tensor_context {
    bool* isOccupy;
    bool* isCollision;
    bool* isFloat;
    bool isCompleted;
    struct data_t data;
    uint32_t len;
    uint64_t key;
    uint8_t num_worker;
    struct window_manager* window_manager_arr;
    // struct timespec start_time;
};

void inline init_tensor(struct tensor_context* tensor, uint32_t len) {
    tensor->data.data_int = (int32_t*) malloc (sizeof(int32_t) * len);
    tensor->data.data_float = (float*) malloc (sizeof(float) * len);
    tensor->isCompleted = true;
    tensor->isOccupy = (bool*)malloc(sizeof(bool) * (MAX_TENSOR_SIZE / MAX_ENTRIES_PER_PACKET + 1));
    tensor->isCollision = (bool*)malloc(sizeof(bool) * (MAX_TENSOR_SIZE / MAX_ENTRIES_PER_PACKET + 1));
    tensor->isFloat = (bool*)malloc(sizeof(bool) * (MAX_TENSOR_SIZE / MAX_ENTRIES_PER_PACKET + 1));
    tensor->len = 0;
    tensor->num_worker = 0;
    tensor->key = 0xffffffffffffffff;
    tensor->window_manager_arr = malloc(MAX_WORKER * sizeof(struct window_manager));
    for (int i = 0; i < MAX_WORKER; i++) {
        tensor->window_manager_arr[i].isACKed = calloc((MAX_TENSOR_SIZE / MAX_ENTRIES_PER_PACKET + 1), sizeof(bool));
        tensor->window_manager_arr[i].total_ACK = MAX_TENSOR_SIZE / MAX_ENTRIES_PER_PACKET + 1;
        tensor->window_manager_arr[i].last_ACK = 0;
    }
    
}

#endif
