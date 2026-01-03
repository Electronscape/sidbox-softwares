#ifndef LANGUAGE_H
#define LANGUAGE_H

#ifdef __cplusplus
extern "C" {
#endif

//#include "main.h"
//#include <stdint.h>


typedef enum {
	#define X(id, text) id,
	#include "en.def"
	#undef X
    STR_COUNT
} lang_id_t;


extern const char *lang_en[];
extern const char *lang_fr[];
extern const char **lang_active;

void init_language(void);
const char* lang_get(lang_id_t id);


#ifdef __cplusplus
}
#endif

#endif
