Compiling and Running the program: SORT.c

SORT.c, shm_com.h and semun.h files were both submitted for the compilation and running of the SORT program.

To compile and run SORT.c:
1) Each .c files and .h file should be in the same folder before compiling. 
2) Open terminal and change directory to the created folder, then compile by typing "gcc -o STATS STATS.c"

3) Then run the code by typing "./SORT"
4) Follow the prompt and view the results

The purpose of this project was to create a concurrent program that uses shared memory to store a list of 5 values and sort them in descending order by forking 4 processes to perform the swapping.
Goals

• Setup shared memory for storage of integer values and state of the array
• Create semaphores to allow only on process to access shared memory at any given time
• Create a loop to traverse through the array two indices at a time starting from index 0
• Check if swap is needed, swap and change the value of state to sorted, if not leave as is and move to the next pair
• After the state of each index has been changed to sorted, check for max, min, median values
• Children are killed and semaphores for integer and state array are deleted and program terminates