
#include "encoder.h"

void encoder_read(encoder* o)
{
    int MSB = gpio_get(o->pin_a);
    int LSB = gpio_get(o->pin_b);

    int encoded  = (MSB << 1) | LSB;
    int sum = (o->prior << 2) | encoded;

    if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) o->value++;
    if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) o->value--;

    o->prior = encoded;
}

void encoder_init(encoder* o, int pin_a, int pin_b)
{
    o->pin_a = pin_a;
    o->pin_b = pin_b;
    o->value = 0;
    o->prior = 0;

    gpio_init(pin_a);
    gpio_init(pin_b);
    gpio_set_dir(pin_a, GPIO_IN);
    gpio_set_dir(pin_b, GPIO_IN);
    gpio_pull_up(pin_a);
    gpio_pull_up(pin_b);
}
