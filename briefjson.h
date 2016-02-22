#ifndef BRIEFJSON_H
#define BRIEFJSON_H
#include <wchar.h>

#ifndef __cplusplus
typedef int bool;
#endif // !__cplusplus

//types of json object
typedef enum
{
	NONE = 0,
	BOOLEAN = 1,
	INTEGER = 2,
	DECIMAL = 3,
	TEXT = 4,
	ARRAY = 5,
	TABLE = 6,
}json_type;

typedef struct json_object{
	json_type type;	//type of json object
	union
	{
		bool boolean;
		long long integer;
		double decimal;
		wchar_t *text;
		struct json_object *item;
	}value;		//union of json value
	struct json_object *next;	//if object included in array or table,it point to next object of the array or table
	wchar_t key[1];				//if object included in table,it is key
}json_object;

void json_object_free(json_object* data);	//free the memory of json object
json_object json_parse(wchar_t json[], wchar_t **message, long* error_pos);//parse json text to json object
wchar_t *json_serialize(json_object* data);//serialize json object to json text

#endif // BRIEFJSON_H
