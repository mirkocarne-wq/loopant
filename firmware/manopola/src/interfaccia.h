// Interfaccia della manopola sul display rotondo 360x360 (LVGL, task sul core 1).
// Manopola: ritocco fine (scatti -> JOG dal pannello) oppure frequenza. Tocco breve: cambia modo.
// Tocco lungo nel modo Frequenza: TUNE sulla frequenza mostrata (poi si torna al ritocco fine).
#pragma once

namespace interfaccia {

void crea();  // con il lock di LVGL

}  // namespace interfaccia
