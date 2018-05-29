/*
 * Copyright (C) 2016 XXXXXXXX
 *
 * This file is subject to the terms and conditions defined in file 'LICENSE.txt', 
 * which is part of this source code package.
 */

/**
 * @file    imu.ino
 * @brief   High level imu functions
 */


#include <Wire.h>
#include <SPI.h>
#include "lsm9ds1.h"

void getGyro(bool verbose)
{
  // To read from the gyroscope, you must first call the readGyro() function.
  // When this exits, it'll update the gx, gy, and gz variables with the most current data.
  imu.readGyro();
  
  // Use the calcGyro helper function to convert a raw ADC value to deg/s (º/s)
  imu_gx = imu.calcGyro(imu.gx);
  imu_gy = imu.calcGyro(imu.gy);
  imu_gz = imu.calcGyro(imu.gz);

  if (verbose) {
    // If verbose is true, print data
    Serial.print("Gyro: ");
    Serial.print(imu_gx, 2);
    Serial.print(", ");
    Serial.print(imu_gy, 2);
    Serial.print(", ");
    Serial.print(imu_gz, 2);
    Serial.println(" deg/s");
  }
}

void getAccel(bool verbose)
{
  // To read from the accelerometer, you must first call the readAccel() function.
  // When this exits, it'll update the ax, ay, and az variables with the most current data.
  imu.readAccel();
  
  // Use the calcAccel helper function to convert a raw ADC value to g's
  imu_ax = imu.calcAccel(imu.ax);
  imu_ay = imu.calcAccel(imu.ay);
  imu_az = imu.calcAccel(imu.az);

  if (verbose) {
    // If verbose is true, print data
    Serial.print("Accel: ");
    Serial.print(imu_ax, 2);
    Serial.print(", ");
    Serial.print(imu_ay, 2);
    Serial.print(", ");
    Serial.print(imu_az, 2);
    Serial.println(" g");
  }
}

void getMag(bool verbose)
{
  // To read from the magnetometer, you must first call the readMag() function.
  // When this exits, it'll update the mx, my, and mz variables with the most current data.
  imu.readMag();
  
  // Use the calcMag helper function to convert a raw ADC value to Gauss
  imu_mx = imu.calcMag(imu.mx);
  imu_my = imu.calcMag(imu.my);
  imu_mz = imu.calcMag(imu.mz);

  if (verbose) {
    // If verbose is true, print data
    Serial.print("Accel: ");
    Serial.print(imu_mx, 2);
    Serial.print(", ");
    Serial.print(imu_my, 2);
    Serial.print(", ");
    Serial.print(imu_mz, 2);
    Serial.println(" gauss");
  }
}

// Calculate pitch, roll, and heading.
// Pitch/roll calculations take from this app note:
// http://cache.freescale.com/files/sensors/doc/app_note/AN3461.pdf?fpsp=1
// Heading calculations taken from this app note:
// http://www51.honeywell.com/aero/common/documents/myaerospacecatalog-documents/Defense_Brochures-documents/Magnetic__Literature_Application_notes-documents/AN203_Compass_Heading_Using_Magnetometers.pdf
void getSimpleAttitude(float ax, float ay, float az, float mx, float my, float mz)
{
  float roll = atan2(ay, az);
  float pitch = atan2(-ax, sqrt(ay * ay + az * az));
  
  float heading;
  if (my == 0)
    heading = (mx < 0) ? 180.0 : 0;
  else
    heading = atan2(mx, my);
    
  heading -= DECLINATION * PI / 180;
  
  if (heading > PI) heading -= (2 * PI);
  else if (heading < -PI) heading += (2 * PI);
  else if (heading < 0) heading += 2 * PI;
  
  // Convert everything from radians to degrees:
  heading *= 180.0 / PI;
  pitch *= 180.0 / PI;
  roll  *= 180.0 / PI;
  
  Serial.print("Pitch, Roll: ");
  Serial.print(pitch, 2);
  Serial.print(", ");
  Serial.println(roll, 2);
  Serial.print("Heading: "); Serial.println(heading, 2);
}
