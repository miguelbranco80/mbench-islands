#ifndef CONFIG_H_
#define CONFIG_H_

int read_config_file(char *file);

int get_config_value(char *key, char **val);

int get_int_config_value(char *key, int *val);

int get_ulong_config_value(char *key, unsigned long long int *val);

#endif			/* CONFIG_H_ */
