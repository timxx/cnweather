cnWeather
=========

China Weather

DATA FROM: www.weather.com.cn

HOW IT WORKS
============

1)
***GET CITY LIST***

GET:    http://www.weather.com.cn/data/list3/city.xml
RETURN: All Cities

2)
***GET SUB CITY LIST***

GET:    http://www.weather.com.cn/data/list3/city*ID*.xml
        *ID* replace to one of the city code from 1), e.g. 01
RETURN: Sub cities

3)
***REPEAT 2) UNTIL YOU GOT XXXXXX|YYXXXXXX***

4)
***GET WEATHER DATA***

GET:    http://m.weather.com.cn/data/XXXXXXXXX.html
        OR http://www.weather.com.cn/data/sk/XXXXXXXXX.html
        IF you want less details
RETURN: Weather information
