#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "briefJson.h"

static void print(json_object *data, int n,wchar_t *key)
{
	for (int i = 0; i < n; ++i)
		wprintf(L" |");
	wprintf(L"-");
	if(key) wprintf(L"key:%ls\t", key);
	switch (data->type)
	{
	case TABLE: 
	{
		wprintf(L"type:map\n");
		json_object *item = (json_object *)data->value.item;
		while (item)
		{
			print(item, n + 1, item->key);
			item = item->next;
		}
		return;
	}
	case ARRAY:
	{
		wprintf(L"type:array\n");
		json_object *item = (json_object *)data->value.item;
		while (item)
		{
			print(item, n + 1, 0);
			item = item->next;
		}
		return;
	}
	case BOOLEAN:
	{
		wprintf(L"type:bool\tvalue:%ls\n",data->value.boolean?L"true":L"false");
		break;
	}
	case DECIMAL:
	{
		wprintf(L"type:double\tvalue:%lf\n", data->value.decimal);
		break;
	}
	case INTEGER:
	{
		wprintf(L"type:int\tvalue:%lld\n", data->value.integer);
		break;
	}
	case TEXT:
	{
		wprintf(L"type:text\tvalue:%ls\n", data->value.text);
		break;
	}
	case NONE:
	{
		wprintf(L"type:null\n");
		break;
	}
	default:
		break;		
	}
}


int main()
{
	wchar_t json[] = L"{\"programmers\":[{\"firstName\":\"Brett\",\"lastName\":\"McLaughlin\",\"email\":\"aaaa\",\"age\":33},{\"firstName\":\"Jason\",\"lastName\":\"Hunter\",\"email\":\"bbbb\",\"age\":25},{\"firstName\":\"Elliotte\",\"lastName\":\"Harold\",\"email\":\"cccc\",\"age\":30}],\"authors\":[{\"firstName\":\"Isaac\",\"lastName\":\"Asimov\",\"genre\":\"sciencefiction\",\"age\":53},{\"firstName\":\"Tad\",\"lastName\":\"Williams\",\"genre\":\"fantasy\",\"age\":47},{\"firstName\":\"Frank\",\"lastName\":\"Peretti\",\"genre\":\"christianfiction\",\"age\":28}],\"musicians\":[{\"firstName\":\"Eric\",\"lastName\":\"Clapton\",\"instrument\":\"guitar\",\"age\":19},{\"firstName\":\"Sergei\",\"lastName\":\"Rachmaninoff\",\"instrument\":\"piano\",\"age\":26}]}";
	json_object item = json_parse(json,0,0);
	print(&item, 0, 0);
	wprintf(L"\n");
	wchar_t *text = json_serialize(&item);
	wprintf(L"%ls\n", text);
	json_object_free(&item);
	free(text);
	system("pause");
 	return 0;
}
