/* Enhanced, this for using - sherin999 */


/* Simple implementation of Biquad filters -- Tom St Denis
 *
 * Based on the work

 Cookbook formulae for audio EQ biquad filter coefficients
 ---------------------------------------------------------
 by Robert Bristow-Johnson, pbjrbj@viconet.com  a.k.a. robert@audioheads.com

 * Available on the web at

http://www.smartelectronix.com/musicdsp/text/filters005.txt

 * Enjoy.
 *
 * This work is hereby placed in the public domain for all purposes, whether
 * commercial, free [as in speech] or educational, etc.  Use the code and please
 * give me credit if you wish.
 *
 * Tom St Denis -- http://tomstdenis.home.dhs.org
 */

/* this would be biquad.h */
#include <math.h>
#include <stdlib.h>
#include <stdio.h>


#ifndef M_LN2
#define M_LN2	   0.69314718055994530942
#endif

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

/* whatever sample type you want */
typedef double smp_type;

/* this holds the data required to update samples thru a filter */
typedef struct {
	smp_type a0, a1, a2, a3, a4;
	smp_type x1, x2, y1, y2;
}
biquad;

extern smp_type BiQuad(smp_type sample, biquad * b);
extern biquad *BiQuad_new(int type, smp_type dbGain, /* gain of filter */
		smp_type freq,             /* center frequency */
		smp_type srate,            /* sampling rate */
		smp_type bandwidth);       /* bandwidth in octaves */

/* filter types */
enum {
	LPF, /* low pass filter */
	HPF, /* High pass filter */
	BPF, /* band pass filter */
	NOTCH, /* Notch Filter */
	PEQ, /* Peaking band EQ filter */
	LSH, /* Low shelf filter */
	HSH /* High shelf filter */
};

/* Below this would be biquad.c */
/* Computes a BiQuad filter on a sample */
smp_type BiQuad(smp_type sample, biquad * b)
{
	smp_type result;

	/* compute result */
	result = b->a0 * sample + b->a1 * b->x1 + b->a2 * b->x2 -
		b->a3 * b->y1 - b->a4 * b->y2;

	//printf("%f <==(%f *%f) + (%f * %f) + (%f * %f) - (%f * %f) - (%f * %f)\n", result, b->a0, sample, b->a1, b->x1, b->a2, b->x2, b->a3, b->y1, b->a4, b->y2);
	/* shift x1 to x2, sample to x1 */
	b->x2 = b->x1;
	b->x1 = sample;

	/* shift y1 to y2, result to y1 */
	b->y2 = b->y1;
	b->y1 = result;

	return result;
}

/* sets up a BiQuad Filter */
biquad *BiQuad_new(int type, smp_type dbGain, smp_type freq,
		smp_type srate, smp_type bandwidth)
{
	biquad *b;
	smp_type A, omega, sn, cs, alpha, beta;
	smp_type a0, a1, a2, b0, b1, b2;

	b = malloc(sizeof(biquad));
	if (b == NULL)
		return NULL;

	/* setup variables */
	A = pow(10, dbGain /40);
	omega = 2 * M_PI * freq /srate;
	sn = sin(omega);
	cs = cos(omega);
	alpha = sn * sinh(M_LN2 /2 * bandwidth * omega /sn);
	beta = sqrt(A + A);

	switch (type) {
		case LPF:
			b0 = (1 - cs) /2;
			b1 = 1 - cs;
			b2 = (1 - cs) /2;
			a0 = 1 + alpha;
			a1 = -2 * cs;
			a2 = 1 - alpha;
			break;
		case HPF:
			b0 = (1 + cs) /2;
			b1 = -(1 + cs);
			b2 = (1 + cs) /2;
			a0 = 1 + alpha;
			a1 = -2 * cs;
			a2 = 1 - alpha;
			break;
		case BPF:
			b0 = alpha;
			b1 = 0;
			b2 = -alpha;
			a0 = 1 + alpha;
			a1 = -2 * cs;
			a2 = 1 - alpha;
			break;
		case NOTCH:
			b0 = 1;
			b1 = -2 * cs;
			b2 = 1;
			a0 = 1 + alpha;
			a1 = -2 * cs;
			a2 = 1 - alpha;
			break;
		case PEQ:
			b0 = 1 + (alpha * A);
			b1 = -2 * cs;
			b2 = 1 - (alpha * A);
			a0 = 1 + (alpha /A);
			a1 = -2 * cs;
			a2 = 1 - (alpha /A);
			break;
		case LSH:
			b0 = A * ((A + 1) - (A - 1) * cs + beta * sn);
			b1 = 2 * A * ((A - 1) - (A + 1) * cs);
			b2 = A * ((A + 1) - (A - 1) * cs - beta * sn);
			a0 = (A + 1) + (A - 1) * cs + beta * sn;
			a1 = -2 * ((A - 1) + (A + 1) * cs);
			a2 = (A + 1) + (A - 1) * cs - beta * sn;
			break;
		case HSH:
			b0 = A * ((A + 1) + (A - 1) * cs + beta * sn);
			b1 = -2 * A * ((A - 1) + (A + 1) * cs);
			b2 = A * ((A + 1) + (A - 1) * cs - beta * sn);
			a0 = (A + 1) - (A - 1) * cs + beta * sn;
			a1 = 2 * ((A - 1) - (A + 1) * cs);
			a2 = (A + 1) - (A - 1) * cs - beta * sn;
			break;
		default:
			free(b);
			return NULL;
	}

	/* precompute the coefficients */
	b->a0 = b0 /a0;
	b->a1 = b1 /a0;
	b->a2 = b2 /a0;
	b->a3 = a1 /a0;
	b->a4 = a2 /a0;
	printf("b0: %f b1: %f b2: %f a1: %f a2: %f\n", b0, b1, b2, a1, a2);

	/* zero initial samples */
	b->x1 = b->x2 = 0;
	b->y1 = b->y2 = 0;

	return b;
}
/* crc==3062280887, version==4, Sat Jul  7 00:03:23 2001 */

// WAVE PCM soundfile format (you can find more in https://ccrma.stanford.edu/courses/422/projects/WaveFormat/ )
typedef struct 
{
	char chunk_id[4];
	int chunk_size;
	char format[4];
	char subchunk1_id[4];
	int subchunk1_size;
	short int audio_format;
	short int num_channels;
	int sample_rate;			// sample_rate denotes the sampling rate.
	int byte_rate;
	short int block_align;
	short int bits_per_sample;
	char subchunk2_id[4];
	int subchunk2_size;			// subchunk2_size denotes the number of samples.
} wav_header;

#define WSIZE 512
short bufferIn[WSIZE] = {0};
short bufferOut[WSIZE] = {0};

main()
{
	unsigned int len = 1024;
	FILE * fp_in, *fp_out;
	int i =0, nb;
	biquad b;
	biquad * pb = &b;
	wav_header wav_header_buf;

	pb = BiQuad_new(LPF, 0, 100, 44100, 0);

	fp_in = fopen("in.wav", "rb");
	fp_out = fopen("out.wav", "wb");


	fread(&wav_header_buf, 1, sizeof(wav_header), fp_in);
	fwrite(&wav_header_buf,1, sizeof(wav_header), fp_out);

	while (!feof(fp_in)){

		
		nb = fread(bufferIn, 2, WSIZE, fp_in);
		printf("nb: %d\n", nb);
  
		for(i = 0; i < WSIZE; i++)
		{
			bufferOut[i] = 32768.0 * BiQuad(bufferIn[i]/ 32768.0 , pb);
		}
		fwrite(bufferOut,2,nb,fp_out);			// Writing read data into output file
	}

}
