#include "lang.h"

const char *lang_en[STR_COUNT] = {
#define X(id, text) [id] = text,
#include "en.def"
#undef X
};


