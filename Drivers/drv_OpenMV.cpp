#include "drv_Uart3.hpp"
#include "Basic.hpp"
#include "FreeRTOS.h"
#include "task.h"
#include "SensorsBackend.hpp"
#include "MeasurementSystem.hpp"
#include "semphr.h"
#include "drv_OpenMV.hpp"

static const unsigned char packet_ID[4] = {'A', 'A', 'E', 'E'};
SemaphoreHandle_t openMVMutex;
_OpenMV OpenMV;
int openmvflag = 0;

static void _OpenMV_Server(void *pvParameters)
{
    /*状态机*/

    unsigned char rc_counter = 0;
    unsigned char x_counter = 0;
    unsigned char y_counter = 0;
    unsigned char x_enter = 0;
    unsigned char y_enter = 0;
    unsigned char buf_x[10];
    unsigned char buf_y[10]; 
    unsigned char nagtiveFlag = 0;
    unsigned char digCount = 0;
    unsigned char decimal = 0;
    double a = 1;
    int j = 0;
    double num = 0;

    openMVMutex = xSemaphoreCreateMutex();
    /*状态机*/

    while (1)
    {
        uint8_t rdata;
        if (Read_Uart3(&rdata, 1, 2, 0.5))
        {
            if (rc_counter < 2)
            {
                //接收包头
                if (rdata != packet_ID[rc_counter])
                    rc_counter = 0;
                else
                {
                    ++rc_counter;
                }
            }
            if (rc_counter > 1 && rc_counter < 4)
            {
                //接收包尾
                if (rdata != packet_ID[rc_counter])
                    rc_counter = 2;
                else
                {
                    ++rc_counter;
                }
            }
            if (rc_counter == 4)
            {
                //读取完整包结束进行数据处理
                rc_counter = 0;
                x_enter = 0;
                y_enter = 0;
                if (x_counter != 0)
                {
                    for (unsigned char i = 0; i < x_counter; i++)
                    {
                        if (i == 0 && buf_x[i] == '-')
                        {
                            nagtiveFlag = 1;
                        }
                        else if (buf_x[i] >= '0' && buf_x[i] <= '9' && decimal == 0)
                        {
                            num = num * 10 + (buf_x[i] - '0');
                            ++digCount;
                        }
                        else if (buf_x[i] >= '0' && buf_x[i] <= '9' && decimal == 1)
                        {
                            a = a * 0.1;
                            num = num + (buf_x[i] - '0') * a;
                            ++digCount;
                        }
                        if (buf_x[i] == '.')
                        {
                            decimal = 1;
                            a = 1;
                        }
                    }

                    if (nagtiveFlag)
                    {
                        if (xSemaphoreTake(openMVMutex, portMAX_DELAY))
                        {
                            OpenMV.dev_x = -num;
                            xSemaphoreGive(openMVMutex);
                        }
                    }
                    else
                    {
                        if (xSemaphoreTake(openMVMutex, portMAX_DELAY))
                        {
                            OpenMV.dev_x = num;
                            xSemaphoreGive(openMVMutex);
                        }
                    }
                    a = 1;
                    num = 0;
                    digCount = 0;
                    decimal = 0;
                    nagtiveFlag = 0;
                }

                if (y_counter != 0)
                {
                    for (unsigned char i = 0; i < y_counter; i++)
                    {
                        if (i == 0 && buf_y[i] == '-')
                        {
                            nagtiveFlag = 1;
                        }
                        else if (buf_y[i] >= '0' && buf_y[i] <= '9' && decimal == 0)
                        {
                            num = num * 10 + (buf_y[i] - '0');
                            ++digCount;
                        }
                        else if (buf_y[i] >= '0' && buf_y[i] <= '9' && decimal == 1)
                        {
                            a = a * 0.1;
                            num = num + (buf_y[i] - '0') * a;
                            ++digCount;
                        }
                        if (buf_y[i] == '.')
                        {
                            decimal = 1;
                            a = 1;
                        }
                    }
                    if (nagtiveFlag)
                    {
                        if (xSemaphoreTake(openMVMutex, portMAX_DELAY))
                        {
                            OpenMV.dev_y = -num;
                            xSemaphoreGive(openMVMutex);
                        }
                    }
                    else
                    {
                        if (xSemaphoreTake(openMVMutex, portMAX_DELAY))
                        {
                            OpenMV.dev_y = num;
                            xSemaphoreGive(openMVMutex);
                        }
                    }
                    a = 1;
                    num = 0;
                    digCount = 0;
                    decimal = 0;
                    nagtiveFlag = 0;
                }
            }
            if (x_enter)
            { //读取X的值
                buf_x[x_counter] = rdata;
                ++x_counter;
            }
            if (y_enter)
            { //读取Y的值
                buf_y[y_counter] = rdata;
                ++y_counter;
            }
            if (rdata == 'X' && rc_counter == 2)
            { //判断进入X值的读取
                x_enter = 1;
                y_enter = 0;
                x_counter = 0;
            }
            if (rdata == 'Y' && rc_counter == 2)
            { //判断进入Y值的读取
                x_enter = 0;
                y_enter = 1;
                y_counter = 0;
            }
        }
        else
        {
             if (xSemaphoreTake(openMVMutex, portMAX_DELAY))
            {
                OpenMV.dev_y = 0;
                OpenMV.dev_x = 0;
                xSemaphoreGive(openMVMutex);
            }
        }
        
    }
}
void init_drv_OpenMV()
{
    //波特率115200
    SetBaudRate_Uart3(115200, 2, 2);
    xTaskCreate(_OpenMV_Server, "OpenMV", 1024, NULL, SysPriority_UserTask, NULL);
}