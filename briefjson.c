#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "briefJson.h"

typedef struct
{
	json_object data;
	wchar_t *json;
	wchar_t *pos;
	wchar_t *message;
}parse_engine;

typedef struct strlist
{
	size_t length;
	struct strlist *next;
	union
	{
		wchar_t text[1];
		struct strlist *last;
	}v;
}strlist;

static size_t strlist_append(strlist *des, const wchar_t *src, size_t length)
{
	if (!des->next)
		des->v.last = des;
	strlist *str = (strlist *)malloc(sizeof(strlist) + sizeof(wchar_t)*length);
	memcpy(str->v.text, src, sizeof(wchar_t)*length);
	str->next = 0;
	str->length = length;
	des->length += length;
	des->v.last->next = str;
	des->v.last = str;
	return des->length;
}

static void strlist_free(strlist *list)
{
	while (list->next)
	{
		strlist *tmp = list->next;
		list->next = list->next->next;
		free(tmp);
	}
}

static wchar_t *strlist_to_string(strlist *list)
{
	wchar_t *text = (wchar_t *)malloc(sizeof(wchar_t)*(list->length + 1));
	text[list->length] = 0;
	wchar_t *pos = text;
	strlist* str = list->next;
	while (str)
	{
		wcsncpy(pos, str->v.text, str->length);
		pos += str->length;
		str = str->next;
	}
	strlist_free(list);
	return text;
}

static json_object* insert_item(json_object *list, wchar_t *key)
{
	json_object *item;
	if (key) 
	{
		size_t len = wcslen(key);
		item = (json_object *)malloc(sizeof(json_object) + sizeof(wchar_t)*len);
		item->key[len] = 0;
		wcsncpy(item->key, key, len);
	}else item = (json_object *)malloc(sizeof(json_object));
	item->type = NONE;
    item->next = 0;
    if(!list->value.item) list->value.item=item;
    else for(json_object *i=list->value.item;;i=i->next)
        if(!i->next)
        {
            i->next = item;
            break;
        }
    return item;
}

static wchar_t next_token(parse_engine* engine) {
	wchar_t ch;
    while ((ch = *engine->pos++) <= ' '&&ch>0);
	return *(engine->pos - 1);
}

static int parsing(parse_engine* engine, json_object *pos_parse)
{
	wchar_t c = next_token(engine);
	switch (c)
	{
	case 0:
		engine->message = (wchar_t *)L"Unexpected end";
		return 1;

	case '[':
	{
		pos_parse->type = ARRAY;
		pos_parse->value.item = 0;
		if (next_token(engine) == ']') return 0;
		--engine->pos;
		while (1)
		{
			json_object *item = insert_item(pos_parse, 0);
			if (next_token(engine) == ',')
			{
				--engine->pos;
			}
			else 
			{
				--engine->pos;
				if (parsing(engine, item))
					return 1;
			}
			switch (next_token(engine))
			{
			case ',':
				if (next_token(engine) == ']')
					return 0;
				--engine->pos;
				break;
			case ']':
				return 0;
			default:
				engine->message = (wchar_t *)L":Expected a ',' or ']'";
				return 1;
			}
		}
	}

	case '{':
	{
		pos_parse->type = TABLE;
		pos_parse->value.item = 0;
		if (next_token(engine) == '}') return 0;
		--engine->pos;
		while (1)
		{
			json_object key;
			if (parsing(engine, &key) || key.type != TEXT)
			{
				json_object_free(&key);
				engine->message = (wchar_t *)L"Illegal key of pair";
				return 1;
			}
			if (next_token(engine) != ':')
			{
				engine->message = (wchar_t *)L"Expected a ':'";
				return 1;
			}
			json_object* item=insert_item(pos_parse, key.value.text);
			json_object_free(&key);
			if (parsing(engine, item))
			{
				return 1;
			}
			switch (next_token(engine))
			{
			case ';':
			case ',':
				if (next_token(engine) == '}')
					return 0;
				--engine->pos;
				break;
			case '}':
				return 0;
			default:
				engine->message = (wchar_t *)L"Expected a ',' or '}'";
				return 1;
			}
		}
	}

	case '\'':
	case '"':
	{
		pos_parse->type = TEXT;
		pos_parse->value.text = 0;
		strlist str = { 0 };
		while (1)
		{
			wchar_t ch = *engine->pos++;
			switch (ch)
			{
			case '\n':
			case '\r':
				strlist_free(&str);
				engine->message = (wchar_t *)L"Unterminated string";
				return 1;
			case '\\':
				ch = *engine->pos++;
				switch (ch)
				{
				case 'b':
					strlist_append(&str, L"\b", 1);
					break;
				case 't':
					strlist_append(&str, L"\t", 1);
					break;
				case 'n':
					strlist_append(&str, L"\n", 1);
					break;
				case 'f':
					strlist_append(&str, L"\f", 1);
					break;
				case 'r':
					strlist_append(&str, L"\r", 1);
					break;
				case '"':
				case '\'':
				case '\\':
				case '/':
					strlist_append(&str, &ch, 1);
					break;
				case 'u': {
					wchar_t num = 0;
					for (int i = 0; i < 4; ++i)
					{
						wchar_t tmp = *engine->pos++;
						if (tmp >= '0'&&tmp <= '9')
							tmp = tmp - '0';
						else if (tmp >= 'A'&&tmp <= 'F')
							tmp = tmp - ('A' - 10);
						else if (tmp >= 'a'&&tmp <= 'f')
							tmp = tmp - ('a' - 10);
						num = num << 4 | tmp;
					}
					strlist_append(&str, &num, 1);
					break;
				}
				default:
					strlist_free(&str);
					engine->message = (wchar_t *)L"Illegal escape";
					return 1;
				}
				break;
			default:
				if (ch == c)
				{
					pos_parse->value.text = strlist_to_string(&str);
					strlist_free(&str);
					return 0;
				}
				strlist_append(&str, &ch, 1);
				break;
			}
		}
		break;
	}
	}
    int length = 0;
    char buffer[32] = { 0 };
    while (c >= ' ') {
        if(strchr(",:]}/\\\"[{;=#", c))
            break;
        if(length<sizeof(buffer)&&strchr("0123456789+-.AEFLNRSTUaeflnrstu",c))
        {
            buffer[length++]=(char)c;
            c = *engine->pos++;
        }
        else{
            engine->message=(wchar_t *)L"Illegal Value";
            return 1;
        }
	}
    --engine->pos;
	if (!length)
	{
		pos_parse->type = TEXT;
		pos_parse->value.text = (wchar_t *)malloc(sizeof(wchar_t));
		pos_parse->value.text[0] = 0;
		return 0;
	}
	else {
		if (!strcmp(buffer, "TRUE") || !strcmp(buffer, "true"))
		{
			pos_parse->type = BOOLEAN;
			pos_parse->value.boolean = true;
			return 0;
		}
		else if (!strcmp(buffer, "FALSE") || !strcmp(buffer, "false"))
		{
			pos_parse->type = BOOLEAN;
			pos_parse->value.boolean = false;
			return 0;
		}
		else if (!strcmp(buffer, "NULL") || !strcmp(buffer, "null"))
		{
			pos_parse->type = NONE;
			return 0;
		}
		pos_parse->type = (strstr(buffer, ".") || strstr(buffer, "e") || strstr(buffer, "E")) ? DECIMAL : INTEGER;
		char *format = pos_parse->type == INTEGER ? "%lld" : "%lf";
		if (sscanf(buffer, format, &pos_parse->value)) return 0;
		engine->message = (wchar_t *)L"Unexpected end";
		return 1;
	}
}

static void object_to_string(json_object *data, strlist *head)
{
    switch (data->type) {
        case NONE:
        {
            strlist_append(head, L"null", 4);
            break;
        }
        case INTEGER:
        case DECIMAL:
        {
            char tmp[32] = { 0 };
            wchar_t buffer[32] = { 0 };
            const char *format = data->type == INTEGER ? "%lld" : "%lf";
            sprintf(tmp, format, data->value);
            size_t len = strlen(tmp);
            for (size_t i = 0; i < len; ++i)
                buffer[i] = tmp[i];
            strlist_append(head, buffer, len);
            break;
        }
        case BOOLEAN:
        {
            int len = data->value.boolean ? 4 : 5;
            const wchar_t *value = data->value.boolean ? L"true" : L"false";
            strlist_append(head, value, len);
            break;
        }
        case TEXT:
        {
            strlist_append(head, L"\"", 1);
            strlist_append(head, data->value.text, wcslen(data->value.text));
            strlist_append(head, L"\"", 1);
            break;
        }
        case TABLE:
        {
            json_object *item = data->value.item;
            int index = 0;
            while (item)
            {
                if(index) strlist_append(head, L",\"", 2);
                else strlist_append(head, L"{\"", 2);
                strlist_append(head, item->key, wcslen(item->key));
                strlist_append(head, L"\":", 2);
                object_to_string(item,head);
                
                item = item->next;
                ++index;
            }
            strlist_append(head, L"}", 1);
            break;
        }
        case ARRAY:
        {
            json_object *item = data->value.item;
            int index = 0;
            while (item)
            {
                strlist_append(head, index ? L"," : L"[", 1);
                object_to_string(item, head);
                item = item->next;
                ++index;
            }
            strlist_append(head, L"]", 1);
            break;            
        }
    }
}

json_object json_parse(wchar_t json[],wchar_t **message,long* error_pos)
{
	parse_engine result;
	result.data.type = NONE;
	result.pos = json;
	result.json = json;
	if (parsing(&result, &result.data))
	{
		if(message)		*message = result.message;
		json_object_free(&result.data);
		if(error_pos)		*error_pos = result.pos-result.json;
		json_object null_item;
		null_item.type = NONE;
		return null_item;
	}
	if(message)	*message = (wchar_t *)L"SUCCEED";
	if(error_pos) *error_pos = -1;
	return result.data;
}

wchar_t *json_serialize(json_object *data)
{
	strlist head = { 0 };
	object_to_string(data, &head);
	wchar_t *string = strlist_to_string(&head);	
	return string;
}

void json_object_free(json_object *data)
{
    if (data->type == ARRAY || data->type == TABLE)
        while (data->value.item)
        {
            json_object *item = data->value.item;
            data->value.item = item->next;
            json_object_free(item);
            free(item);
        }
    else if (data->type == TEXT)
        free(data->value.text);
}

void json_text_free(wchar_t json[])
{
	free(json);
}
