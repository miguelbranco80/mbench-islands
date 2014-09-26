#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct cfg_entry_s cfg_entry_t;
struct cfg_entry_s {
	char k[80];
	char v[80];
	cfg_entry_t* next;
};

static cfg_entry_t *head = NULL;
static cfg_entry_t *tail = NULL;

int
read_config_file(char *file)
{
	if (head)
		return -1;
	
	FILE *f = fopen(file, "r");
	char k[80], v[80];

	while (fscanf(f, "%[a-zA-Z0-9_]%*[= ]%[-0-9 ]", k, v) != EOF) {

		cfg_entry_t* e = malloc(sizeof(cfg_entry_t));
		if (!e)
			return -1;
		e->next = NULL;
		
		strncpy(e->k, k, 80);
		strncpy(e->v, v, 80);
		
		if (!head)
			head = e;
		else
			tail->next = e;
		tail = e;

		if (fgetc(f) == EOF)
			break;
	}
	fclose(f);

	return 0;
}

int
get_config_value(char* key, char **val)
{
	cfg_entry_t *e;
	for (e = head; e != NULL; e = e->next)
		if (strcmp(e->k, key) == 0)
			*val = e->v;
			return 0;
	return -1;
}

int
get_int_config_value(char *key, int *val)
{
	char *str;
	if (get_config_value(key, &str) == 0) {
		*val = atoi(str);
		return 0;
	}
	return -1;
}

int
get_ulong_config_value(char *key, unsigned long long int *val)
{
	char *str;
	if (get_config_value(key, &str) == 0) {
		*val = atoll(str);
		return 0;
	}
	return -1;
}
