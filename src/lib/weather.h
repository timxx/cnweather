#ifndef __WEATHER_H__
#define __WEATHER_H__

#ifdef __cplusplus
export "C" {
#endif

#include <glib.h>

typedef struct
{
    guint		city_id;
	gchar*		city;

    struct
    {
        gchar*	temperature;
        gchar*	weather;
        gchar*	wind;
		gint	img;
    }
    weather[3];

} WeatherInfo;

/* Possible value for ProxyInfo type */
#define TYPE_HTTP   CURLPROXY_HTTP
#define TYPE_SOCKS4 CURLPROXY_SOCKS4
#define TYPE_SOCKS5 CURLPROXY_SOCKS5

typedef struct
{
    gint     type;
    gchar*   server;
    gint     port;
    gchar*   usr;
    gchar*   pwd;
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
gint
weather_get(wSession *ws,
            guint city_id,
            WeatherInfo *wi);

/**
 * try to get weather by current ip adrress
 * @ws
 * @wi
 * @return
 */
gint
weather_get_default_city(wSession *ws,
			WeatherInfo *wi);
/**
 * set proxy for session
 * @ws
 * @pi
 * @return 0 if OK
 */
gint weather_set_proxy(wSession *ws, ProxyInfo *pi);


WeatherInfo *weather_new_info();

/**
 * free WeatherInfo->weather & city only
 * @wi
 */
void weather_free_info(WeatherInfo *wi);

gint weather_get_city_list(const gchar *db_file);

#ifdef __cplusplus
}
#endif

#endif /* __WEATHER_H__ */
