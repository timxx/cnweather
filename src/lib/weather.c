#include <curl/curl.h>
#include <string.h>
#include <sqlite3.h>

#include "weather.h"

struct _wSession
{
    gchar*   buffer;
    size_t  length;
    CURL*   curl;

	void*	write_data;
	curl_write_callback write_cb;
};

static const gchar *_temp[] = {
	"temp1", "temp2", "temp3"
};

static const gchar *_weather[] = {
	"weather1", "weather2", "weather3"
};

static const gchar *_wind[] = {
	"wind1", "wind2", "wind3"
};

static const gchar *_img[] = {
	"img1", "img3", "img5"
};

static const gchar *_agent = "cnWeather/0.1";
static const gchar *_city_list_url = "http://www.weather.com.cn/data/list3/city";
static const gchar *_weather_data_url = "http://m.weather.com.cn/data/";
static const gchar *_default_city_url = "http://61.4.185.48:81/g";
static const gchar *_city_db_url = "http://cloud.github.com/downloads/timxx/cnweather/cities.db";
static const gchar *_cur_weather_data_url = "http://www.weather.com.cn/data/sk/";

static size_t write_func(void *ptr, size_t size, size_t nmemb, void *data);
static void parse_data(gchar *data, size_t len, WeatherInfo *wi);
static gint get_value(const gchar *data, size_t len, const gchar *key, gchar **value);
static gchar* get_default_city_id();

static int get_url_data(wSession *ws, const gchar *url);

static void get_city_list(gchar *parent_id, sqlite3 *db);
static void get_city_id(gchar *id, gchar *name, gchar *parent_id, sqlite3 *db);

static void init_db_tables(sqlite3 *db);
static void insert_item_to_province(sqlite3 *db, gchar *id, gchar *name);
static void insert_item_to_city(sqlite3 *db, gchar *id, gchar *name, gchar *pid);
static void insert_item_to_town(sqlite3 *db, gchar *id, gchar *name, gchar *city_id, gchar *cid);

static gint exe_sql(sqlite3 *db, gchar *sql);

static size_t write_file_func(void *ptr, size_t size, size_t nmemb, void *data);

static gint
get_real_time_temperature(wSession *ws, const gchar *city_id, gchar **temp);

wSession* weather_open()
{
    wSession *ws = NULL;

    ws = (wSession *)g_malloc0(sizeof(wSession));
    if( ws == NULL)
        return ws;

    ws->curl = curl_easy_init();

    if (ws->curl){
        curl_easy_setopt(ws->curl, CURLOPT_USERAGENT, _agent);
    }

	ws->write_data = ws;
	ws->write_cb = (curl_write_callback )&write_func;

    return ws;
}

void weather_close(wSession *ws)
{
    if (ws)
    {
        if (ws->curl)
            curl_easy_cleanup(ws->curl);

        if (ws->buffer)
            g_free(ws->buffer);

        ws->buffer = NULL;
        ws->length = 0;
        ws->curl = NULL;

        g_free(ws);
    }
}

int weather_get(wSession *ws,
            WeatherInfo *wi)
{
    gchar url[250];
    gint ret = CURLE_OK;

    if (ws == NULL || ws->curl == NULL || wi == NULL)
        return -1;

	if (ws->buffer != NULL)
	{
		g_free(ws->buffer);
		ws->buffer = NULL;
		ws->length = 0;
	}

	if (wi->city_id == NULL)
	{
		wi->city_id = get_default_city_id();
	}

    snprintf(url, 250, "%s%s.html", _weather_data_url, wi->city_id);

	ret = get_url_data(ws, url);

	if (ret == CURLE_OK)
	{
		gchar *temp = NULL;
		parse_data(ws->buffer, ws->length, wi);

		if (get_real_time_temperature(ws,  wi->city_id, &temp) == 0 && temp != NULL)
		{
			wi->temp = g_strdup_printf("%s ℃", temp);
			g_free(temp);
		}
	}

    return ret;
}

gint weather_set_proxy(wSession *ws, ProxyInfo *pi)
{
    gint ret = CURLE_OK;

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

    ws->buffer = (char*)g_realloc(ws->buffer, ws->length + real_size + 1);
    if (ws->buffer == NULL)
        return 0;

    if (memcpy(&(ws->buffer[ws->length]), ptr, real_size) == NULL)
        return 0;

    ws->length += real_size;
    ws->buffer[ws->length] = 0;

    return real_size;
}

static void parse_data(gchar *data, size_t len, WeatherInfo *wi)
{
    size_t i;
    
    if (wi == NULL)
        return ;

    if (data == NULL || len <= 0)
        return ;

	for(i=0; i<3; ++i)
	{
		wi->weather[i].temperature = NULL;
		wi->weather[i].weather = NULL;
		wi->weather[i].wind = NULL;
	}

	get_value(data, len, "city", &wi->city);

    for(i=0; i<3; ++i)
    {
		gchar *img = NULL;

		get_value(data, len, _temp[i],		&wi->weather[i].temperature);
		get_value(data, len, _weather[i],	&wi->weather[i].weather);
		get_value(data, len, _wind[i],		&wi->weather[i].wind);
		get_value(data, len, _img[i],		&img);
		if (img)
		{
			wi->weather[i].img = g_strtod(img, NULL);
			g_free(img);
		}
	}
}

static gint get_value(const gchar *data, size_t len, const gchar *key, gchar **value)
{
	size_t i;
	const gchar *p, *t;
	
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

	i = p - t; /* len to copy */
	*value = (gchar *)g_malloc0_n(i + sizeof(gchar), sizeof(gchar));
	if (*value == NULL)
		return -1;

	strncpy(*value, t, i);

	return 0;
}

WeatherInfo *weather_new_info()
{
	WeatherInfo *wi = NULL;
	gint i;

	wi = (WeatherInfo*)g_malloc(sizeof(WeatherInfo));
	if (wi == NULL)
		return NULL;

	for(i=0; i<3; i++)
	{
		wi->weather[i].temperature = NULL;
		wi->weather[i].weather = NULL;
		wi->weather[i].wind = NULL;
		wi->weather[i].img = 0;
	}

	wi->temp = NULL;
	wi->city_id = NULL;
	wi->city = NULL;

	return wi;
}

void weather_free_info(WeatherInfo *wi)
{
	if (wi)
	{
		gint i;

		for(i=0; i<3; ++i)
		{
			g_free(wi->weather[i].temperature);
			g_free(wi->weather[i].weather);
			g_free(wi->weather[i].wind);
		}

		g_free(wi->city);
		g_free(wi->city_id);
		g_free(wi->temp);

		g_free(wi);
	}
}

WeatherInfo *weather_dup_info(WeatherInfo *wi)
{
	WeatherInfo *w = NULL;
	gint i;

	if (wi == NULL)
		return NULL;

	w = (WeatherInfo*)g_malloc(sizeof(WeatherInfo));
	if (w == NULL)
		return NULL;

	for(i=0; i<3; i++)
	{
		w->weather[i].temperature = g_strdup(wi->weather[i].temperature);
		w->weather[i].weather = g_strdup(wi->weather[i].weather);
		w->weather[i].wind = g_strdup(wi->weather[i].wind);
		w->weather[i].img = wi->weather[i].img;
	}

	w->temp = g_strdup(wi->temp);
	w->city_id = g_strdup(wi->city_id);
	w->city = g_strdup(wi->city);

	return w;
}

static gchar* get_default_city_id()
{
	wSession *ws;
	gchar *p, *t;
	gchar *id = NULL;

	ws = weather_open();
	if (ws == NULL)
		return NULL;

	do
	{
		if (get_url_data(ws, _default_city_url) != 0)
			break;
		
		if (ws->buffer == NULL || ws->length == 0)
			break;
		
		p = strstr(ws->buffer, "id");
		if (p == NULL)
			break;

		while (p && !g_ascii_isdigit(*p))
			p++;

		if ( !p )
			break;
		t = p;

		while(p && g_ascii_isdigit(*p))
			p++;
		if (p)
			*p = 0;

		id = g_strdup(t);
	}
	while(0);

	weather_close(ws);

	return id;
}

static gint get_url_data(wSession *ws, const gchar *url)
{
	gint ret = -1;
    do
    {
        ret = curl_easy_setopt(ws->curl, CURLOPT_URL, url);
        if (ret != CURLE_OK)
            break;

        ret = curl_easy_setopt(ws->curl, CURLOPT_HTTPGET, 1);
        if (ret != CURLE_OK)
            break;

		curl_easy_setopt(ws->curl, CURLOPT_FOLLOWLOCATION, 1);

        ret = curl_easy_setopt(ws->curl, CURLOPT_WRITEFUNCTION, ws->write_cb);
		if (ret != CURLE_OK)
            break;

        ret = curl_easy_setopt(ws->curl, CURLOPT_WRITEDATA, ws->write_data);
        if (ret != CURLE_OK)
            break;

		curl_easy_setopt(ws->curl, CURLOPT_CONNECTTIMEOUT, 10);

		/* IMPORTANT */
		curl_easy_setopt(ws->curl, CURLOPT_NOSIGNAL, 1L);

        ret = curl_easy_perform(ws->curl);
    }
    while(0);

	return ret;
}

gint weather_get_city_list(const gchar *db_file)
{
	sqlite3 *db = NULL;

	if (sqlite3_open(db_file, &db) != SQLITE_OK)
		return -1;

	init_db_tables(db);

	exe_sql(db, "BEGIN;");

	get_city_list("", db);

	exe_sql(db, "COMMIT;");

	sqlite3_close(db);

	return 0;
}

static void get_city_list(gchar *parent_id, sqlite3 *db)
{
	wSession *ws;

	gchar **city_list = NULL;
	gchar *url = NULL;
	gint i;

	ws = weather_open();
	if (ws == NULL)
		return ;

	do
	{
		url = g_strdup_printf("%s%s.xml", _city_list_url, parent_id);
		if (url == NULL)
			break;

		if (get_url_data(ws, url) != 0)
			break;

		if (ws->buffer == NULL || ws->length == 0)
			break;

		city_list = g_strsplit(ws->buffer, ",", 0);
		if (city_list == NULL)
			break;

		i = 0;

		while(city_list[i])
		{
			gchar **values;
			gint len;

			values = g_strsplit(city_list[i], "|", 0);
			if (values == NULL)
			{
				i++;
				continue;
			}

			len = strlen(values[0]);

			switch(len)
			{
				case 2:
					insert_item_to_province(db, values[0], values[1]);
					get_city_list(values[0], db);

					break;

				case 4:
					insert_item_to_city(db, values[0], values[1], parent_id);
					get_city_list(values[0], db);

					break;

				case 6:
					get_city_id(values[0], values[1], parent_id, db);
					break;
			}
			
			g_strfreev(values);

			i++;
		}

	}
	while(0);

	if (city_list)
		g_strfreev(city_list);

	if (url)
		g_free(url);

	weather_close(ws);
}

static void get_city_id(gchar *id, gchar *name, gchar *parent_id, sqlite3 *db)
{
	wSession *ws;

	gchar **values = NULL;
	gchar *url = NULL;

	ws = weather_open();
	if (ws == NULL)
		return ;

	do
	{
		url = g_strdup_printf("%s%s.xml", _city_list_url, id);
		if (url == NULL)
			break;

		if (get_url_data(ws, url) != 0)
			break;

		if (ws->buffer == NULL || ws->length == 0)
			break;

		values = g_strsplit(ws->buffer, "|", 0);
		if (values == NULL)
			break;

		insert_item_to_town(db, id, name, values[1], parent_id);
	}
	while(0);

	if (values)
		g_strfreev(values);

	if (url)
		g_free(url);

	weather_close(ws);
}

static void init_db_tables(sqlite3 *db)
{
	gchar *sql;

	sql = "DROP TABLE IF EXISTS province";
	exe_sql(db, sql);

	sql = "DROP TABLE IF EXISTS city";
	exe_sql(db, sql);

	sql = "DROP TABLE IF EXISTS town";
	exe_sql(db, sql);

	sql = "CREATE TABLE province("
				"pid VARCHAR(3) PRIMARY KEY, "
				"pname VARCHAR(32)"
				")"
				;
	exe_sql(db, sql);

	sql = "CREATE TABLE city("
				"cid VARCHAR(5) PRIMARY KEY, "
				"cname VARCHAR(32), "
				"pid VARCHAR(3), "
				"FOREIGN KEY (pid) REFERENCES province(pid) "
				"ON UPDATE CASCADE"
				")"
				;
	exe_sql(db, sql);

	sql =  "CREATE TABLE town("
				"tid VARCHAR(7) PRIMARY KEY, "
				"tname VARCHAR(32), "
				"city_id VARCHAR(10), "
				"cid VARCHAR(5), "
				"FOREIGN KEY (cid) REFERENCES city(cid) "
				"ON UPDATE CASCADE"
				")"
				;
	exe_sql(db, sql);
}

static void insert_item_to_province(sqlite3 *db, gchar *id, gchar *name)
{
	gchar *sql;

	sql = g_strdup_printf("INSERT INTO province values('%s', '%s')", id, name);

	exe_sql(db, sql);

	g_free(sql);
}

static void insert_item_to_city(sqlite3 *db, gchar *id, gchar *name, gchar *pid)
{
	gchar *sql;

	sql = g_strdup_printf("INSERT INTO city values('%s', '%s', '%s')", id, name, pid);

	exe_sql(db, sql);

	g_free(sql);
}

static void insert_item_to_town(sqlite3 *db, gchar *id, gchar *name, gchar *city_id, gchar *cid)
{
	gchar *sql;

	sql = g_strdup_printf("INSERT INTO town values('%s', '%s', '%s', '%s')", id, name, city_id, cid);

	exe_sql(db, sql);

	g_free(sql);
}

static gint exe_sql(sqlite3 *db, gchar *sql)
{
	gint result;
	gchar *msg = NULL;

	result = sqlite3_exec(db, sql, NULL, NULL, &msg);

	if (result != SQLITE_OK)
	{
		g_print("Error: %s (%s, %d)\n", msg, __FILE__, __LINE__);
		sqlite3_free(msg);

		return -1;
	}

	return 0;
}

gint weather_get_city_db(const gchar *db_file)
{
	wSession *ws;
	FILE *fp = NULL;
	gint ret;

	g_return_val_if_fail(db_file != NULL, -1);

	ws = weather_open();
	g_return_val_if_fail(ws != NULL, -1);

	fp = fopen(db_file, "w");
	if (fp == NULL)
	{
		weather_close(ws);
		return -1;
	}

	ws->write_cb = (curl_write_callback )&write_file_func;
	ws->write_data = fp;

	ret = get_url_data(ws, _city_db_url);

	fclose(fp);

	weather_close(ws);

	return ret;
}

static size_t write_file_func(void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t real_size = size * nmemb;
	FILE *fp = (FILE *)data;

    if (ptr == NULL || real_size == 0)
        return 0;

	fwrite(ptr, size, nmemb, fp);

    return real_size;
}

static gint
get_real_time_temperature(wSession *ws, const gchar *city_id, gchar **temp)
{
	gchar url[250];
    gint ret = CURLE_OK;

	if (ws->buffer != NULL)
	{
		g_free(ws->buffer);
		ws->buffer = NULL;
		ws->length = 0;
	}

    snprintf(url, 250, "%s%s.html", _cur_weather_data_url, city_id);

	ret = get_url_data(ws, url);

	if (ret == CURLE_OK)
	{
		ret = get_value(ws->buffer, ws->length, "temp", temp);
	}

    return ret;
}
