#include <stdio.h>
#include <thread>
#include <queue>

#include "CycleTimer.h"

typedef struct 
{
    int startRow;
    int numberRows;
} Pair;

typedef struct
{
    float x0, x1;
    float y0, y1;
    unsigned int width;
    unsigned int height;
    int maxIterations;
    int *output;
    int threadId;
    int numThreads;
    int startRow;
    int totalRows;
    std::queue<Pair> pairs;
} WorkerArgs;

extern void mandelbrotSerial(
    float x0, float y0, float x1, float y1,
    int width, int height,
    int startRow, int numRows,
    int maxIterations,
    int output[]);

//
// workerThreadStart --
//
// Thread entrypoint.
void workerThreadStart(WorkerArgs *const args)
{

    // TODO FOR CS149 STUDENTS: Implement the body of the worker
    // thread here. Each thread should make a call to mandelbrotSerial()
    // to compute a part of the output image.  For example, in a
    // program that uses two threads, thread 0 could compute the top
    // half of the image and thread 1 could compute the bottom half.
    int i = 0;
    while (!args->pairs.empty()) {
        double startTime = CycleTimer::currentSeconds();
        mandelbrotSerial(args->x0, args->y0, args->x1, args->y1, args->width, args->height, args->pairs.front().startRow, args->pairs.front().numberRows, args->maxIterations, args->output);
        double endTime = CycleTimer::currentSeconds();
        printf("Thread %d - partition %d: \t\t[%.3f] ms\n",  args->threadId, i, (endTime - startTime) * 1000);
        args->pairs.pop();
        i = i + 1;
    }
}

//
// MandelbrotThread --
//
// Multi-threaded implementation of mandelbrot set image generation.
// Threads of execution are created by spawning std::threads.
void mandelbrotThread(
    int numThreads,
    float x0, float y0, float x1, float y1,
    int width, int height,
    int maxIterations, int output[])
{
    static constexpr int MAX_THREADS = 32;
    static int numPartitions = 4;
    int numPairs = numThreads*numPartitions;
    //Pair *p = new Pair[];
    Pair *pairs = (Pair *)malloc(numPairs * sizeof(Pair));

    if (numThreads > MAX_THREADS)
    {
        fprintf(stderr, "Error: Max allowed threads is %d\n", MAX_THREADS);
        exit(1);
    }

    // Creates thread objects that do not yet represent a thread.
    std::thread workers[MAX_THREADS];
    WorkerArgs args[MAX_THREADS];

    int numberRowsPerTask = height/(numPairs);
    int total_rows_covered = 0;
    for (int p = 0; p < numThreads*numPartitions; ++p) {
        pairs[p].startRow = p*numberRowsPerTask;
        pairs[p].numberRows = numberRowsPerTask;
        printf("Pair %d: (%d, %d)\n", p, pairs[p].startRow, pairs[p].numberRows);
        total_rows_covered += numberRowsPerTask;
    }
    // For odd numThreads*numPartitions, assign remaining fraction to the last pair. 
    if (total_rows_covered < height) {
        pairs[numThreads*numPartitions - 1].numberRows += (height - total_rows_covered);
    }
    
    for (int i = 0; i < numThreads; i++)
    {
        // TODO FOR CS149 STUDENTS: You may or may not wish to modify
        // the per-thread arguments here.  The code below copies the
        // same arguments for each thread
        args[i].x0 = x0;
        args[i].y0 = y0;
        args[i].x1 = x1;
        args[i].y1 = y1;
        args[i].width = width;
        args[i].height = height;
        args[i].maxIterations = maxIterations;
        args[i].numThreads = numThreads;
        args[i].output = output;

        args[i].threadId = i;
    }

    for (int p = 0; p < numThreads*numPartitions; ++p) {
        int threadIndex = p % numThreads;
        args[threadIndex].pairs.push(pairs[p]);
    }

    // Spawn the worker threads.  Note that only numThreads-1 std::threads
    // are created and the main application thread is used as a worker
    // as well.
    for (int i = 1; i < numThreads; i++)
    {
        workers[i] = std::thread(workerThreadStart, &args[i]);
    }

    workerThreadStart(&args[0]);

    // join worker threads
    for (int i = 1; i < numThreads; i++)
    {
        workers[i].join();
    }
}
