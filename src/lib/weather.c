#include <curl/curl.h>
#include <string.h> /* for memcpy strstr */
#include <stdlib.h> /* for realloc, free*/
#include <ctype.h>	/* for isdigit */

#include "weather.h"

struct _wSession
{
    char*   buffer;
    size_t  length;
    CURL*   curl;
};

static const char *_temp[] = {
	"temp1", "temp2", "temp3"
};

static const char *_weather[] = {
	"weather1", "weather2", "weather3"
};

static const char *_wind[] = {
	"wind1", "wind2", "wind3"
};

static const char *_agent = "cnWeather/0.1";
//static const char *_city_list_url = "http://www.weather.com.cn/data/list3/city";
static const char *_weather_data_url = "http://m.weather.com.cn/data/";
static const char *_default_city_url = "http://61.4.185.48:81/g";

static size_t write_func(void *ptr, size_t size, size_t nmemb, void *data);
static void parse_data(char *data, size_t len, WeatherInfo *wi);
static int get_value(const char *data, size_t len, const char *key, char **value);
static unsigned int get_default_city_id();

static int get_url_data(wSession *ws, const char *url);

wSession* weather_open()
{
    wSession *ws = NULL;

    ws = (wSession *)calloc(1, sizeof(wSession));
    if( ws == NULL)
        return ws;

    ws->curl = curl_easy_init();

    if (ws->curl){
        curl_easy_setopt(ws->curl, CURLOPT_USERAGENT, _agent);
    }

    return ws;
}

void weather_close(wSession *ws)
{
    if (ws)
    {
        if (ws->curl)
            curl_easy_cleanup(ws->curl);

        if (ws->buffer)
            free(ws->buffer);

        ws->buffer = NULL;
        ws->length = 0;
        ws->curl = NULL;

        free(ws);
    }
}

int weather_get(wSession *ws,
            unsigned int city_id,
            WeatherInfo *wi)
{
    char url[250];
    int ret = CURLE_OK;

    if (ws == NULL || ws->curl == NULL || wi == NULL)
        return -1;

    snprintf(url, 250, "%s%d.html", _weather_data_url, city_id);

	ret = get_url_data(ws, url);

    wi->city_id = city_id;
    if (ret == CURLE_OK)
        parse_data(ws->buffer, ws->length, wi);

    return ret;
}

int weather_get_default_city(wSession *ws, WeatherInfo *wi)
{
	unsigned int id;

	id = get_default_city_id();
	if (id == 0)
		return -1;

	return weather_get(ws, id, wi);
}

int weather_set_proxy(wSession *ws, ProxyInfo *pi)
{
    int ret = CURLE_OK;

    if (ws == NULL || ws->curl ||
        pi == NULL || pi->server
        )
        return -1;

    do
    {
        ret = curl_easy_setopt(ws->curl, CURLOPT_PROXY, pi->server);
        if (ret != CURLE_OK)
            break;

        ret = curl_easy_setopt(ws->curl, CURLOPT_PROXYPORT, pi->port);
        if (ret != CURLE_OK)
            break;

        if (pi->usr != NULL)
        {
            curl_easy_setopt(ws->curl, CURLOPT_PROXYUSERNAME, pi->usr);
            if (pi->pwd != NULL)
                curl_easy_setopt(ws->curl, CURLOPT_PROXYPASSWORD, pi->pwd);
        }

        ret = curl_easy_setopt(ws->curl, CURLOPT_PROXYTYPE, pi->type);
    }
    while(0);

    return ret;
}

static size_t write_func(void *ptr, size_t size, size_t nmemb, void *data)
{
    size_t real_size = size * nmemb;
    wSession *ws = (wSession *)data;

    if (ptr == NULL)
        return 0;

    ws->buffer = (char*)realloc(ws->buffer, ws->length + real_size + 1);
    if (ws->buffer == NULL)
        return 0;

    if (memcpy(&(ws->buffer[ws->length]), ptr, real_size) == NULL)
        return 0;

    ws->length += real_size;
    ws->buffer[ws->length] = 0;

    return real_size;
}

static void parse_data(char *data, size_t len, WeatherInfo *wi)
{
    size_t i;
    
    if (wi == NULL)
        return ;

    if (data == NULL || len <= 0)
        return ;

	for(i=0; i<3; ++i)
	{
		wi->weather[i].temperature = 0;
		wi->weather[i].weather = NULL;
		wi->weather[i].wind = NULL;
	}

	get_value(data, len, "city", &wi->city);

    for(i=0; i<3; ++i)
    {
		get_value(data, len, _temp[i],		&wi->weather[i].temperature);
		get_value(data, len, _weather[i],	&wi->weather[i].weather);
		get_value(data, len, _wind[i],		&wi->weather[i].wind);
	}
}

static int get_value(const char *data, size_t len, const char *key, char **value)
{
	size_t i;
	const char *p, *t;
	
	p = strstr(data, key);
	if (p == NULL)
		return -1;

	i = p - data;
	while( *p != ':' && i < len) {
		p++; i++;
	}

	if ( !p || i >= len)
		return -1;

	while ( *p != '"' && i < len) {
		p++; i++;
	}

	if (!p || i >= len)
		return -1;

	while ((*p == '"' || *p == ' ') && i < len){
		p++; i++;
	}

	if (!p || i >= len)
		return -1;

	t = p;

	while( *p != '"' && i < len) {
		p++; i++;
	}

	i = p - t;

	*value = (char *)calloc(i, sizeof(char));
	if (*value == NULL)
		return -1;

	strncpy(*value, t, i);

	return 0;
}

WeatherInfo *weather_new_info()
{
	WeatherInfo *wi = NULL;
	int i;

	wi = (WeatherInfo*)malloc(sizeof(WeatherInfo));
	if (wi == NULL)
		return NULL;

	for(i=0; i<3; i++)
	{
		wi->weather[i].temperature = 0;
		wi->weather[i].weather = NULL;
		wi->weather[i].wind = NULL;
	}

	wi->city_id = 0;
	wi->city = NULL;

	return wi;
}

void weather_free_info(WeatherInfo *wi)
{
	if (wi)
	{
		int i;

		for(i=0; i<3; ++i)
		{
			if (wi->weather[i].temperature)
				free(wi->weather[i].temperature);

			if (wi->weather[i].weather)
				free(wi->weather[i].weather);

			if (wi->weather[i].wind)
				free(wi->weather[i].wind);
		}

		if (wi->city){
			free(wi->city);
		}
	}
}

static unsigned int get_default_city_id()
{
	unsigned int id = 0;
	wSession *ws;
	char *p, *t;

	ws = weather_open();
	if (ws == NULL)
		return 0;

	do
	{
		if (get_url_data(ws, _default_city_url) != 0)
			break;
		
		if (ws->buffer == NULL || ws->length == 0)
			break;
		
		p = strstr(ws->buffer, "id");
		if (p == NULL)
			break;

		while (p && !isdigit(*p))
			p++;

		if ( !p )
			break;
		t = p;

		while(p && isdigit(*p))
			p++;
		if (p)
			*p = 0;

		id = atoi(t);
	}
	while(0);

	weather_close(ws);

	return id;
}

static int get_url_data(wSession *ws, const char *url)
{
	int ret = -1;
    do
    {
        ret = curl_easy_setopt(ws->curl, CURLOPT_URL, url);
        if (ret != CURLE_OK)
            break;

        ret = curl_easy_setopt(ws->curl, CURLOPT_HTTPGET, 1);
        if (ret != CURLE_OK)
            break;

		curl_easy_setopt(ws->curl, CURLOPT_FOLLOWLOCATION, 1);

        ret = curl_easy_setopt(ws->curl, CURLOPT_WRITEFUNCTION, &write_func);
		if (ret != CURLE_OK)
            break;

        ret = curl_easy_setopt(ws->curl, CURLOPT_WRITEDATA, ws);
        if (ret != CURLE_OK)
            break;

		curl_easy_setopt(ws->curl, CURLOPT_CONNECTTIMEOUT, 30);

        ret = curl_easy_perform(ws->curl);
    }
    while(0);

	return ret;
}
