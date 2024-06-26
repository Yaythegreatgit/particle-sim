#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H

// Visualization Includes
#include <glad/glad.h>

//Program Includes
#include <common.h>
#include <shaderClass.h>

class ParticleSystem {
public:

	virtual ~ParticleSystem() {}

	virtual float* getPositions() = 0;

	virtual unsigned int* getColors() = 0;

	virtual void update(float timeDelta) = 0;

	virtual void flip() = 0;

	virtual void writecurpostofile(char* file, int steps, float milliseconds) = 0;
	
	virtual void display() = 0;

protected:
	
	int p_numParticles;

	int numProtons;
	int numNeutrons;
	int numElectrons;

	// Particle Data
	float* positions; // 1D Array containing spacial data of each particle (positionElementsCount * numParticles)
	float* positions2; //Used for double buffering

	float* src; //Will always point to either positions or positions2
	float* dst; //Will always point to either positions or positions2

	float* velocities; // 1D Array containing velocity data of each particle (velocityElementsCount * numParticles)
	unsigned int* colors; // 1D Array containing RGB data of each particle (colorElementsCount * numParticles)
	unsigned char* particleType; // 1D Array which denotes particle type (0 = Electron; 1 = Proton, 2 = Neutron)

#if (RENDER_ENABLE)
	//Shader buffers
	GLuint VAO;
	GLuint positionBuffer;
	GLuint colorBuffer;

	Shader* shaderProgram;
#endif
};

#endif