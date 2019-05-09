	#include <pthread.h>
	#include <semaphore.h>
	#include <stdbool.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <errno.h>
	#include <unistd.h>

		// Agent semaphore shows items on the tablo.
	sem_t agent_ready;

		// Every smoker semaphore shows when a smoker has the items they are need.
	sem_t smoker_semaphors[3];

		// This is an array of strings describing what every smoker type needs.
	char* smoker_types[3] = { "Match and Tobacco", "Match and Paper", "Tobacco and Paper" };

		/* 
	 		This list shows item types which are on the table. This should correspond
	 	with the smoker_types such that every item is the one the smoker has. So the
	 	first item would be paper then tobacco and then matches.

		*/

	bool items_on_table[3] = { false, false, false };

		// Each pusher push a certain type item, manage these with semaphore.
	sem_t pusher_semaphores[3];


		/*
			Function of smoker , handles waiting for the items that they need and then
		smoking. Repeat this situation three times.

		*/

	void* smoker(void* arg)
	{
		int smoker_id = *(int*) arg;
		int type_id   = smoker_id % 3;

		// 3 times smokes
		for (int s = 0; s < 3; ++s)
		{
			printf("\033[0;37mSmoker %d \033[0;31m>>\033[0m Waiting for %s\n",
				smoker_id, smoker_types[type_id]);

			// Wait for the proper combination of items to be on the tablo.
			sem_wait(&smoker_semaphors[type_id]);

			// Make the cigarette before releasing the agent.
			printf("\033[0;37mSmoker %d \033[0;32m<<\033[0m Making the cigarette now\n", smoker_id);
			usleep(rand() % 50000);
			sem_post(&agent_ready);

			// We are now smoking
			printf("\033[0;37mSmoker %d \033[0;37m--\033[0m Smoking now\n", smoker_id);
			usleep(rand() % 50000);
		}

		return NULL;
	}

	// This semaphore give the pusher exclusive access to the items on the tablo.
	sem_t pusher_lock;

		/*
		  The pusher is responsible from releasing the proper smoker semaphore when the
		right items are on the tablo.
		*/

	void* pusher(void* arg)
	{
		int pusher_id = *(int*) arg;

		for (int s = 0; s < 12; ++s)
		{
			// Wait for this pusher to be needed.
			sem_wait(&pusher_semaphores[pusher_id]);
			sem_wait(&pusher_lock);

			// Check if the other item we need is on the tablo.
			if (items_on_table[(pusher_id + 1) % 3])
			{
				items_on_table[(pusher_id + 1) % 3] = false;
				sem_post(&smoker_semaphors[(pusher_id + 2) % 3]);
			}
			else if (items_on_table[(pusher_id + 2) % 3])
			{
				items_on_table[(pusher_id + 2) % 3] = false;
				sem_post(&smoker_semaphors[(pusher_id + 1) % 3]);
			}
			else
			{
				// The other items are not on the tablo yet.
				items_on_table[pusher_id] = true;
			}

			sem_post(&pusher_lock);
		}

		return NULL;
	}


	// The agent puts items on the tablo.

	void* agent(void* arg)
	{
		int agent_id = *(int*) arg;

		for (int s = 0; s < 6; ++s)
		{
			usleep(rand() % 200000);

			// Wait for a lock on the agent.
			sem_wait(&agent_ready);

			// Release the items which is this agent gives out.
			sem_post(&pusher_semaphores[agent_id]);
			sem_post(&pusher_semaphores[(agent_id + 1) % 3]);

			printf("\033[0;35m==> \033[0;33mAgent %d giving out %s\033[0;0m\n",
				agent_id, smoker_types[(agent_id + 2) % 3]);
		}

		return NULL;
	}


	//The main thread handles the agent's arbitration of items.
	 
	int main(int argc, char* arvg[])
	{
		// Seed our random number since we will be using random numbers.
		srand(time(NULL));

		/* 
			There is only one agent semaphore since only one set of items may be on
		the table at any given time. A values of 1 = nothing on the tablo.
		*/

		sem_init(&agent_ready, 0, 1);

		// Initialize the pusher lock semaphore.
		sem_init(&pusher_lock, 0, 1);

		// Initialize the semaphores for the smokers and pusher.
		for (int s = 0; s < 3; ++s)
		{
			sem_init(&smoker_semaphors[s], 0, 0);
			sem_init(&pusher_semaphores[s], 0, 0);
		}



		// Smoker ID is will be passed to the threads. Allocate the ID is on the stack.
		int smoker_ids[6];

		pthread_t smoker_threads[6];

		// Create the 6 smoker threads with IDs.
		for (int s = 0; s < 6; ++s)
		{
			smoker_ids[s] = s;

			if (pthread_create(&smoker_threads[s], NULL, smoker, &smoker_ids[s]) == EAGAIN)
			{
				perror("Insufficient resources to create thread");
				return 0;
			}
		}

		// Pusher ID is will be passed to the threads. Allocate the ID is on the stack.
		int pusher_ids[6];

		pthread_t pusher_threads[6];

		for (int s = 0; s < 3; ++s)
		{
			pusher_ids[s] = s;

			if (pthread_create(&pusher_threads[s], NULL, pusher, &pusher_ids[s]) == EAGAIN)
			{
				perror("Insufficient resources to create thread");
				return 0;
			}
		}

		// Agent ID is will be passed to the threads. Allocate the ID is on the stack.
		int agent_ids[6];

		pthread_t agent_threads[6];

		for (int s = 0; s < 3; ++s)
		{
			agent_ids[s] =s;

			if (pthread_create(&agent_threads[s], NULL, agent, &agent_ids[s]) == EAGAIN)
			{
				perror("Insufficient resources to create thread");
				return 0;
			}
		}

		// Make sure all the smokers are finished smoking.
		for (int s = 0; s < 6; ++s)
		{
			pthread_join(smoker_threads[s], NULL);
		}

		return 0;
	}
