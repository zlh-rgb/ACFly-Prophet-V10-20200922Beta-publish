#pragma once

typedef struct
{
    double dev_x;
    double dev_y;
    uint8_t Rsv; //预留
} __PACKED _OpenMV;

void init_drv_OpenMV();