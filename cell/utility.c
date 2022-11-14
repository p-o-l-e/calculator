/////////////////////////////////////////////////////////////////////////////////////////
// Utilities
// V.0.3.8 2022-07-22
// MIT License
// Copyright (c) 2022 unmanned
/////////////////////////////////////////////////////////////////////////////////////////
#include "utility.h"


void svflto_clr(ltosvf* o)
{
    o->ic1eq = 0.0f;
    o->ic2eq = 0.0f;
}

void svflto_init(ltosvf* o, int cutoff, float Q)
{
    // o->g = tanf(PI * cutoff / SAMPLE_RATE);
    o->g = o->ftable[cutoff];
    o->k = 1.0f/Q;
    o->a = 1.0f/(1.0f + o->g*(o->g + o->k));
    o->b = o->g * o->a;
}

void svflto_process(ltosvf* o, float in)
{
    float va = o->a*o->ic1eq + o->b*(in - o->ic2eq);
    float vb = o->ic2eq + o->g*va;
    o->ic1eq = 2.0f*va - o->ic1eq;
    o->ic2eq = 2.0f*vb - o->ic2eq;
    o->low   = vb;
    o->band  = va;
    o->high  = in - o->k*va - vb;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

void psf_init(psf* o, int time)
{
    o->a = time;
    o->b = ONE24 - o->a;
    o->o = 0;
}

int psf_process(psf* o, int in)
{
    o->o = mul_fr32(in, o->b, SFT) + mul_fr32(o->o, o->a, SFT);
    return o->o;
}


void ef_init(ef* o, float aMs, float rMs)
{
    o->envelope = 0.0f;
    o->a = powf( 0.01, 1.0 / ( aMs * SAMPLE_RATE * 0.001 ) );
    o->r = powf( 0.01, 1.0 / ( rMs * SAMPLE_RATE * 0.001 ) );
}

void ef_process(ef* o, float in)
{
    float f = fabs(in);
    if (f > o->envelope) o->envelope = o->a * ( o->envelope - f ) + f;
    else                 o->envelope = o->r * ( o->envelope - f ) + f;
}

void limiter_init(limiter* o, float aMs, float rMs, float threshold)
{
    ef_init(&o->e, aMs, rMs);
    o->threshold = threshold;
}


float limit(limiter* o, float in)
{
    float out = in;
    ef_process(&o->e, in);
    if(o->e.envelope > o->threshold)
    {
        out /= expf(o->e.envelope - o->threshold);
    }
    return out;
}

int gate(gator* o, int in, int width)
{
    if(abs(in - o->eax) > width)
    {
        o->eax = in;
        return o->eax;
    }
    else
    {
        return o->eax;
    }
}


int rand_in_range(int l, int r) 
{ 
    return (rand() % (r - l + 1)) + l;
}



void snh_init(snh* o)
{
    o->t = 0;
    o->value = 0.0f;
}

int snh_process(snh* o, int input, int time)
{
    if (o->t>time)
    {
        o->t = 0;
        o->value = input;
    }
    o->t++;
    return o->value;
}
