/* drax Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __DIO
#define __DIO

char* b_pwd();

char* normalize_path(char* bp, char* path);

int get_file_content(char* bp, char* name, char** content);

#endif
