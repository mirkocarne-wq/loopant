// Usa segreti.h se esiste, altrimenti le chiavi di esempio (con avviso in compilazione).
#pragma once
#if __has_include("segreti.h")
#include "segreti.h"
#else
#warning "segreti.h mancante: uso le chiavi di ESEMPIO. Copia segreti.esempio.h in segreti.h e cambia le chiavi."
#include "segreti.esempio.h"
#endif
