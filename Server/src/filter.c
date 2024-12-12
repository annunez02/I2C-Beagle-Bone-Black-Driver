#include "../inc/filter.h"
#include "../inc/sensor_functions.h"
#include "../inc/server_functions.h"


extern int SMA_filter_order;
extern float EWMA_filter_alpha;

extern outData * out_data;

float* SMA_aux_ac_x = NULL;
float* SMA_aux_ac_y = NULL;
float* SMA_aux_ac_z = NULL;
float* SMA_aux_temp = NULL;
float* SMA_aux_gy_x = NULL;
float* SMA_aux_gy_y = NULL;
float* SMA_aux_gy_z = NULL;



int SMA_alloc()
{
    /*
        Pido memoria y lleno de ceros para todos los coefs del filtro
    */

    SMA_aux_ac_x = (float*)calloc(SMA_filter_order, sizeof(float));
    SMA_aux_ac_y = (float*)calloc(SMA_filter_order, sizeof(float));
    SMA_aux_ac_z = (float*)calloc(SMA_filter_order, sizeof(float));
    SMA_aux_temp = (float*)calloc(SMA_filter_order, sizeof(float));
    SMA_aux_gy_x = (float*)calloc(SMA_filter_order, sizeof(float));
    SMA_aux_gy_y = (float*)calloc(SMA_filter_order, sizeof(float));
    SMA_aux_gy_z = (float*)calloc(SMA_filter_order, sizeof(float));

    // Si alguna es NULL entra

    if((!SMA_aux_ac_x || !SMA_aux_ac_y || !SMA_aux_ac_z || !SMA_aux_temp || !SMA_aux_gy_x || !SMA_aux_gy_y || !SMA_aux_gy_z))
    {
        perror(RED"[ERROR] SMA ALLOC: Error al pedir memoria para el filtro SMA"DEFAULT);
        return -1;
    }

    return 0;
}

void SMA_free()
{
    free(SMA_aux_ac_x);
    free(SMA_aux_ac_y);
    free(SMA_aux_ac_z);
    free(SMA_aux_temp);
    free(SMA_aux_gy_x);
    free(SMA_aux_gy_y);
    free(SMA_aux_gy_z);    
}

void SMA_filter()
{
    /*
        Logica del promediador (orden 3):
        x[n]  x[n-1]  x[n-2]
            0   -   -   =   -
            1   0   -   =   -
            1   1   0   =   2/3
            1   1   1   =   1
    */

    float ac_x = 0, ac_y = 0, ac_z = 0, temp = 0, gy_x = 0, gy_y = 0, gy_z = 0;

    // shift reg

    for(int i = SMA_filter_order - 1; i > 0; i--)
    {
        SMA_aux_ac_x[i] = SMA_aux_ac_x[i - 1];
        SMA_aux_ac_y[i] = SMA_aux_ac_y[i - 1];
        SMA_aux_ac_z[i] = SMA_aux_ac_z[i - 1];
        SMA_aux_temp[i] = SMA_aux_temp[i - 1];
        SMA_aux_gy_x[i] = SMA_aux_gy_x[i - 1];
        SMA_aux_gy_y[i] = SMA_aux_gy_y[i - 1];
        SMA_aux_gy_z[i] = SMA_aux_gy_z[i - 1];
    }

    // guardo el dato leido en el 0

    SMA_aux_ac_x[0] = out_data->out_ac_x;
    SMA_aux_ac_y[0] = out_data->out_ac_y;
    SMA_aux_ac_z[0] = out_data->out_ac_z;
    SMA_aux_temp[0] = out_data->out_temp;
    SMA_aux_gy_x[0] = out_data->out_gy_x;
    SMA_aux_gy_y[0] = out_data->out_gy_y;
    SMA_aux_gy_z[0] = out_data->out_gy_z;

    // hago la suma
    for(int i = 0; i < SMA_filter_order; i++)
    {
        ac_x += SMA_aux_ac_x[i];
        ac_y += SMA_aux_ac_y[i];
        ac_z += SMA_aux_ac_z[i];
        temp += SMA_aux_temp[i];
        gy_x += SMA_aux_gy_x[i];
        gy_y += SMA_aux_gy_y[i];
        gy_z += SMA_aux_gy_z[i];
    }
    
    out_data->SMA_out_ac_x = ac_x / SMA_filter_order;
    out_data->SMA_out_ac_y = ac_y / SMA_filter_order;
    out_data->SMA_out_ac_z = ac_z / SMA_filter_order;
    out_data->SMA_out_temp = temp / SMA_filter_order;
    out_data->SMA_out_gy_x = gy_x / SMA_filter_order;
    out_data->SMA_out_gy_y = gy_y / SMA_filter_order;
    out_data->SMA_out_gy_z = gy_z / SMA_filter_order;
}

void EWMA_filter()
{
    // Update EWMA filter for each data
    out_data->EWMA_out_ac_x = (EWMA_filter_alpha * out_data->out_ac_x) + ((1 - EWMA_filter_alpha) * out_data->EWMA_out_ac_x);
    out_data->EWMA_out_ac_y = (EWMA_filter_alpha * out_data->out_ac_y) + ((1 - EWMA_filter_alpha) * out_data->EWMA_out_ac_y);
    out_data->EWMA_out_ac_z = (EWMA_filter_alpha * out_data->out_ac_z) + ((1 - EWMA_filter_alpha) * out_data->EWMA_out_ac_z);
    out_data->EWMA_out_temp = (EWMA_filter_alpha * out_data->out_temp) + ((1 - EWMA_filter_alpha) * out_data->EWMA_out_temp);
    out_data->EWMA_out_gy_x = (EWMA_filter_alpha * out_data->out_gy_x) + ((1 - EWMA_filter_alpha) * out_data->EWMA_out_gy_x);
    out_data->EWMA_out_gy_y = (EWMA_filter_alpha * out_data->out_gy_y) + ((1 - EWMA_filter_alpha) * out_data->EWMA_out_gy_y);
    out_data->EWMA_out_gy_z = (EWMA_filter_alpha * out_data->out_gy_z) + ((1 - EWMA_filter_alpha) * out_data->EWMA_out_gy_z);
}