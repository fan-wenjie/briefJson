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

#define BUFFER_SIZE 1024

typedef struct string_node
{
	size_t size;
	struct string_node *next;
	wchar_t string[BUFFER_SIZE];
}string_node;

typedef struct
{
	size_t size;
	string_node *next;
	string_node *last;
}string_buffer;

static void buffer_append(string_buffer *buffer, wchar_t* string,size_t length)
{
	if (!buffer->last)
	{
		string_node *node = (string_node *)malloc(sizeof(string_node));
		node->next = 0;
		buffer->size = node->size = 0;
		buffer->next = buffer->last = node;
	}
	wchar_t *pos = string;
	while (length)
	{
		size_t copysize = length;
		string_node *copynode = buffer->last;
		if (copynode->size + length > BUFFER_SIZE)
		{
			copysize = BUFFER_SIZE - copynode->size;
			string_node *node = (string_node *)malloc(sizeof(string_node));
			node->next = 0;
			node->size = 0;
			copynode->next = node;
			buffer->last = node;
		}
		wcsncpy(copynode->string + copynode->size, pos, copysize);
		pos += copysize;
		length -= copysize;
		buffer->size += copysize;
		copynode->size += copysize;
	}
}

static wchar_t *buffer_tostr(string_buffer *buffer)
{
	wchar_t *string = (wchar_t *)malloc(sizeof(wchar_t)*(buffer->size + 1));
	wchar_t *pos = string;
	string[buffer->size] = 0;
	while (buffer->next)
	{
		string_node *node = buffer->next;
		buffer->next = node->next;
		wcsncpy(pos, node->string, node->size);
		pos += node->size;
		free(node);
	}
	return string;
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
			json_object* item = insert_item(pos_parse, key.value.text);
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
		string_buffer sb = { 0 };
		while (1)
		{
			wchar_t ch = *engine->pos++;
			switch (ch)
			{
			case L'\n':
			case L'\r':
				free(buffer_tostr(&sb));
				engine->message = (wchar_t *)L"Unterminated string";
				return 1;
			case L'\\':
				ch = *engine->pos++;
				switch (ch)
				{
				case L'b':
					buffer_append(&sb, L"\b", 1);
					break;
				case L't':
					buffer_append(&sb, L"\t", 1);
					break;
				case L'n':
					buffer_append(&sb, L"\n", 1);
					break;
				case L'f':
					buffer_append(&sb, L"\f", 1);
					break;
				case L'r':
					buffer_append(&sb, L"\r", 1);
					break;
				case L'"':
				case L'\'':
				case L'\\':
				case L'/':
					buffer_append(&sb, &ch, 1);
					break;
				case L'u': {
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
					buffer_append(&sb, &num, 1);
					break;
				}
				default:
					free(buffer_tostr(&sb));
					engine->message = (wchar_t *)L"Illegal escape";
					return 1;
				}
				break;
			default:
				if (ch == c)
				{
					pos_parse->value.text = buffer_tostr(&sb);
					return 0;
				}
				buffer_append(&sb, &ch, 1);
				break;
			}
		}
		break;
	}
	}
	int length = 0;
	char buffer[32] = { 0 };
	while (c >= ' ') {
		if (strchr(",:]}/\\\"[{;=#", c))
			break;
		if (length<sizeof(buffer) && strchr("0123456789+-.AEFLNRSTUaeflnrstu", c))
		{
			buffer[length++] = (char)c;
			c = *engine->pos++;
		}
		else {
			engine->message = (wchar_t *)L"Illegal Value";
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
		const char *format = pos_parse->type == INTEGER ? "%lld" : "%lf";
		if (sscanf(buffer, format, &pos_parse->value)) return 0;
		engine->message = (wchar_t *)L"Unexpected end";
		return 1;
	}
}

static void object_to_string(json_object *data, string_buffer *head)
{
	switch (data->type) {
	case NONE:
	{
		buffer_append(head, L"null", 4);
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
		buffer_append(head, buffer, len);
		break;
	}
	case BOOLEAN:
	{
		if (data->value.boolean)
			buffer_append(head, L"true", 4);
		else
			buffer_append(head, L"false", 5);
		break;
	}
	case TEXT:
	{
		buffer_append(head, L"\"", 1);
		wchar_t *pos = data->value.text;
		wchar_t *end = data->value.text + wcslen(data->value.text);
		while (pos != end)
		{
			switch (*pos++)
			{
			case L'\b':
				buffer_append(head, L"\\b", 2);
				break;
			case L'\t':
				buffer_append(head, L"\\t", 2);
				break;
			case L'\n':
				buffer_append(head, L"\\n", 2);
				break;
			case L'\f':
				buffer_append(head, L"\\f", 2);
				break;
			case L'\r':
				buffer_append(head, L"\\r", 2);
				break;
			default:
				buffer_append(head, pos - 1, 1);
				break;
			}
		}
		buffer_append(head, L"\"", 1);
		break;
	}
	case TABLE:
	{
		json_object *item = data->value.item;
		int index = 0;
		while (item)
		{
			if (index) buffer_append(head, L",", 1);
			else buffer_append(head, L"{", 1);
			json_object key;
			key.type = TEXT;
			key.value.text = item->key;
			object_to_string(&key, head);
			buffer_append(head, L":", 1);
			object_to_string(item, head);

			item = item->next;
			++index;
		}
		buffer_append(head, L"}", 1);
		break;
	}
	case ARRAY:
	{
		json_object *item = data->value.item;
		int index = 0;
		while (item)
		{
			buffer_append(head, index ? L"," : L"[", 1);
			object_to_string(item, head);
			item = item->next;
			++index;
		}
		buffer_append(head, L"]", 1);
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
		if(error_pos)		*error_pos = result.pos - result.json;
		json_object null_item;
		null_item.type = NONE;
		return null_item;
	}
	if(message)	*message = (wchar_t *)L"SUCCEED";
	if(error_pos) *error_pos = 0;
	return result.data;
}

wchar_t *json_serialize(json_object *data)
{
	string_buffer head = { 0 };
	object_to_string(data, &head);
	wchar_t *string = buffer_tostr(&head);	
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
