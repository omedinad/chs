#ifndef GET_GOALS_RUBI_CSV_H_INCLUDED
#define GET_GOALS_RUBI_CSV_H_INCLUDED

#include <stdio.h>
#include <stdlib.h> /* atoi */
#include <string.h> /* memcpy */
#include <errno.h>

typedef struct {
	int min, sec, dec;
} Pace;

int validate_int_HR(int );
int validate_str_HR(char * );
int validate_str_time(const char * , Pace *);
char * pace_to_str( Pace * );
int get_field_number(const char * , const char * , int );
void float_to_Pace(Pace * , float );
float pace_to_float(const Pace * );
int computeValues(int *, float * , int*, Pace * , int*, char * , const size_t * , const int * , const Pace *  );

#endif