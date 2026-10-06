#ifdef _SVID_SOURCE
#undef _SVID_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#define NUM_LOOPS 25

// Shared memory indices
#define BANK_ACCOUNT 0
#define TURN 1

void ClientProcess(volatile int *ShmPTR);

int main()
{
    int ShmID;
    volatile int *ShmPTR;
    pid_t pid;
    int status;

    // Request shared memory for two integers: BankAccount and Turn
    ShmID = shmget(IPC_PRIVATE, 2 * sizeof(int), IPC_CREAT | 0666);
    if (ShmID < 0)
    {
        perror("Error: shmget failed");
        exit(1);
    }

    // Attach the shared memory segment
    ShmPTR = (volatile int *)shmat(ShmID, NULL, 0);
    if ((long)ShmPTR == -1)
    {
        perror("Error: shmat failed");
        exit(1);
    }

    // Initialize BankAccount = 0, Turn = 0 (Dad goes first)
    ShmPTR[BANK_ACCOUNT] = 0;
    ShmPTR[TURN] = 0;

    // Fork the child process
    pid = fork();
    if (pid < 0)
    {
        perror("Error: fork failed");
        exit(1);
    }
    else if (pid == 0)
    {
        // Child process executes the Poor Student routine
        ClientProcess(ShmPTR);
        exit(0);
    }

    // Parent process: Dear Old Dad
    srand((unsigned int)(time(NULL) ^ (getpid() << 16)));

    for (int i = 0; i < NUM_LOOPS; i++)
    {
        // 1. Sleep a random amount of time between 0 - 5 seconds
        sleep(rand() % 6);

        // 2. Strict alternation: spin-wait until Turn == 0
        while (ShmPTR[TURN] != 0)
        {
            // Busy wait
        }

        // 3. Read current BankAccount inside the critical section
        int account = ShmPTR[BANK_ACCOUNT];

        // 4. If account <= 100, try to deposit money
        if (account <= 100)
        {
            int balance = rand() % 101; // 0 - 100

            if (balance % 2 == 0)
            {
                // Even: deposit into account
                account += balance;
                printf("Dear old Dad: Deposits $%d / Balance = $%d\n", balance, account);
            }
            else
            {
                // Odd: no money
                printf("Dear old Dad: Doesn't have any money to give\n");
            }
        }
        else
        {
            printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n", account);
        }
        fflush(stdout);

        // 5. Write back to shared BankAccount
        ShmPTR[BANK_ACCOUNT] = account;

        // 6. Set Turn = 1 (Student's turn)
        ShmPTR[TURN] = 1;
    }

    // Wait for child process to complete
    wait(&status);

    // Detach and remove shared memory with error checking
    if (shmdt((const void *)ShmPTR) == -1)
    {
        perror("Error: shmdt failed");
    }

    if (shmctl(ShmID, IPC_RMID, NULL) == -1)
    {
        perror("Error: shmctl IPC_RMID failed");
    }

    return 0;
}

void ClientProcess(volatile int *ShmPTR)
{
    // Seed child random generator uniquely
    srand((unsigned int)(time(NULL) ^ getpid()));

    for (int i = 0; i < NUM_LOOPS; i++)
    {
        // 1. Sleep a random amount of time between 0 - 5 seconds
        sleep(rand() % 6);

        // 2. Strict alternation: spin-wait until Turn == 1
        while (ShmPTR[TURN] != 1)
        {
            // Busy wait
        }

        // 3. Read current BankAccount inside the critical section
        int account = ShmPTR[BANK_ACCOUNT];

        // 4. Randomly generate balance needed between 0 - 50
        int balance = rand() % 51;
        printf("Poor Student needs $%d\n", balance);

        // 5 & 6. Check if balance needed is <= account
        if (balance <= account)
        {
            account -= balance;
            printf("Poor Student: Withdraws $%d / Balance = $%d\n", balance, account);
        }
        else
        {
            printf("Poor Student: Not Enough Cash ($%d)\n", account);
        }
        fflush(stdout);

        // 7. Write back to shared BankAccount
        ShmPTR[BANK_ACCOUNT] = account;

        // 8. Set Turn = 0 (Dad's turn)
        ShmPTR[TURN] = 0;
    }

    // Detach shared memory in child
    if (shmdt((const void *)ShmPTR) == -1)
    {
        perror("Error: child shmdt failed");
    }
}