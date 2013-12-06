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

extern double colorPower;
extern int frameCount;
extern pthread_mutex_t lock1, lock2;

extern int amountYDarken;
extern int allowEECM;
extern double amountGamma;

#endif /* FFPLAY_H_ */
