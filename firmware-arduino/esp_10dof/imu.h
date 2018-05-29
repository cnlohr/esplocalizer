/*
 * Copyright (C) 2016 XXXXXXXX
 *
 * This file is subject to the terms and conditions defined in file 'LICENSE.txt', 
 * which is part of this source code package.
 */

/**
 * @file    imu.h
 * @brief   High level imu functions
 */


#ifndef IMU_H
#define IMU_H

#include "Arduino.h"

// Earth's magnetic field varies by location. Add or subtract 
// a declination to get a more accurate heading. Calculate 
// your's here:
// http://www.ngdc.noaa.gov/geomag-web/#declination
#define DECLINATION -14.55 // Declination (degrees) in Edmoton, AB, Canada.

float imu_gx, imu_gy, imu_gz;
float imu_ax, imu_ay, imu_az;
float imu_mx, imu_my, imu_mz;

void getGyro(bool verbose = 0);
void getAccel(bool verbose = 0);
void getMag(bool verbose = 0);
void getSimpleAttitude(float ax, float ay, float az, float mx, float my, float mz);

#endif  /* IMU_H */