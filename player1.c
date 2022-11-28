#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include "binary_sem.h"
#include "semun.h"

#define BUF_SIZE 1024
#define OBJ_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

// Program 10 - Player 1

/*
    Notes:
    shmget() can obtain the identifier of a previously created
    shared memory segment, or it can create a new set

    1 for X, -1 for O

    p2 is responsible for iterating counter
*/

// block of shared memory
struct shmseg
{
    int counter;
    int board[3][3];
};

// check if all 9 spots are filled
// if full, return 1 
// else, return 0
// TODO test
int checkBoardFull(struct shmseg *smap)
{
    int i = 0;
    int j = 0;
    int spacesFilled = 0;

    for (i = 0; i <= 3; i++)
    {
        for (j = 0; j <= 3; j++)
        {
            if (smap->board[i][j] != 0)
            {
                spacesFilled++;
            }
        }
    }
    
    if (spacesFilled == 9)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

// The following functions are helper functions for p1Move

// look for winning row placement and place if so
// rows: 0,0 and 0,2 - 1,0 and 1,2 - 2,0 and 2,2
// TODO test
int tryRow(struct shmseg *smap, int x1, int y1, int x2, int y2)
{
    // check x1, y1 for X
    if (smap->board[x1][y1] != 1)
    {
        return 1;
    }
    // check x2, y2 for X
    if (smap->board[x2][y2] != 1)
    {
        return 1;
    }

    // for any of the 3 rows, x1 should equal winX
    int winX = x1;
    // and winY will always be 1
    int winY = 1;

    // if winning place is available
    if (smap->board[winX][winY] == 0)
    {
        // place
        smap->board[winX][winY] = 1;
        return 0;
    }

    // if we can't place
    return 1;
}

// **end helper functions**

// first play - a random corner
// second play - the opposite corner (if available)
// third play - try the center if O hasn't placed there
// returns 0 if move successfully made
int p1Move(struct shmseg *smap)
{
    // Stores a randomly generated value
    int r;

    // first move: choose a random corner to place X
    // corners: 0,0  0,2  2,0  2,2
    if (smap->counter == 0)
    {
        // Seed time
        time_t t;
        srand((unsigned) time(&t));

        // Generate random number from 1-4
        r = rand() % (3 + 1 - 0) + 1;
        if (r == 1)
        {
            smap->board[0][0] = 1;
            return 0;
        }
        else if (r == 2)
        {
            smap->board[0][2] = 1;
            return 0;
        }
        else if (r == 3)
        {
            smap->board[2][0] = 1;
            return 0;
        }
        else if (r == 4)
        {
            smap->board[2][2] = 1;
            return 0;
        }
        else
        {
            // This should never happen
            printf("ERROR: r not a value between 1-4\n");
            printf("r's value: %d\n", r);
            exit(EXIT_FAILURE);
        }
    }

    // p1's second move will first try to 
    // place at the opposite corner of p1's first move
    if (smap->counter == 1)
    {
        // if X is left top (and right bottom is clear)
        if (smap->board[0][0] == 1 && smap->board[2][2] != -1)
        {
            // place right bottom
            smap->board[2][2] = 1;
            return 0;
        }
        // if X is right top
        else if (smap->board[0][2] == 1 && smap->board[2][0] != -1)
        {
            // place left bottom
            smap->board[2][0] = 1;
            return 0;
        }
        // if X is left bottom
        else if (smap->board[2][0] == 1 && smap->board[0][2] != -1)
        {
            // place right top
            smap->board[0][2] = 1;
            return 0;
        }
        // if X is right bottom
        else if (smap->board[2][2] == 1 && smap->board[0][0] != -1)
        {
            // place left top
            smap->board[0][0] = 1;
            return 0;
        }
        // if we make it here, O is placed in the opposite corner as X
        // so X should just place in a random corner(?)
        // NOTE I haven't seen if this works or not yet
        else
        {
            // try left top
            if (smap->board[0][0] == 0)
            {
                smap->board[0][0] = 1;
                return 0;
            }
            // try right top
            if (smap->board[0][2] == 0)
            {
                smap->board[0][2] = 1;
                return 0;
            }
            // try left bottom
            if (smap->board[2][0] == 0)
            {
                smap->board[2][0] = 1;
                return 0;
            }
            // try right bottom
            if (smap->board[2][0] == 0)
            {
                smap->board[2][0] = 1;
                return 0;
            }
        }
    }

    // third move:
    // if X is placed in 2 opposite corners, p1 should try to play center
    // for a diagonal win
    if (smap->counter == 2)
    {
        // if X has left top and bottom right, and center is available
        if (smap->board[0][0] == 1 && smap->board[2][2] == 1 && smap->board[1][1] == 0)
        {
            // place center
            smap->board[1][1] = 1;
            return 0;
        }
        // if X has bottom left and top right, and center is available
        if (smap->board[2][0] == 1 && smap->board[0][2] == 1 && smap->board[1][1] == 0)
        {
            // place center
            smap->board[1][1] = 1;
            return 0;
        }

        // if we end up here, x has no diagonal play -or- O has the center
        // check if X can win in a row or column

        // if X has left top and left bottom, and left middle is available
        if (smap->board[0][0] == 1 && smap->board[2][0] == 1 && smap->board[1][0] == 0)
        {
            // place left middle
            smap->board[1][0] = 1;
            return 0;
        }
        // if X has right top and right bottom, and right middle is available
        if (smap->board[0][2] == 1 && smap->board[2][2] == 1 && smap->board[1][2] == 0)
        {
            // place right middle
            smap->board[1][2] = 1;
            return 0;
        }
        // if X has left top and right top, and top middle is available
        if (smap->board[0][0] == 1 && smap->board[0][2] == 1 && smap->board[0][1] == 0)
        {
            // place top middle
            smap->board[0][1] = 1;
            return 0;
        }
        // if X has left bottom and right bottom, and bottom middle is available
        if (smap->board[2][0] == 1 && smap->board[2][2] == 1 && smap->board[2][1] == 0)
        {
            // place top middle
            smap->board[2][1] = 1;
            return 0;
        }

        // if not, take another available corner?


    }
}

// return 1 - row win not found
// return 0 - row win found
int rowWin(struct shmseg *smap)
{
    int i;
  
    for(i = 0; i < 3; i++)
    {
        // row win found
        if(smap->board[i][0] == 1 && smap->board[i][0] == smap->board[i][1] && smap->board[i][1] == smap->board[i][2])
	    {
	        return 0;
	    }
    }
    // row win not found
    return 1;
}

int columnWin(struct shmseg *smap)
{
    int i;

    for(i = 0; i < 3; i++)
    {
        // column win found
        if(smap->board[0][i] == 1 && smap->board[0][i] == smap->board[1][i] && smap->board[1][i] == smap->board[2][i])
	    {
	        return 0;
	    }
    }
    // column win not found
    return 1;
}

int diagonalWin(struct shmseg *smap)
{
    // top left to bottom right diagonal win
    if(smap->board[0][0] == 1 && smap->board[0][0] == smap->board [1][1] && smap->board[1][1] == smap->board[2][2])
    {
        return 0;
    }
  
    // bottom left to top right diagonal win
    if(smap->board[2][0] == 1 && smap->board[2][0] == smap->board[1][1] && smap->board[1][1] == smap->board[0][2])
    {
        return 0;
    }
  
    return 1;
}

int rowBlock(struct shmseg *smap)
{
    int i;

    for(i = 0; i < 3; i++)
    {
        if(smap->board[i][0] == 1 && smap->board[i][1] == 1)
	    {
	        // right block
	        return 0;
	    }
        if(smap->board[i][0] == 1 && smap->board[i][2] == 1)
	    {
	        // middle block
	        return 0;
	    }
        if (smap->board[i][1] == 1 && smap->board[i][2] == 1)
	    {
	        // left block
	        return 0;
	    }
    }

    // row block not found
    return 1;
}

int columnBlock(struct shmseg *smap)
{
    int i;

    for(i = 0; i < 3; i++)
    {
      if(smap->board[0][i] == 1 && smap->board[1][i] == 1)
	    {
	        // bottom block
	        return 0;
	    }
      if(smap->board[0][i] == 1 && smap->board[2][i] == 1)
	    {
	        // middle block
	        return 0;
	    }
      if (smap->board[1][i] == 1 && smap->board[2][i] == 1)
	    {
	        // top block
	        return 0;
	    }
    }

    // column block not found
    return 1;
}

int diagonalBlock(struct shmseg *smap)
{
    // top left to bottom right block
    if(smap->board[0][0] == 1 && smap->board[1][1] == 1)
    {
        // bottom right block
        return 0;
    }
    if(smap->board[0][0] == 1 && smap->board[2][2] == 1)
    {
        // center block
        return 0;
    }
    if(smap->board[1][1] == 1 && smap->board[2][2] == 1)
    {
        // top left block
        return 0;
    }

    // top right to bottom left block
    if(smap->board[0][2] == 1 && smap->board[1][1] == 1)
    {
        // bottom left block
        return 0;
    }
    if(smap->board[0][2] == 1 && smap->board[2][0] == 1)
    {
        // center block
        return 0;
    }
    if(smap->board[1][1] == 1 && smap->board[2][0] == 1)
    {
        // top right block
        return 0;
    }
  
    // diagonal block not found
    return 1;
}

int checkError(int e, const char *str)
{
    if (e == -1)
    {
        if (errno == EINTR) return e;
        perror(str);
        exit(EXIT_FAILURE);
    }
    return e;
}

char intToChar(struct shmseg *smap, int i, int j)
{
    if (smap->board[i][j] == 1)
    {
        return 'X';
    }
    else if (smap->board[i][j] == -1)
    {
        return 'O';
    }
    else
    {
        return ' ';
    }
}

// function to print the tic-tac-toe board
void printBoard(struct shmseg *smap)
{
    int iteration = 6;
    int a = 0;
    for(int i = 1; i <= iteration; i++)
    {
        if (i % 2 != 0 )
        {
            printf("  %c | %c | %c ",  intToChar(smap,a,0), intToChar(smap,a,1), intToChar(smap,a,2));
            a++;
        }
        else if (i == 6)
        {
            printf("\n");
        }
        else
        {
            printf("\n ---|---|---\n");
        }
    }

    printf("\n");
}

int main(int argc, char* argv[])
{
    // Misc. setup
    int fd;
    int rblock, cblock, dblock;

    // Seed time
    time_t t;
    srand((unsigned) time(&t));

    // Semaphore/shared memory setup

    struct shmseg *smap;

    // semaphore id, shared memory id
    int semid, shmid, val;
    //union semun dummy;

    // semaphore key and shared memory key
    key_t semK, shmK;

    // 1. Attempt to create FIFO called xoSync
    if (mkfifo("xoSync", S_IRWXU) == -1)
    {
        // If the error isn't EEXIST
        if (errno != EEXIST)
        {
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
        printf("FIFO already exists\n");
    }
    else {printf("FIFO created\n");}

    // 2. Generate 2 random numbers for creating System V keys
    // Generate 2 values between 10-99
    int num1 = rand() % (99 + 1 - 10) + 10;
    int num2 = rand() % (99 + 1 - 10) + 10;

    // 3. Generate the System V keys with ftok using the random numbers 
    // and the FIFO xoSync. Use the first value generated as the projection value
    // for shared memory and the second value generated 
    // as the projection value for the semaphores.

    shmK = ftok("xoSync",num1);
    if (shmK == -1)
    {
        perror("ftok1");
        exit(EXIT_FAILURE);
    }
    semK = ftok("xoSync",num2);
    if (semK == -1)
    {
        perror("ftok2");
        exit(EXIT_FAILURE);
    }

    // 4. Create the block of shared memory
    // TODO verify this is right
    checkError(shmid = shmget(shmK, sizeof(struct shmseg), IPC_CREAT | OBJ_PERMS), "shmget");

    // 5. Create a semaphore set with a size of 2
    checkError(semid = semget(semK, 2, IPC_CREAT | OBJ_PERMS), "semget");

    // 6. Initialize the semaphores in the semaphore set.
    // 0 is p1, 1 is p2
    // Set one semaphore to available
    checkError(initSemAvailable(semid,0), "initSemAvailable");
    // Set one semaphore to in use
    checkError(initSemInUse(semid,1), "initSemInUse");

    // 7. Attach the segment of shared memory to process and initialize it
    smap = shmat(shmid, NULL, 0);
    if (smap == (void *) -1)
    {
        checkError(-1, "shmat");
    }

    // 8. Open the FIFO xoSync for write.
    checkError(fd = open("xoSync", O_WRONLY), "open producer for write");

    // 9. Write the random numbers generated in step 2 and used in step 3 to the FIFO.
    checkError(write(fd, &num1, sizeof(num1)), "write 1");
    checkError(write(fd, &num2, sizeof(num2)), "write 2");

    // 10. Close the FIFO
    close(fd);

    // TODO initialize counter to 0 here?
    smap->counter = 0;

    // 11. Enter the gameplay loop.
    while (smap->counter > -1)
    {
        // 1. reserve player 1's semaphore
        checkError(reserveSem(semid, 0), "reserveSem");

        // 2. display the state of the game board
        printBoard(smap);

        // 3. make player 1's move
        // logic goes here - dont know if this is how we will do it
        // first play - a random corner
        // second play - the opposite corner (if available)
        // third play - try the center if O hasn't placed there
        p1Move(smap);

        // 4. display the state of the game board.
        printBoard(smap);

        // 5. if player 1 has won, or no more plays exist set the turn counter to -1
        if (rowWin(smap) == 0 || columnWin(smap) == 0 || diagonalWin(smap) == 0 || checkBoardFull(smap) == 1)
        {
            smap->counter = -1;
        }

        // 6. release player 2's semaphore
        checkError(releaseSem(semid, 1), "releaseSem");

        // DEBUG see counter's value
        printf("DEBUG: counter = %d\n",smap->counter);
    }

    // 12. Open the FIFO xoSync for write
    checkError(fd = open("xoSync", O_WRONLY), "open producer");
    // 13. Close the FIFO
    close(fd);
    // 14. Detach the segment of shared memory
    checkError(shmdt(smap), "shmdt");
    // 15. Delete semaphores and shared memory before exiting
    checkError(semctl(semid,0,IPC_RMID),"semctl");
    checkError(shmctl(shmid,0,IPC_RMID),"shmctl");

    exit(EXIT_SUCCESS);
}
