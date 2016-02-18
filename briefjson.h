#ifndef BRIEFJSON_H
#define BRIEFJSON_H

#include <wchar.h>

typedef enum
{
	NONE = 0,
	BOOLEAN = 1,
	INTEGER = 2,
	DECIMAL = 3,
	TEXT = 4,
	ARRAY = 5,
	TABLE = 6,
}type_t;

typedef struct data_item{
	type_t type;
	union
	{
		int boolen;
		long long integer;
		double decimal;
		wchar_t *text;
		struct list_item *item;
	}v;
}data_item;

typedef struct list_item {
	struct data_item value;
	struct list_item *next;
	wchar_t key[0];
}list_item;


typedef struct
{
	data_item data;
	wchar_t *json;
	unsigned pos;
	int succeed;
	wchar_t *message;
}parse_result;

void data_free(data_item data);
parse_result json_parse(wchar_t json[]);
wchar_t *json_serialize(data_item data);

#endif // BRIEFJSON_H
