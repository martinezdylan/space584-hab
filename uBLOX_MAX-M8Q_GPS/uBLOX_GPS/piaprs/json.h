typedef void (*json_callback)(char* a, char* b, void *c);
void json_parse(char *filename, json_callback handler, void *param);
