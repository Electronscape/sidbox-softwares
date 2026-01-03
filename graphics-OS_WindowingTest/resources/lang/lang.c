#include "lang.h"

#include <stdbool.h>


const char **lang_active;
static bool lang_ready = false;


void lang_set_en(void) { lang_active = lang_en; }
void lang_set_fr(void) { lang_active = lang_fr; }


void init_language(void){
	lang_set_en();
	//lang_set_fr();
	lang_ready = true;
}

const char* lang_get(lang_id_t id) {
	if (!lang_ready) return "LANG_NOT_INIT";  // or assert/panic in debug
	if ((unsigned) id >= STR_COUNT) return "<INVALID>";
	const char *s = lang_active[id];
	return s ? s : "<MISSING>";
}
