#ifndef PARTICLESYSTEMGPU_H
#define PARTICLESYSTEMGPU_H

// Standard Library Includes
#include <iostream>
#include <fstream>

// Program Includes
#include "particleSystem.h"
//#include "ordered.h"

// Visualization Includes
#include <glad/glad.h>

// GPU Library Includes
#include <cuda.h>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cuda_gl_interop.h>

class ParticleSystemGPU : public ParticleSystem
{
public:
	ParticleSystemGPU(int numParticles, int init_method, int seed);

	~ParticleSystemGPU(void);

	float* getPositions(void);

	float* getVelocities(void);

	unsigned int* getColors(void);

	void update(float timeDelta);

	void flip();

	void writecurpostofile(char* file, int steps, float milliseconds);

	void display();

	//Particle Data Device
	float* d_positions;
	float* d_positions2;
	float* d_velocities;
	unsigned int* d_colors;
	unsigned char* d_particleType;

protected:
	
	//Kernel specs
	dim3 dimBlock;
	dim3 dimGrid;

#if orderedParticles
	int electronGridSize;
	int protonGridSize;
	int neutronGridSize;
#endif

	cudaGraphicsResource* positionResource;
	cudaGraphicsResource* colorResource;

	cudaEvent_t event;

	//Double buffering
	bool buf;
	float* src;
	float* dst;

	//Binning
	int* bin;
	int* d_bin; //(binx, biny, binz, binDepth, 1)
	int* overflow;
	int* d_overflow;
};


#endif