#include "particleSystemGpu.h"

__constant__ double d_inv_masses[3];
__constant__ double d_charges[3];

__global__ void update_naive(double timeDelta, int numParticles, double* positions, double* velocities, unsigned char* particleType) {
	int gid = blockIdx.x * blockDim.x + threadIdx.x;
	if (gid < numParticles) {
		int part_type = particleType[gid];
		double force_x = 0.0;
		double force_y = 0.0;
		double force_z = 0.0;
		for (int j = 0; j < numParticles; j++) {
			double dist_x = positions[gid] - positions[j];
			double dist_y = positions[gid + 1] - positions[j + 1];
			double dist_z = positions[gid + 2] - positions[j + 2];
			double dist_square = (dist_x * dist_x) + (dist_y * dist_y) + (dist_z * dist_z);
			double dist = sqrt(dist_square);
			if (gid == j || dist < yukawa_cutoff) {
				continue;
			}

			//Coulomb force
			double force = (double)coulomb_scalar / dist_square * d_charges[part_type] * d_charges[particleType[j]];
			
			

			//Strong Forces
			//P-N close attraction N-N close attraction 
			if (part_type != 0 && particleType[j] != 0) {
				force += yukawa_scalar * exp(-dist / yukawa_radius) / dist;
			}
			//Break force into components
			force_x += force * dist_x / dist;
			force_y += force * dist_y / dist;
			force_z += force * dist_z / dist;
		}

		//Update velocities
		velocities[gid] += force_x * d_inv_masses[part_type] * timeDelta;
		velocities[gid + 1] += force_y * d_inv_masses[part_type] * timeDelta;
		velocities[gid + 2] += force_z * d_inv_masses[part_type] * timeDelta;

		//Update positions from velocities
		positions[gid * 4] += velocities[gid * 3] * timeDelta;
		if (abs(positions[gid * 4]) > 1) {
			velocities[gid * 3] = -1 * velocities[gid * 3];
		}
		
		positions[gid * 4 + 1] += velocities[gid * 3 + 1] * timeDelta;
		if (abs(positions[gid * 4 + 1]) > 1) {
			velocities[gid * 3 + 1] = -1 * velocities[gid * 3 + 1];
		}

		positions[gid * 4 + 2] += velocities[gid * 3 + 2] * timeDelta;
		if (abs(positions[gid * 4 + 2]) > 1) {
			velocities[gid * 3 + 2] = -1 * velocities[gid * 3 + 2];
		}
	}

}



ParticleSystemGPU::ParticleSystemGPU(int numParticles, int initMethod, int seed) {
		p_numParticles = numParticles;

		blockSize = TILE_SIZE;
		gridSize = (int)ceil((double)numParticles / (double)TILE_SIZE);
		cudaEventCreate(&event);

		// Initialize Positions array
		int positionElementsCount = 4 * numParticles;
		positions = new double[positionElementsCount];

		// Initialize Colors array
		int colorElementsCount = 3 * numParticles;
		colors = new unsigned int[colorElementsCount];

		int velocityElementsCount = 3 * numParticles;
		velocities = new double[velocityElementsCount];

		// Initialize Particle Type array
		particleType = new unsigned char[numParticles];

		// Circular initialization
		if (initMethod == 0) {
				for (unsigned int i = 0; i < numParticles; i++) {
						double theta = (double)((numParticles - 1 - i) / (double)numParticles * 2.0 * 3.1415); // Ensure floating-point division
						positions[i * 4] = (double)cos(theta);
						positions[i * 4 + 1] = (double)sin(theta);
						positions[i * 4 + 2] = 1.0f;
						positions[i * 4 + 3] = 1.0f; // This will always stay as 1, it will be used for mapping 3D to 2D space

						colors[i * 3] = i % 255;
						colors[i * 3 + 1] = 255 - (i % 255);
						colors[i * 3 + 2] = 55;
				}
		}
		//Read from a file
		else if (initMethod == 1) {

		}
		// Random initialization in 3 dimensions
		else if (initMethod == 2) {
				if (seed != -1) {
						srand(seed);
				}
				for (unsigned int i = 0; i < numParticles; i++) {
						// Randomly initialize position in range [-1,1)
						positions[i * 4] = ((double)(rand() % 2000) - 1000.0) / 1000.0;
						positions[i * 4 + 1] = ((double)(rand() % 2000) - 1000.0) / 1000.0;
						positions[i * 4 + 2] = ((double)(rand() % 2000) - 1000.0) / 1000.0;
						positions[i * 4 + 3] = 1.0f; // This will always stay as 1, it will be used for mapping 3D to 2D space

						// Randomly initializes velocity in range [-250000,250000)
						velocities[i * 3] = ((double)(rand() % 500) - 250.0) * 1000.0;
						velocities[i * 3 + 1] = ((double)(rand() % 500) - 250.0) * 1000.0;
						velocities[i * 3 + 2] = ((double)(rand() % 500) - 250.0) * 1000.0;

						// Generates random number (either 0, 1, 2) from uniform dist
						//particleType[i] = rand() % 3 % 2; 
						//particleType[i] = rand() % 3;
						particleType[i] = 2;

						// Sets color based on particle type
						if (particleType[i] == 0) { // If Electron
								colors[i * 3] = ELECTRON_COLOR[0];
								colors[i * 3 + 1] = ELECTRON_COLOR[1];
								colors[i * 3 + 2] = ELECTRON_COLOR[2];
						}
						else if (particleType[i] == 1) { // If Proton
								colors[i * 3] = PROTON_COLOR[0];
								colors[i * 3 + 1] = PROTON_COLOR[1];
								colors[i * 3 + 2] = PROTON_COLOR[2];
						}
						else {
								colors[i * 3] = NEUTRON_COLOR[0]; //Else neutron
								colors[i * 3 + 1] = NEUTRON_COLOR[1];
								colors[i * 3 + 2] = NEUTRON_COLOR[2];
						}
				}
		}
		//Error bad method
		else {
				std::cerr << "Bad Initialization";
		}

#if (RENDER_ENABLE)
			glGenVertexArrays(1, &VAO);

			glBindVertexArray(VAO);

			glGenBuffers(1, &positionBuffer);
			glGenBuffers(1, &colorBuffer);

			glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(double) * 4 * numParticles, positions, GL_STREAM_DRAW);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

			glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(unsigned int) * 3 * numParticles, colors, GL_STREAM_DRAW);
			glVertexAttribIPointer(1, 3, GL_UNSIGNED_INT, 0, (void*)0);

			glBindBuffer(GL_ARRAY_BUFFER, 0);

			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);

			shaderProgram = new Shader();
#endif

		//Initialize device

		cudaMemcpyToSymbol(d_inv_masses, inv_masses, 3 * sizeof(double));
		cudaMemcpyToSymbol(d_charges, charges, 3 * sizeof(double));

#if (RENDER_ENABLE)
		cudaGraphicsGLRegisterBuffer(&positionResource, positionBuffer, cudaGraphicsMapFlagsNone);
#else
		cudaMalloc(&d_positions, positionElementsCount * sizeof(double));
		cudaMemcpy(d_positions, positions, positionElementsCount * sizeof(double), cudaMemcpyHostToDevice);
#endif

		cudaMalloc(&d_velocities, velocityElementsCount * sizeof(double));
		cudaMemcpy(d_velocities, velocities, velocityElementsCount * sizeof(double), cudaMemcpyHostToDevice);

#if (RENDER_ENABLE)
		cudaGraphicsGLRegisterBuffer(&colorResource, colorBuffer, cudaGraphicsMapFlagsNone);
#else
		cudaMalloc(&d_colors, colorElementsCount * sizeof(unsigned int));
		cudaMemcpy(d_colors, colors, colorElementsCount * sizeof(unsigned int), cudaMemcpyHostToDevice);
#endif
	
		cudaMalloc(&d_particleType, numParticles * sizeof(unsigned char));
		cudaMemcpy(d_particleType, particleType, numParticles * sizeof(unsigned char), cudaMemcpyHostToDevice);
}

double* ParticleSystemGPU::getPositions() {
#if (RENDER_ENABLE)
		size_t Size;
		cudaGraphicsMapResources(1, &positionResource, 0);
		cudaGraphicsResourceGetMappedPointer((void**)&d_positions, &Size, positionResource);
#endif
	
		int numBytes = p_numParticles * 4 * sizeof(double);
		cudaMemcpy(positions, d_positions, numBytes, cudaMemcpyDeviceToHost);

#if (RENDER_ENABLE)
		cudaGraphicsUnmapResources(1, &positionResource, 0);
#endif
		return positions;
}

double* ParticleSystemGPU::getVelocities() {
		int numBytes = p_numParticles * 3 * sizeof(double);
		cudaMemcpy(velocities, d_velocities, numBytes, cudaMemcpyDeviceToHost);
		return velocities;
}

unsigned int* ParticleSystemGPU::getColors() {
#if (RENDER_ENABLE)
		size_t Size;
		cudaGraphicsMapResources(1, &colorResource, 0);
		cudaGraphicsResourceGetMappedPointer((void**)&d_colors, &Size, colorResource);
#endif
	
		int numBytes = p_numParticles * 3 * sizeof(unsigned int);
		cudaMemcpy(colors, d_colors, numBytes, cudaMemcpyDeviceToHost);

#if (RENDER_ENABLE)
		cudaGraphicsUnmapResources(1, &colorResource, 0);
#endif
	return colors;
}



void ParticleSystemGPU::update(double timeDelta) {
#if (RENDER_ENABLE)
		size_t Size;
		cudaGraphicsMapResources(1, &positionResource, 0);
		cudaGraphicsResourceGetMappedPointer((void**)&d_positions, &Size, positionResource);
#endif
		update_naive<<<gridSize, blockSize>>>(timeDelta, p_numParticles, d_positions, d_velocities, d_particleType);
	
		//std::cout << cudaGetErrorString(cudaGetLastError()) << std::endl;

		cudaError_t cudaStatusFlag = cudaGetLastError();
		if (cudaStatusFlag != cudaSuccess) {
			std::cerr << "Kernel failed: " << cudaGetErrorString(cudaStatusFlag) << std::endl;

			// Do cudaFree here

			//return false;

			// Should probably make this function return a boolean which indicates success
			// Stop sim if error encountered
		}

		cudaEventRecord(event);

		cudaEventSynchronize(event);

#if (RENDER_ENABLE)
		cudaGraphicsUnmapResources(1, &positionResource, 0);
#endif
}



void ParticleSystemGPU::writecurpostofile(char* file) {
		getPositions();
		std::ofstream outfile(file);

		if (outfile.is_open()) {
			for (int i = 0; i < p_numParticles; i++) {
				outfile << positions[i * 4] << " ";
				outfile << positions[i * 4 + 1] << " ";
				outfile << positions[i * 4 + 2] << " ";
				outfile << positions[i * 4 + 3] << "\n";
			}
		}
		else {
			std::cerr << "Unable to open file: " << file << std::endl;
		}
}

	
void ParticleSystemGPU::display() {
#if (RENDER_ENABLE)
		//Positions are already updated since we work directly on the data!

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		shaderProgram->Activate();

		glDrawArrays(GL_POINTS, 0, p_numParticles);
#endif
}

ParticleSystemGPU::~ParticleSystemGPU() {
	p_numParticles = 0;
	delete[] positions;
	delete[] colors;
	delete[] velocities;
	delete[] particleType;

	//VBO will handle positions and colors buffers if we rendered.
	cudaFree(d_velocities);
	cudaFree(d_particleType);

	cudaEventDestroy(event);
#if (RENDER_ENABLE)
		delete shaderProgram;
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &positionBuffer);
		glDeleteBuffers(1, &colorBuffer);
#else
		cudaFree(d_positions);
		cudaFree(d_colors);
#endif
	
}