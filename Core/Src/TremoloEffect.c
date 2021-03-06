/*
 * TremoloEffect.c
 *
 *  Created on: May 22, 2022
 *      Author: Armin
 */
#include <stdlib.h>
#include <stdint.h>
#include "TremoloEffect.h"
#include <math.h>

#define PI 3.14159265359

TremoloEffect* tremolo = NULL;

float Tremolo_LFO(float phase, int waveform)
{
	switch(waveform) {
		case Triangle:
			if(phase < 0.25f)
				return 0.5f + 2.0f*phase;
			else if(phase < 0.75f)
				return 1.0f - 2.0f*(phase - 0.25f);
			else
				return 2.0f*(phase-0.75f);
		case Square:
			if(phase < 0.5f)
				return 1.0f;
			else
				return 0.0f;
		case Sine:
		default:
			return 0.5f + 0.5f*sinf(2.0 * PI * phase);
		}
}

void Tremolo_Init(int sampleRate) {
	if(tremolo != NULL) return;

	tremolo = malloc(sizeof(TremoloEffect));
	tremolo->phase = 0;
	tremolo->inverseSampleRate = 1.0f/sampleRate;
	tremolo->depth = 1.0f;
	tremolo->waveform = Triangle;
}

float Tremolo_Process(float in, float depth, float frequency) {
	tremolo->depth = depth;
	tremolo->frequency = 6.0f*frequency;
	float out;
	out = in * (1.0f - tremolo->depth * Tremolo_LFO(tremolo->phase, tremolo->waveform));

	tremolo->phase += tremolo->frequency*tremolo->inverseSampleRate;
	if(tremolo->phase >= 1.0)
		tremolo->phase -= 1.0;

	return out;
}

void Tremolo_Set_Waveform(int waveform) {
	tremolo->waveform = waveform;
}

void Tremolo_Free() {
	if(tremolo != NULL) {
		free(tremolo);
		tremolo = NULL;
	}
}
