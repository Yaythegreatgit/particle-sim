#include <particleSystemCpu.h>

ParticleSystemCPU::ParticleSystemCPU(int numParticles, int initMethod, int seed) {
		p_numParticles = numParticles;

		// Initialize Positions array
		int positionElementsCount = 4 * numParticles;
		positions = new float[positionElementsCount];

		// Initialize Colors array
		int colorElementsCount = 3 * numParticles;
		colors = new unsigned int[colorElementsCount];

		int velocityElementsCount = 3 * numParticles;
		velocities = new float[velocityElementsCount];

		// Initialize Particle Type array
		particleType = new unsigned char[numParticles];


		// Circular initialization
		if (initMethod == 0) {
			for (unsigned int i = 0; i < numParticles; i++) {
				
				float theta = (float)((numParticles - 1 - i) / (float)numParticles * 2.0 * 3.1415); // Ensure floating-point division
				int stride, pos_offset, col_offset;
				if (STORAGE_TYPE && !RENDER_ENABLE) {
					stride = numParticles;
					pos_offset = 1;
					col_offset = 1;
				}
				else {
					stride = 1;
					pos_offset = 4;
					col_offset = 3;
				}
				positions[i * pos_offset] = (float)cos(theta);
				positions[i * pos_offset + stride] = (float)sin(theta);
				positions[i * pos_offset + 2*stride] = 1.0f;
				positions[i * pos_offset + 3*stride] = 1.0f; // This will always stay as 1, it will be used for mapping 3D to 2D space

				colors[i * col_offset] = i % 255;
				colors[i * col_offset + stride] = 255 - (i % 255);
				colors[i * col_offset + 2*stride] = 55;
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
				int stride, pos_offset, vel_offset;
				if (STORAGE_TYPE && !RENDER_ENABLE) {
					stride = numParticles;
					pos_offset = 1;
					vel_offset = 1;
				}
				else {
					stride = 1;
					pos_offset = 4;
					vel_offset = 3;
				}
				// Randomly initialize position in range [-1,1)
				positions[i * pos_offset] = ((float)(rand() % 2000) - 1000.0) / 1000.0;
				positions[i * pos_offset + stride] = ((float)(rand() % 2000) - 1000.0) / 1000.0;
				positions[i * pos_offset + 2 * stride] = ((float)(rand() % 2000) - 1000.0) / 1000.0;
				positions[i * pos_offset + 3 * stride] = 1.0f; // This will always stay as 1, it will be used for mapping 3D to 2D space
			
				// Randomly initializes velocity in range [-250000,250000)
				velocities[i * vel_offset] = ((float)(rand() % 500) - 250.0) * 1000.0;
				velocities[i * vel_offset + stride] = ((float)(rand() % 500) - 250.0) * 1000.0;
				velocities[i * vel_offset + 2 * stride] = ((float)(rand() % 500) - 250.0) * 1000.0;

				// Generates random number (either 0, 1, 2) from uniform dist
				particleType[i] = rand() % 3;
				//particleType[i] = 2;

				// Sets color based on particle type
				if (particleType[i] == 0) { // If Electron
						colors[i * vel_offset] = ELECTRON_COLOR[0];
						colors[i * vel_offset + stride] = ELECTRON_COLOR[1];
						colors[i * vel_offset + 2 * stride] = ELECTRON_COLOR[2];
				}
				else if (particleType[i] == 1) { // If Proton
						colors[i * vel_offset] = PROTON_COLOR[0];
						colors[i * vel_offset + stride] = PROTON_COLOR[1];
						colors[i * vel_offset + 2 * stride] = PROTON_COLOR[2];
				}
				else {
						colors[i * vel_offset] = NEUTRON_COLOR[0]; //Else neutron
						colors[i * vel_offset + stride] = NEUTRON_COLOR[1];
						colors[i * vel_offset + 2 * stride] = NEUTRON_COLOR[2];
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
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * numParticles, positions, GL_STREAM_DRAW);

		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(unsigned int) * 3 * numParticles, colors, GL_STATIC_DRAW);


		glVertexAttribIPointer(1, 3, GL_UNSIGNED_INT, 0, (void*)0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		shaderProgram = new Shader();
#endif
}

float* ParticleSystemCPU::getPositions(void) {
	return positions;
}

float* ParticleSystemCPU::getVelocities(void) {
	return velocities;
}

unsigned int* ParticleSystemCPU::getColors(void) {
	return colors;
}

float square(float val) {
	return pow(val, 2);
}
#if (STORAGE_TYPE && !RENDER_ENABLE)
void ParticleSystemCPU::update(float timeDelta) {
	for (int i = 0; i < p_numParticles; i++) {
		//Update velocities
		int part_type = particleType[i];
		float force_x = 0.0;
		float force_y = 0.0;
		float force_z = 0.0;
		for (int j = 0; j < p_numParticles; j++) {
			float dist_x = positions[i] - positions[j];
			float dist_y = positions[i + p_numParticles] - positions[j + p_numParticles];
			float dist_z = positions[i + 2 * p_numParticles] - positions[j + 2 * p_numParticles];

			float dist_square = square(dist_x) + square(dist_y) + square(dist_z);
			float dist = sqrt(dist_square);
			float force = 0.0;
			if (i == j || dist < yukawa_cutoff) {
				continue;
			}

			//Natural Coloumb force
			force += (float)coulomb_scalar / dist_square * charges[part_type] * charges[particleType[j]];

			//Strong Forces
			//P-N close attraction N-N close attraction 
			if (part_type != 0 && particleType[j] != 0) {
				force += yukawa_scalar * exp(-dist / yukawa_radius) / dist;
			}

			force_x += force * dist_x / dist;
			force_y += force * dist_y / dist;
			force_z += force * dist_z / dist;


		}

		//Update velocities 
		velocities[i] += force_x * inv_masses[part_type] * timeDelta;
		velocities[i + p_numParticles] += force_y * inv_masses[part_type] * timeDelta;
		velocities[i + 2 * p_numParticles] += force_z * inv_masses[part_type] * timeDelta;

		//Update positions from velocities
		positions[i] += velocities[i] * timeDelta;
		if (abs(positions[i]) > 1) {
			velocities[i] = -1 * velocities[i];
		}
		positions[i + p_numParticles] += velocities[i + p_numParticles] * timeDelta;
		if (abs(positions[i + p_numParticles]) > 1) {
			velocities[i + p_numParticles] = -1 * velocities[i + p_numParticles];
		}
		positions[i + 2 * p_numParticles] += velocities[i + 2 * p_numParticles] * timeDelta;
		if (abs(positions[i + 2 * p_numParticles]) > 1) {
			velocities[i + 2 * p_numParticles] = -1 * velocities[i + 2 * p_numParticles];
		}
	}
}
#else
void ParticleSystemCPU::update(float timeDelta) {
	for (int i = 0; i < p_numParticles; i++) {
		//Update velocities
		int part_type = particleType[i];
		float force_x = 0.0;
		float force_y = 0.0;
		float force_z = 0.0;
		for (int j = 0; j < p_numParticles; j++) {
			float dist_x = positions[i*4] - positions[j*4];
			float dist_y = positions[i*4 + 1] - positions[j*4 + 1];
			float dist_z = positions[i*4 + 2] - positions[j*4 + 2];

			float dist_square = square(dist_x) + square(dist_y) + square(dist_z);
			float dist = sqrt(dist_square);
			float force = 0.0;
			if (i == j || dist < yukawa_cutoff) {
				continue;
			}
			
			//Natural Coloumb force
			force += (float) coulomb_scalar / dist_square * charges[part_type] * charges[particleType[j]];

			//Strong Forces
			//P-N close attraction N-N close attraction 
			if (part_type != 0 && particleType[j] != 0) {
				force += yukawa_scalar * exp(-dist / yukawa_radius) / dist;	
			}

			force_x += force * dist_x / dist;
			force_y += force * dist_y / dist;
			force_z += force * dist_z / dist;


		}

		//Update velocities 
		velocities[i*3] += force_x * inv_masses[part_type] * timeDelta;
		velocities[i*3 + 1] += force_y * inv_masses[part_type] * timeDelta;
		velocities[i*3 + 2] += force_z * inv_masses[part_type] * timeDelta;

		//Update positions from velocities
		positions[i * 4] += velocities[i * 3] * timeDelta;
		if (abs(positions[i * 4]) > 1) {
			velocities[i * 3] = -1 * velocities[i * 3];
		}
		positions[i * 4 + 1] += velocities[i * 3 + 1] * timeDelta;
		if (abs(positions[i * 4 + 1]) > 1) {
			velocities[i * 3 + 1] = -1 * velocities[i * 3 + 1];
		}
		positions[i * 4 + 2] += velocities[i * 3 + 2] * timeDelta;
		if (abs(positions[i * 4 + 2]) > 1) {
			velocities[i * 3 + 2] = -1 * velocities[i * 3 + 2];
		}
	}
}
#endif

void ParticleSystemCPU::writecurpostofile(char* file, int steps, float milliseconds) {
	std::ofstream outfile(file);

	if (outfile.is_open()) {
		outfile << "particles:" << p_numParticles << " iterations:" << steps << " timing:" << milliseconds << " structure:" << STORAGE_TYPE << "\n";
		for (int i = 0; i < p_numParticles; i++) {
#if (STORAGE_TYPE && !RENDER_ENABLE)
			outfile << positions[i] << " ";
			outfile << positions[i + p_numParticles] << " ";
			outfile << positions[i + 2 * p_numParticles] << " ";
			outfile << positions[i + 3 * p_numParticles] << "\n";
#else
			outfile << positions[i * 4] << " ";
			outfile << positions[i * 4 + 1] << " ";
			outfile << positions[i * 4 + 2] << " ";
			outfile << positions[i * 4 + 3] << "\n";
#endif
		}
	}
	else {
		std::cerr << "Unable to open file: " << file << std::endl;
	}
}

void ParticleSystemCPU::display() {
	//If render wasn't specified this function does nothing. The code shouldn't allow it but it's a good check
#if (RENDER_ENABLE)
		//Update the positions
		glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * p_numParticles, positions, GL_STREAM_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		shaderProgram->Activate();

		glPointSize(5.0);

		glDrawArrays(GL_POINTS, 0, p_numParticles);
#endif
}

ParticleSystemCPU::~ParticleSystemCPU() {
	p_numParticles = 0;
	delete[] positions;
	delete[] colors;
	delete[] velocities;
	delete[] particleType;

#if (RENDER_ENABLE)
		delete shaderProgram;
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &positionBuffer);
		glDeleteBuffers(1, &colorBuffer);
#endif
}