
#include "lang.h"


const char *lang_fr[STR_COUNT] = {
#define X(id, text) [id] = text,
#include "fr.def"
#undef X
};



