#include <stdio.h>
#include <math.h>
#include <sekai/common.h>
float interp_linear(float* x,float* y,int nx,float ref)
{
    int i;
    for(i=0; i<nx-1; i++)
    {
        if(ref>=x[i] && ref <= x[i+1])
        {
            float x1=x[i];
            float x2=x[i+1];
            float tmp = (ref-x1)/(x2-x1);
            return y[i]*(1-tmp)+y[i+1]*tmp;
        }
    }
    fprintf(stderr,"INTERP_LINEAR: out of range\n");
    return NAN;
}

double interp_linear(double* x,double* y,int nx,double ref)
{
    int i;
    for(i=0; i<nx-1; i++)
    {
        if(ref>=x[i] && ref <= x[i+1])
        {
            double x1=x[i];
            double x2=x[i+1];
            double tmp = (ref-x1)/(x2-x1);
            return y[i]*(1-tmp)+y[i+1]*tmp;
        }
    }
    fprintf(stderr,"INTERP_LINEAR: out of range\n");
    return NAN;
}
