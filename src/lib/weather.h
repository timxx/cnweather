#ifndef __WEATHER_H__
#define __WEATHER_H__

#ifdef __cplusplus
export "C" {
#endif

typedef struct
{
    unsigned int city_id;

    struct
    {
        char*   temperature;
        char*   weather;
        char*   wind;
    }
    weather[3];

} WeatherInfo;

/* Possible value for ProxyInfo type */
#define TYPE_HTTP   CURLPROXY_HTTP
#define TYPE_SOCKS4 CURLPROXY_SOCKS4
#define TYPE_SOCKS5 CURLPROXY_SOCKS5

typedef struct
{
    int     type;
    char*   server;
    int     port;
    char*   usr;
    char*   pwd;
} ProxyInfo;

typedef struct _wSession wSession;

/**
 * open  new weather session
 * @return new session if ok of NULL if failed
 */
wSession*
weather_open();

/**
 * close session
 * @session session handle
 */
void
weather_close(wSession *ws);

/**
 * get weather info by city id
 * @ws
 * @city_id
 * @wi weather info in return
 * @return 0 if no problem, failed other vaules
 */
int
weather_get(wSession *ws,
            unsigned int city_id,
            WeatherInfo *wi);

/**
 * set proxy for session
 * @ws
 * @pi
 * @return 0 if OK
 */
int weather_set_proxy(wSession *ws, ProxyInfo *pi);

/**
 * free WeatherInfo->weather only
 * @wi
 */
void weather_free_info(WeatherInfo *wi);

#ifdef __cplusplus
}
#endif

#endif /* __WEATHER_H__ */
