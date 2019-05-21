#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <complex.h>
#include <omp.h>
#include <pthread.h>
#include "mandelbrot_set.h"

static int global_start;
static int CHUNK_SIZE = 8;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

struct pthread_args
{
	int x_end;
	int y_end;
	int max_iter;
	void* image;
	double view_x0;
	double view_x1; 
	double view_y0; 
	double view_y1;
    double x_stepsize; 
	double y_stepsize;
    int palette_shift;
};

void* draw_individual(void* args)
{
	struct pthread_args *arg = (struct pthread_args*) args;
	int x_end=arg->x_end;
	int y_end=arg->y_end;
	int max_iter=arg->max_iter;
	double view_x0=arg->view_x0;;
	double view_x1=arg->view_x1; 
	double view_y0=arg->view_y0; 
	double view_y1=arg->view_y1;
    double x_stepsize=arg->x_stepsize; 
	double y_stepsize=arg->y_stepsize;
    int palette_shift=arg->palette_shift;
	unsigned char (*image)[arg->x_end][3]=arg->image;
	int chunk_size=CHUNK_SIZE;
	double y;
	double x;
	complex double Z;
	complex double C;
	int k;

	while(1){
		pthread_mutex_lock(&mtx);
		if ( y_end - global_start < 1 ) {
	        pthread_mutex_unlock(&mtx);
			break;
		}
	    int y_start = global_start; 
		// increase the shared loop start    
	    global_start += chunk_size;
		
		pthread_mutex_unlock(&mtx);
		if ( y_end - y_start < chunk_size )
	        {chunk_size = y_end - y_start;}

		for (int i = y_start; i < y_start+chunk_size; i++)
		{
			
			for (int j = 0; j < x_end; j++)
			{
				y = view_y1 - i * y_stepsize;
				x = view_x0 + j * x_stepsize;

				Z = 0 + 0 * I;
				C = x + y * I;

				k = 0;

				do
				{
					Z = Z * Z + C;
					k++;
				} while (cabs(Z) < 2 && k < max_iter);

				if (k == max_iter)
				{
					memcpy(image[i][j], "\0\0\0", 3);
				}
				else
				{
					int index = (k + palette_shift)
								% (sizeof(colors) / sizeof(colors[0]));
					memcpy(image[i][j], colors[index], 3);
				}
			}
			
		}
	}
}

void mandelbrot_draw(int x_resolution, int y_resolution, int max_iter,
                     double view_x0, double view_x1, double view_y0, double view_y1,
                     double x_stepsize, double y_stepsize,
                     int palette_shift, unsigned char (*img)[x_resolution][3],
							int num_threads) {
	
	struct pthread_args* args = (struct pthread_args*) malloc (num_threads*sizeof (struct pthread_args));
	pthread_t *threads = (pthread_t*) malloc (num_threads*sizeof(pthread_t));
	global_start=0;
	// Dividing work
	for (int i=0; i<num_threads; i++)
	{
		args[i].x_end=x_resolution;
		args[i].y_end=y_resolution;
		args[i].max_iter=max_iter;
		args[i].y_stepsize=y_stepsize;
		args[i].x_stepsize=x_stepsize;
		args[i].image=img;
		args[i].view_x0=view_x0;
		args[i].view_x1=view_x1; 
		args[i].view_y0=view_y0; 
		args[i].view_y1=view_y1;
		args[i].x_stepsize=x_stepsize; 
		args[i].y_stepsize=y_stepsize;
		args[i].palette_shift=palette_shift;
		pthread_create(&threads[i],NULL,draw_individual,&args[i]);
	}
	//joining 
	for (int i=0;i<num_threads;i++)
	{
		pthread_join(threads[i],NULL);
	}
	free (args);
	free (threads);
}
