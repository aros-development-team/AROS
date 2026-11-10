/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: the card's sensors, as the hidd asks the driver for them
          (nouveau_aros_sensor() in drm/nouveau/nouveau_drm.c).
*/
#ifndef NOUVEAU_AROS_SENSORS_H
#define NOUVEAU_AROS_SENSORS_H

enum
{
    /* thousandths of a degree Celsius */
    NVSENSOR_TEMP,              /* the GPU's temperature */
    NVSENSOR_TEMP_FANBOOST,     /* the fan is driven harder from here */
    NVSENSOR_TEMP_DOWNCLOCK,    /* the GPU is slowed down from here */
    NVSENSOR_TEMP_CRITICAL,
    NVSENSOR_TEMP_SHUTDOWN,     /* the card switches itself off */
    /* the fan */
    NVSENSOR_FAN_RPM,
    NVSENSOR_FAN_DUTY,          /* percent */
    NVSENSOR_FAN_DUTY_MIN,
    NVSENSOR_FAN_DUTY_MAX,
    /* millivolts */
    NVSENSOR_VOLT,              /* the GPU core supply */
    NVSENSOR_VOLT_MIN,
    NVSENSOR_VOLT_MAX,
    /* milliwatts */
    NVSENSOR_POWER,             /* what the board draws */
    NVSENSOR_POWER_MAX,
    NVSENSOR_POWER_CRITICAL,

    NVSENSOR_COUNT
};

/* 0 and the reading in *value, or a negative errno when the card has no such sensor */
int nouveau_sensor(int what, long *value);

#endif /* NOUVEAU_AROS_SENSORS_H */
