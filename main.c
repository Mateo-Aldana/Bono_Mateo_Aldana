#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/ledc.h"
#include "driver/gptimer.h"

#define ledV             GPIO_NUM_17
#define ledR             GPIO_NUM_4
#define PULSADOR_DER     GPIO_NUM_5
#define PULSADOR_IZQ     GPIO_NUM_16

#define poten            ADC1_CHANNEL_7

#define MOT_HS_IZQ       GPIO_NUM_18
#define MOT_LS_IZQ       GPIO_NUM_15
#define MOT_HS_DER       GPIO_NUM_13
#define MOT_LS_DER       GPIO_NUM_12

#define PWM_MODO         LEDC_LOW_SPEED_MODE
#define PWM_TMR          LEDC_TIMER_0
#define PWM_RESOLUCION   LEDC_TIMER_8_BIT
#define PWM_FREQ         100

#define PWM_CANAL_HS_IZQ LEDC_CHANNEL_0
#define PWM_CANAL_HS_DER LEDC_CHANNEL_1
#define PWM_DUTY_MAXIMO  255

#define SEGMENTO_A   GPIO_NUM_19
#define SEGMENTO_B   GPIO_NUM_21
#define SEGMENTO_C   GPIO_NUM_22
#define SEGMENTO_D   GPIO_NUM_23
#define SEGMENTO_E   GPIO_NUM_25
#define SEGMENTO_F   GPIO_NUM_33
#define SEGMENTO_G   GPIO_NUM_32

#define DIGITO2    GPIO_NUM_14
#define DIGITO3    GPIO_NUM_27
#define DIGITO4    GPIO_NUM_26

const uint8_t tabla_digitos[10][7] = {
    {0,0,0,0,0,0,1},
    {1,0,0,1,1,1,1},
    {0,0,1,0,0,1,0},
    {0,0,0,0,1,1,0},
    {1,0,0,1,1,0,0},
    {0,1,0,0,1,0,0},
    {0,1,0,0,0,0,0},
    {0,0,0,1,1,1,1},
    {0,0,0,0,0,0,0},
    {0,0,0,0,1,0,0}
};

typedef enum {
    MOT_DETENIDO = 0,
    MOT_HORARIO,
    MOT_ANTIHORARIO
} direccion_motor_t;

gptimer_handle_t temporizador_delay = NULL;

void iniciar_temporizador(void)
{
    gptimer_config_t cfg_timer = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,
    };
    gptimer_new_timer(&cfg_timer, &temporizador_delay);
    gptimer_enable(temporizador_delay);
    gptimer_start(temporizador_delay);
}

void esperar_us(uint32_t us)
{
    uint64_t t_inicio, t_actual;
    gptimer_get_raw_count(temporizador_delay, &t_inicio);
    do {
        gptimer_get_raw_count(temporizador_delay, &t_actual);
    } while ((t_actual - t_inicio) < us);
}

void inicializar_pines(void)
{
    gpio_set_direction(ledV, GPIO_MODE_OUTPUT);
    gpio_set_direction(ledR, GPIO_MODE_OUTPUT);

    gpio_set_direction(PULSADOR_DER, GPIO_MODE_INPUT);
    gpio_set_direction(PULSADOR_IZQ, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PULSADOR_DER, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(PULSADOR_IZQ, GPIO_PULLUP_ONLY);

    gpio_set_direction(MOT_HS_IZQ, GPIO_MODE_OUTPUT);
    gpio_set_direction(MOT_LS_IZQ, GPIO_MODE_OUTPUT);
    gpio_set_direction(MOT_HS_DER, GPIO_MODE_OUTPUT);
    gpio_set_direction(MOT_LS_DER, GPIO_MODE_OUTPUT);

    gpio_set_direction(SEGMENTO_A, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEGMENTO_B, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEGMENTO_C, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEGMENTO_D, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEGMENTO_E, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEGMENTO_F, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEGMENTO_G, GPIO_MODE_OUTPUT);

    gpio_set_direction(DIGITO2, GPIO_MODE_OUTPUT);
    gpio_set_direction(DIGITO3, GPIO_MODE_OUTPUT);
    gpio_set_direction(DIGITO4, GPIO_MODE_OUTPUT);
}

void inicializar_adc(void)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(poten, ADC_ATTEN_DB_11);
}

void inicializar_pwm_motor(void)
{
    ledc_timer_config_t cfg_tmr = {
        .speed_mode = PWM_MODO,
        .duty_resolution = PWM_RESOLUCION,
        .timer_num = PWM_TMR,
        .freq_hz = PWM_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&cfg_tmr);

    ledc_channel_config_t canal_izq = {
        .gpio_num = MOT_HS_IZQ,
        .speed_mode = PWM_MODO,
        .channel = PWM_CANAL_HS_IZQ,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = PWM_TMR,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&canal_izq);

    ledc_channel_config_t canal_der = {
        .gpio_num = MOT_HS_DER,
        .speed_mode = PWM_MODO,
        .channel = PWM_CANAL_HS_DER,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = PWM_TMR,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&canal_der);
}

int leer_potenciometro(void)
{
    int suma = 0;
    for (int i = 0; i < 10; i++)
    {
        suma += adc1_get_raw(poten);
        esperar_us(200);
    }
    return suma / 10;
}

void desactivar_digitos(void)
{
    gpio_set_level(DIGITO2, 1);
    gpio_set_level(DIGITO3, 1);
    gpio_set_level(DIGITO4, 1);
}

void borrar_segmentos(void)
{
    gpio_set_level(SEGMENTO_A, 1);
    gpio_set_level(SEGMENTO_B, 1);
    gpio_set_level(SEGMENTO_C, 1);
    gpio_set_level(SEGMENTO_D, 1);
    gpio_set_level(SEGMENTO_E, 1);
    gpio_set_level(SEGMENTO_F, 1);
    gpio_set_level(SEGMENTO_G, 1);
}

void escribir_segmentos(int numero)
{
    gpio_set_level(SEGMENTO_A, tabla_digitos[numero][0]);
    gpio_set_level(SEGMENTO_B, tabla_digitos[numero][1]);
    gpio_set_level(SEGMENTO_C, tabla_digitos[numero][2]);
    gpio_set_level(SEGMENTO_D, tabla_digitos[numero][3]);
    gpio_set_level(SEGMENTO_E, tabla_digitos[numero][4]);
    gpio_set_level(SEGMENTO_F, tabla_digitos[numero][5]);
    gpio_set_level(SEGMENTO_G, tabla_digitos[numero][6]);
}

void encender_digito(int digito, int valor)
{
    desactivar_digitos();
    escribir_segmentos(valor);

    switch (digito)
    {
        case 2: gpio_set_level(DIGITO2, 0); break;
        case 3: gpio_set_level(DIGITO3, 0); break;
        case 4: gpio_set_level(DIGITO4, 0); break;
    }

    esperar_us(2000);
    desactivar_digitos();
}

void mostrar_velocidad(int numero)
{
    if (numero < 0) numero = 0;
    if (numero > 100) numero = 100;

    int centenas = numero / 100;
    int decenas  = (numero / 10) % 10;
    int unidades = numero % 10;

    if (numero == 100)
        encender_digito(2, centenas);
    else
    {
        desactivar_digitos();
        borrar_segmentos();
        esperar_us(2000);
    }

    if (numero >= 10)
        encender_digito(3, decenas);
    else
    {
        desactivar_digitos();
        borrar_segmentos();
        esperar_us(2000);
    }

    encender_digito(4, unidades);
}

static inline void activar_ls_izq(void)    { gpio_set_level(MOT_LS_IZQ, 0); }
static inline void desactivar_ls_izq(void) { gpio_set_level(MOT_LS_IZQ, 1); }

static inline void activar_ls_der(void)    { gpio_set_level(MOT_LS_DER, 0); }
static inline void desactivar_ls_der(void) { gpio_set_level(MOT_LS_DER, 1); }

void ajustar_pwm_hs_izq(int porcentaje)
{
    if (porcentaje < 0) porcentaje = 0;
    if (porcentaje > 100) porcentaje = 100;

    uint32_t duty = (porcentaje * PWM_DUTY_MAXIMO) / 100;
    ledc_set_duty(PWM_MODO, PWM_CANAL_HS_IZQ, duty);
    ledc_update_duty(PWM_MODO, PWM_CANAL_HS_IZQ);
}

void ajustar_pwm_hs_der(int porcentaje)
{
    if (porcentaje < 0) porcentaje = 0;
    if (porcentaje > 100) porcentaje = 100;

    uint32_t duty = (porcentaje * PWM_DUTY_MAXIMO) / 100;
    ledc_set_duty(PWM_MODO, PWM_CANAL_HS_DER, duty);
    ledc_update_duty(PWM_MODO, PWM_CANAL_HS_DER);
}

void detener_pwm_hs(void)
{
    ledc_set_duty(PWM_MODO, PWM_CANAL_HS_IZQ, 0);
    ledc_update_duty(PWM_MODO, PWM_CANAL_HS_IZQ);

    ledc_set_duty(PWM_MODO, PWM_CANAL_HS_DER, 0);
    ledc_update_duty(PWM_MODO, PWM_CANAL_HS_DER);
}

void detener_motor(void)
{
    detener_pwm_hs();
    desactivar_ls_izq();
    desactivar_ls_der();
}

void girar_horario(int velocidad)
{
    detener_motor();
    esperar_us(200);

    desactivar_ls_izq();
    activar_ls_der();

    ajustar_pwm_hs_der(0);
    ajustar_pwm_hs_izq(velocidad);
}

void girar_antihorario(int velocidad)
{
    detener_motor();
    esperar_us(200);

    desactivar_ls_der();
    activar_ls_izq();

    ajustar_pwm_hs_izq(0);
    ajustar_pwm_hs_der(velocidad);
}

void app_main(void)
{
    inicializar_pines();
    inicializar_adc();
    inicializar_pwm_motor();
    iniciar_temporizador();

    int prev_pulsador_der = 1;
    int prev_pulsador_izq = 1;

    gpio_set_level(ledV, 0);
    gpio_set_level(ledR, 0);

    desactivar_digitos();
    borrar_segmentos();

    direccion_motor_t dir_motor = MOT_DETENIDO;
    int valor = 0;

    detener_motor();

    while (1)
    {
        int estado_pulsador_der = gpio_get_level(PULSADOR_DER);
        int estado_pulsador_izq = gpio_get_level(PULSADOR_IZQ);

        if (prev_pulsador_der == 1 && estado_pulsador_der == 0)
        {
            gpio_set_level(ledV, 1);
            gpio_set_level(ledR, 0);
            dir_motor = MOT_HORARIO;
        }

        if (prev_pulsador_izq == 1 && estado_pulsador_izq == 0)
        {
            gpio_set_level(ledV, 0);
            gpio_set_level(ledR, 1);
            dir_motor = MOT_ANTIHORARIO;
        }

        prev_pulsador_der = estado_pulsador_der;
        prev_pulsador_izq = estado_pulsador_izq;

        int lectura = leer_potenciometro();
        valor = (lectura * 100) / 4095;

        if (valor < 15)
            valor = 0;

        if (dir_motor == MOT_HORARIO)
            girar_horario(valor);
        else if (dir_motor == MOT_ANTIHORARIO)
            girar_antihorario(valor);
        else
            detener_motor();

        for (int i = 0; i < 25; i++)
        {
            mostrar_velocidad(valor);
        }

        printf("ADC: %d | Velocidad: %d | Dir: %d\n", lectura, valor, dir_motor);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}