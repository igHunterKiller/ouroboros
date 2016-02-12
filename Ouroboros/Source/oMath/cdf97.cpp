// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <malloc.h>

// http://www.embl.de/~gpau/misc/dwt97.c
// Gregoire Pau

namespace ouro {

// $(CitedCodeBegin)

void cdf97fwd(float* values, unsigned int num_values)
{
	float a;
	unsigned int i;

	// Predict 1
	a=-1.586134342f;
	for (i=1;i<num_values-2;i+=2) {
		values[i]+=a*(values[i-1]+values[i+1]);
	} 
	values[num_values-1]+=2*a*values[num_values-2];

	// Update 1
	a=-0.05298011854f;
	for (i=2;i<num_values;i+=2) {
		values[i]+=a*(values[i-1]+values[i+1]);
	}
	values[0]+=2.0f*a*values[1];

	// Predict 2
	a=0.8829110762f;
	for (i=1;i<num_values-2;i+=2) {
		values[i]+=a*(values[i-1]+values[i+1]);
	}
	values[num_values-1]+=2*a*values[num_values-2];

	// Update 2
	a=0.4435068522f;
	for (i=2;i<num_values;i+=2) {
		values[i]+=a*(values[i-1]+values[i+1]);
	}
	values[0]+=2*a*values[1];

	// Scale
	a=1.0f/1.149604398f;
	for (i=0;i<num_values;i++) {
		if (i%2) values[i]*=a;
		else values[i]/=a;
	}

	// Pack
	float* TempBank = (float*)alloca(sizeof(float) * num_values);

	for (size_t i = 0; i < num_values; i++) 
	{
		if (i%2==0) 
			TempBank[i/2]= values[i];
		else 
			TempBank[num_values/2+i/2] = values[i];
	}

	for (size_t i = 0; i < num_values; i++) 
		values[i]=TempBank[i];
}

void cdf97inv(float* values, unsigned int num_values) 
{
	float a;
	unsigned int i;

	// Unpack
	float* TempBank = (float*)alloca(sizeof(float) * num_values);
	for (i=0;i<num_values/2;i++) {
		TempBank[i*2]=values[i];
		TempBank[i*2+1]=values[i+num_values/2];
	}
	for (i=0;i<num_values;i++) values[i]=TempBank[i];

	// Undo scale
	a=1.149604398f;
	for (i=0;i<num_values;i++) {
		if (i%2) values[i]*=a;
		else values[i]/=a;
	}

	// Undo update 2
	a=-0.4435068522f;
	for (i=2;i<num_values;i+=2) {
		values[i]+=a*(values[i-1]+values[i+1]);
	}
	values[0]+=2*a*values[1];

	// Undo predict 2
	a=-0.8829110762f;
	for (i=1;i<num_values-2;i+=2) {
		values[i]+=a*(values[i-1]+values[i+1]);
	}
	values[num_values-1]+=2*a*values[num_values-2];

	// Undo update 1
	a=0.05298011854f;
	for (i=2;i<num_values;i+=2) {
		values[i]+=a*(values[i-1]+values[i+1]);
	}
	values[0]+=2*a*values[1];

	// Undo predict 1
	a=1.586134342f;
	for (i=1;i<num_values-2;i+=2) {
		values[i]+=a*(values[i-1]+values[i+1]);
	} 
	values[num_values-1]+=2*a*values[num_values-2];
}

}

