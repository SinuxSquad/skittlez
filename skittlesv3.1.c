/*
    Authors: Samad Mazarei, Salvatore Pena
    Date: 12/8/2019
    Dr. Feister
    COMP262

    Using openmp, the following program models an experiment in which bags of 60 skittles in 5 colors are opened and compared to all previously opened bags of skittles, looking for two 
    that contain the same number of each color. Doing this repeaetdly, and tracking the number of bags opened before finding duplicates, we calculate a running average that is approximatly 127
    For this we used a random number generator and modulos arithmetic. The method chosen mimicks randomly selected skittles being placed into each bag,  giving each individual skittle 
    a 1 in 5 chance of being any specific color. This pogram implements openmp in the search portion. This is too small a program and is far from computationally intense, so the overhead 
    of multiple threads out weighs any benefits. Two previous versions attempted to have several threads all calculate their own average simultaneously, then calculating an overall average from 
    those values. This proved to be extrmemely slow, with an average that is much less accurate than the current implementation. The more bags of skittles that are created and compared, the more one may trust 
    the result. We chose to parallelize the portion of this code that takes the longest to carry out - that is, the block in which we compare all previously opened bags to the currently opened bag.
    *NOTE* 
    *The serial implementation of this program still outperforms this one. Upon reflection, MPI would have been the better choice for this project IF we were allowed to implemet 
    time restraints programmatically.
    **Previous versions of this parallel implementation are avaialble upon request.
*/

#include <stdlib.h> // rand()
#include <stdio.h> //io (printf)
#include <time.h> // time(NULL)
#include <omp.h> // Multi threading lib

#define rTHRDS 8 // Number of threads to request
#define N 2500 // Array size for the array of opened bags, determined through experimentation
#define COLORS 5 // Number of colors in a bag 
#define SKITTLES 60 // Number of skittles in a bag

// This is a throw away function with no error checking and only works for what I am using it for 
// which is to raise 10 to a power, and ease readability in fillbag function
unsigned long int powers(int base, int powr)
{
    int temp = 1;
    for(int i = 0; i < powr; i++)
    {
        temp*=base;
    }
    return temp;
}
// This function generates 5 numbers that add to 60
// Then concatenaets them into a single integer value
double fillbag()
{
    int lower = 42;
    int upper = 85;
    int limit = (rand() % (upper - lower + 1)) + lower;
    int arr[COLORS] = {0}; // 5 element array to store number of occurences of 5 colors
    for(int i = 0; i < limit; i++)
    {
        arr[rand()%COLORS]++; // A value 0-5 is generated, and that element in the array is incremented
    }
    // This will hold the unique integer value associated with 
    // this bag of skittles
    unsigned long long int bagsumvalue = 0;
    // This will create an integer value from 
    // color values - 12,13,14,15,16 becomes 12131.41516 
    bagsumvalue = arr[0]*powers(10,8)+arr[1]*powers(10,6)+arr[2]*powers(10,4)+arr[3]*powers(10,2)+arr[4];
    return bagsumvalue/100000.00;
}

// This function takes pointer values to the variables that track and calculate 
// the running average, and using the values of the most recently discovered double
// updates them all, returning the newly calculated average
double updateAverage(long long int *sob, double *df, int nv)
{
    // By adding the most recent number of bags opened to find a double to the total number of bags opened
    // and divinding by the current number of duplicates found, we are able to calculate the most recent 
    // average number. 
    *sob+=nv;
    ++*df;
    return *sob / *df;
}
// This function sets the value pointed to by its argument to 0 (uneccessary but left in from
// previous versions for code readability)
void reset(unsigned long long int *bo)
{
    *bo = 0;
}


int main()
{
    double opened[N]; // Array to store opened bags for searching
    double * ptr = (double*)calloc(N, sizeof(double));
    ptr = opened;
    unsigned long long int bags_opened = 0; // Current number of bags opened , changes when new duplicates found
    unsigned long long int sum_of_bags = 0; // Number of total bags opened
    double doubles_found = 0; // Number of doubles found so far
    double running_average = 0; // Running average number of bags opened to find doubles
    ; // Making several bags at a time
    
    int match = 0; // Flag for checking if match found 
    srand(time(NULL)); // Seeding time
    omp_set_num_threads(rTHRDS); // Request a number of threads
    int ID; // Declaring the thread ID variable for use below 
    const int epstein_didnt_kill_himself = 1; // Since the while loop needs to run indefinitly, a const non zero int value is used
    
    double start = omp_get_wtime();
    double end;
    while (epstein_didnt_kill_himself) // Infinite loop since value of argument is always (1) true
    {
        
        opened[bags_opened] = fillbag(); // Newly filled bag placed in array at 0 or after most recent bag determined not to have a duplicate
        match = 0; // Match flag used to signal to all threads when a duplicate has been found
        #pragma omp parallel for num_threads(8) private(ID) shared(match, start, end) schedule(static,8) // All threads have their own ID but share access to match
        for(int i =0; i < bags_opened;i++) // Iterate over array but stopping prior to index of most recently added bag (the one we eill be comparing)
        {
            ID = omp_get_thread_num(); // Initialize thread ID
             if(match) // If match flag set, thread does no calculation for remaining iterations (cannot use break in parallel for loop)
            {
                continue; 
            }
            else if(opened[bags_opened]==opened[i]) // Check the most recently added (opened) bag against all others
            {
                // Update running average and incrememnting bags_opened for calculation
                running_average = updateAverage(&sum_of_bags, &doubles_found, ++bags_opened);
                // Set flag so that other threads stop comparing (break does not work in omp loops)
                match = 1;
                // Every 5000 duplicates output the average (Takes between 1 and 2 seconds)
                if((int)doubles_found%1000==0) {
                    printf("******\nTime: %f\nAverage: %f\nOpened: %lld\nDuplicates : %d\n",omp_get_wtime() - start, running_average,sum_of_bags, (int)doubles_found);
                    start = omp_get_wtime();
                    }
                // Reset number of bags opened to begin again
                reset(&bags_opened);
            }
        }
        if(!match) // In the case that the most recenlty bag added does not match
        {
            // Increment variable to track bags placed into opened array
            bags_opened++;
        }
    }
     return 0;
    
}