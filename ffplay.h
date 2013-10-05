/*
 * ffplay.h
 *
 *  Created on: Sep 7, 2013
 *      Author: zuyetawarmatik
 */

#ifndef FFPLAY_H_
#define FFPLAY_H_

#include <pthread.h>

#define COLOR_SPACE_SIZE 16777216
extern unsigned char eecm[COLOR_SPACE_SIZE][3];

extern double rPower[256];
extern double gPower[256];
extern double bPower[256];

extern double origColorPower;
extern double newColorPower;

extern pthread_mutex_t lock1, lock2;

extern int ydarken;
extern int alloweecmMPEG12;

#endif /* FFPLAY_H_ */
