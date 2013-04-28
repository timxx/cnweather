#include <stdio.h>
#include <string.h>

#include "../lib/weather.h"

int main(int argc, char **argv)
{
    wSession *ws = NULL;
    WeatherInfo wi;

    ws = weather_open();
    if (ws == NULL)
    {
        printf("weather_open failed\n");
        return -1;
    }

	wi.city_id = strdup("101070101");
    if (weather_get(ws, &wi) != 0)
    {
        printf("failed to get weather\n");
    }
    else
    {
        int i;
        for(i = 0; i<3; i++)
        {
            printf("temp:\t%s\n", wi.weather[i].temperature);
            printf("weather:\t%s\n", wi.weather[i].weather);
            printf("wind:\t%s\n", wi.weather[i].wind);
        }

        weather_free_info(&wi);
    }

    weather_close(ws);

    return 0;
}
