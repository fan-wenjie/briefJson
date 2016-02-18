#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "briefJson.h"

static void print(data_item data, int n,wchar_t *key)
{
	for (int i = 0; i < n; ++i)
		printf(" |");
	printf("—");
	if (key) printf("key:%ws\t", key);
	switch (data.type)
	{
	case TABLE: 
	{
		printf("type:map\n");
		list_item *item = (list_item *)data.v.item;
		while (item)
		{
			print(item->value, n + 1, item->key);
			item = item->next;
		}
		break;
	}
	case ARRAY:
	{
		printf("type:array\n");
		list_item *item = (list_item *)data.v.item;
		while (item)
		{
			print(item->value, n + 1, 0);
			item = item->next;
		}
		break;
	}
	case BOOLEAN:
	{
		printf("type:bool\tvalue:%s\n",data.v.integer?"true":"false");
		break;
	}
	case DECIMAL:
	{
		printf("type:double\tvalue:%lf\n", data.v.decimal);
		break;
	}
	case INTEGER:
	{
		printf("type:int\tvalue:%lld\n", data.v.integer);
		break;
	}
	case TEXT:
	{
		printf("type:text\tvalue:%ws\n", data.v.text);
		break;
	}
	case NONE:
	{
		printf("type:null\n");
		break;
	}
	default:
		break;		
	}
}


int main()
{
	wchar_t json[] = L"{\"programmers\":[{\"firstName\":\"Brett\",\"lastName\":\"McLaughlin\",\"email\":\"aaaa\",\"age\":33},{\"firstName\":\"Jason\",\"lastName\":\"Hunter\",\"email\":\"bbbb\",\"age\":25},{\"firstName\":\"Elliotte\",\"lastName\":\"Harold\",\"email\":\"cccc\",\"age\":30}],\"authors\":[{\"firstName\":\"Isaac\",\"lastName\":\"Asimov\",\"genre\":\"sciencefiction\",\"age\":53},{\"firstName\":\"Tad\",\"lastName\":\"Williams\",\"genre\":\"fantasy\",\"age\":47},{\"firstName\":\"Frank\",\"lastName\":\"Peretti\",\"genre\":\"christianfiction\",\"age\":28}],\"musicians\":[{\"firstName\":\"Eric\",\"lastName\":\"Clapton\",\"instrument\":\"guitar\",\"age\":19},{\"firstName\":\"Sergei\",\"lastName\":\"Rachmaninoff\",\"instrument\":\"piano\",\"age\":26}]}";
	parse_result result = json_parse(json);
	if (!result.succeed) return 1;
	data_item item = result.data;
	print(item, 0, 0);
	printf("\n");
	wchar_t *text = json_serialize(item);
	printf("%ws\n", text);
	data_free(item);
	free(text);
	system("pause");
	return 0;
}