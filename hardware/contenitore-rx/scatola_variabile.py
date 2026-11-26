#!/usr/bin/env python3
"""Scatola del variabile da 1000 pF (antenna RX), da stampare in 3D (CadQuery).

Usa le stesse parti della scatola del motore (`contenitore.py`): fondo, staffa a V per il palo, coperchio con
guarnizione, borchia con paraolio per il tubo isolante. L'asse del variabile sta alla stessa distanza dal palo
dell'asse del motore, quindi i due alberi sono concentrici per costruzione.

Sul palo sta SOPRA la scatola del motore, con la borchia in basso: il tubo isolante sale dal motore al variabile.
La parete in alto è un blocco pieno che regge l'anello di rame: i due capi del tubo entrano in due manicotti e sono
bloccati ciascuno da una vite M4 che lo attraversa (una vite per capo: i capi restano isolati tra loro). Sotto il
blocco la staffa del palo si allunga e prende 2 viti in più, così peso e vento dell'anello vanno dritti al palo.

Tutte le misure del variabile sono STIME DA VERIFICARE (vedi README.md).

Uso:  python3 scatola_variabile.py           -> stl/variabile-*.stl, step/, anteprime/variabile-*.png
"""
import math
import os
import sys

import cadquery as cq

import contenitore as c

QUI = os.path.dirname(os.path.abspath(__file__))

# ============================================================================================================
# PARAMETRI (mm). Stesse coordinate della scatola del motore: X lungo l'asse (0 = parete della borchia, che sul
# palo è in basso), Y in larghezza, Z dal fondo interno verso il coperchio.
# ============================================================================================================

# --- Variabile (da misurare) ---
VAR_L, VAR_W, VAR_H = 135.0, 83.0, 95.0  # corpo: lunghezza, larghezza, altezza (misure dell'utente, circa)
H_ASSE_VAR = c.H_ASSE_VARIABILE        # asse sopra la base del variabile (misura dell'utente: 45 mm)
VAR_ALBERO_D, VAR_ALBERO_L = 6.0, 29.0  # perno di comando ("maniglia" circa 29 mm); diametro da verificare
VAR_PESO = 0.6                          # kg
X_GIUNTO = 8.0                          # davanti del giunto dalla parete interna
SPAZIO_DIETRO = 20.0                    # dietro il variabile: perni degli statori che sporgono, capi, piattine

# --- Anello di rame ---
TUBO_ANELLO = 12.7               # tubo dell'anello: 1/2" (12,7) o 5/8" (15,9)
INTERASSE_CAPI = 70.0            # distanza tra i due capi del tubo (da verificare sull'anello)
Z_CAPI = 85.0                    # distanza dei capi dal fondo interno, vicino ai morsetti in alto del variabile
BLOCCO_S = 20.0                  # spessore del blocco che regge l'anello (oltre la parete)
TUBO_DENTRO = 10.0               # quanto i capi entrano nella scatola oltre il blocco
COLLARE = 4.0                    # collare attorno a ogni capo, sopra il blocco (l'acqua non resta sul giunto)
VITE_CAPO = 4.3                  # vite M4 di traverso: foro passante fino al tubo...
VITE_CAPO_FILETTO = 3.3          # ...e oltre il tubo foro per filettare la plastica

# --- Scatola ---
LY = 105.0                       # più stretta di quella del motore (indicazione dell'utente): 11 mm per lato
X_VAR = X_GIUNTO + c.GIUNTO_L / 2 + VAR_ALBERO_L   # faccia del variabile da cui esce l'albero
LX = float(math.ceil(X_VAR + VAR_L + SPAZIO_DIETRO))
# Fissaggio del variabile: 4 fori filettati M5 sulle linguette delle fiancate, 114 mm lungo l'asse e 97 mm tra le
# linguette di sotto e quelle di sopra (misure dell'utente). Si usano le due di sotto: viti M5 dal basso attraverso la
# piastra, testa nello spazio sotto la piastra.
FORI_VAR = 114.0                 # interasse lungo l'asse tra le due linguette di sotto
FORI_VAR_ALTEZZA = 97.0          # tra linguette di sotto e di sopra (non usate per il fissaggio)
ASOLA_VAR = 6.0                  # gioco lungo l'asse delle asole M5
ALZO_LINGUETTE = 0.0             # rialzo sotto le linguette, se i tiranti in basso sporgono più giù (da misurare)

SCRITTA = "loopant C"


def imposta():
    """Adatta i parametri condivisi di contenitore.py a questa scatola."""
    c.LX, c.LY = LX, LY
    c.Y_ASSE = LY / 2
    # colonnine più alte: la base del variabile va dove il suo asse cade all'altezza comune (ASSE_Z)
    c.COLONNINA_H = c.ASSE_Z - H_ASSE_VAR - c.PIASTRA - ALZO_LINGUETTE
    c.HZ = float(math.ceil(max(c.COLONNINA_H + c.PIASTRA + VAR_H, Z_CAPI + TUBO_ANELLO / 2 + 6)
                           + c.BORDO_H + c.BORDO + 2))
    c.COLONNINE = [(22, 22), (LX - 22, 22), (22, LY - 22), (LX - 22, LY - 22)]
    c.FORI_FRONTE = [("sfiato M12", LY / 2 + 28, 22.0, c.SFIATO)]   # in basso sul palo
    c.FORI_RETRO = []
    # Staffa: si allunga sotto il blocco dell'anello; V vicino al blocco, viti fuori dalle V
    xb = LX + c.PARETE + BLOCCO_S
    c.STAFFA_X = (14.0, xb - 3.0)
    c.BLOCCHI_X = [45.0, LX - 15.0]
    c.VITI_STAFFA = [(24.0, LY / 2 - 28), (24.0, LY / 2 + 28), (LX + 13.0, LY / 2 - 28), (LX + 13.0, LY / 2 + 28)]
    c.SCRITTA = SCRITTA
    c.TESTO_ROT, c.TESTO_X = -90.0, 40.0   # la scatola è girata sul palo: scritta dritta, in basso
    c.CON_OLED = False


def capi():
    return [(LY / 2 - INTERASSE_CAPI / 2, Z_CAPI), (LY / 2 + INTERASSE_CAPI / 2, Z_CAPI)]


def scatola():
    s = c.scatola()
    x0 = LX + c.PARETE - 0.1
    x1 = LX + c.PARETE + BLOCCO_S
    zt = c.HZ - 2                    # fin quasi al coperchio: più rigido contro la leva dell'anello
    blocco = c.scatola_arrotondata(x0, -c.PARETE, -c.FONDO, x1, LY + c.PARETE, zt, 4.0)
    for y, z in capi():
        blocco = blocco.union(c.cilindro_x(x1 - 0.1, y, z, COLLARE + 0.1, TUBO_ANELLO + 8))
    s = s.union(blocco)
    for i, (y, z) in enumerate(capi()):
        s = s.cut(c.cilindro_x(LX - 1, y, z, BLOCCO_S + c.PARETE + COLLARE + 2, TUBO_ANELLO + 0.4))
        # svasatura per il cordone di silicone
        svasa = (cq.Workplane("YZ").workplane(offset=x1 + COLLARE - 1.5).center(y, z).circle(TUBO_ANELLO / 2 + 0.2)
                 .workplane(offset=1.6).circle(TUBO_ANELLO / 2 + 1.8).loft())
        s = s.cut(svasa)
        # vite M4 di traverso dal fianco più vicino: passante fino al tubo, poi foro per filettare
        lato = -1 if y < LY / 2 else 1
        xv = (x0 + x1) / 2
        y_fianco = -c.PARETE if lato < 0 else LY + c.PARETE
        y_oltre = y + lato * (TUBO_ANELLO / 2 + 0.2)
        lungo = abs(y_oltre - y_fianco) + 1
        foro = cq.Workplane("XY").cylinder(lungo, VITE_CAPO / 2, direct=(0, 1, 0)).translate(
            (xv, (y_fianco - lato + y_oltre) / 2, z))
        filetto = cq.Workplane("XY").cylinder(10, VITE_CAPO_FILETTO / 2, direct=(0, 1, 0)).translate(
            (xv, y_oltre + lato * 5, z))
        testa = cq.Workplane("XY").cylinder(3, 4.2, direct=(0, 1, 0)).translate((xv, y_fianco - lato * 0.5, z))
        s = s.cut(foro).cut(filetto).cut(testa)
    return s


def fori_variabile():
    """Centri dei due fori M5 delle linguette di sotto (simmetrici rispetto al corpo, al centro della larghezza)."""
    xm = X_VAR + VAR_L / 2
    return [(xm - FORI_VAR / 2, LY / 2), (xm + FORI_VAR / 2, LY / 2)]


def piastra():
    """Piastra con due asole M5 per le linguette del variabile, su 4 colonnine come nel motore."""
    z0 = c.COLONNINA_H
    zt = z0 + c.PIASTRA
    p = c.scatola_arrotondata(c.BORDO + 1, c.BORDO + 1, z0, LX - c.BORDO - 1, LY - c.BORDO - 1, zt, 4.0)
    for x, y in c.COLONNINE:
        p = p.cut(c.cilindro_z(x, y, z0 - 1, c.PIASTRA + 2, 3.4))
        p = p.cut(c.cilindro_z(x, y, zt - 2.0, 4, 6.2))
    for x, y in fori_variabile():
        if ALZO_LINGUETTE > 0:
            p = p.union(cq.Workplane("XY").workplane(offset=zt - 0.1).center(x, y)
                        .slot2D(ASOLA_VAR + 14, 14).extrude(ALZO_LINGUETTE + 0.1))
        p = p.cut(cq.Workplane("XY").workplane(offset=z0 - 1).center(x, y).slot2D(ASOLA_VAR + 5.5, 5.5)
                  .extrude(c.PIASTRA + ALZO_LINGUETTE + 2))
    # fessure per fascette ai lati del variabile (cavetti, eventuale fermo in più)
    ya, yb = LY / 2 - VAR_W / 2 - 1, LY / 2 + VAR_W / 2 + 1
    for x in (X_VAR + 20, X_VAR + VAR_L - 20):
        for y in (ya, yb):
            p = p.cut(cq.Workplane("XY").workplane(offset=z0 - 1).center(x, y).rect(5, 2.2).extrude(c.PIASTRA + 2))
    return p


def ingombri():
    zb = c.COLONNINA_H + c.PIASTRA
    za = c.Z_ASSE
    pezzi = {}
    pezzi["variabile"] = (c.scatola_arrotondata(X_VAR, LY / 2 - VAR_W / 2, zb, X_VAR + VAR_L, LY / 2 + VAR_W / 2,
                                                zb + VAR_H, 2.0), "#b0bec5")
    viti = None
    for x, y in fori_variabile():
        vt = c.cilindro_z(x, y, c.COLONNINA_H - 3.5, 3.5, 8.5).union(c.cilindro_z(x, y, c.COLONNINA_H, 12, 5))
        viti = vt if viti is None else viti.union(vt)
    pezzi["viti M5"] = (viti, "#5f6368")
    pezzi["albero variabile"] = (c.cilindro_x(X_VAR - VAR_ALBERO_L, LY / 2, za, VAR_ALBERO_L, VAR_ALBERO_D), "#bdc1c6")
    pezzi["giunto 8→6"] = (c.cilindro_x(X_GIUNTO, LY / 2, za, c.GIUNTO_L, c.GIUNTO_D), "#c5a572")
    pezzi["tubo isolante"] = (c.cilindro_x(-c.PARETE - c.BOSS_SPORGENZA - 25, LY / 2, za,
                                           25 + c.PARETE + c.BOSS_SPORGENZA + X_GIUNTO + c.GIUNTO_L / 2,
                                           c.TUBO_ISOLANTE), "#f1f3f4")
    for i, (y, z) in enumerate(capi()):
        x_in = LX - TUBO_DENTRO
        pezzi[f"capo {i + 1}"] = (c.cilindro_x(x_in, y, z, TUBO_DENTRO + c.PARETE + BLOCCO_S + COLLARE + 60,
                                               TUBO_ANELLO), "#b87333")
        # piattina verso lo statore
        yv = LY / 2 + (VAR_W / 2 - 3) * (1 if y > LY / 2 else -1)
        y0, y1 = sorted((y, yv))
        pezzi[f"piattina {i + 1}"] = (c.scatola_arrotondata(x_in, y0 - 1, z - 6, x_in + 2, y1 + 1, z + 6, 0),
                                      "#b87333")
    return pezzi


def controlla(sc, pi, pezzi):
    errori = []
    interno = c.scatola_arrotondata(0, 0, c.COLONNINA_H + c.PIASTRA, LX, LY, c.HZ - c.BORDO_H - c.BORDO, 2.0)
    for nome in ("variabile",):
        if pezzi[nome][0].cut(interno).val().Volume() > 1:
            errori.append(f"{nome}: fuori dallo spazio utile")
        if pezzi[nome][0].intersect(sc).val().Volume() > 1 or pezzi[nome][0].intersect(pi).val().Volume() > 1:
            errori.append(f"{nome}: tocca scatola o piastra")
    for i in (1, 2):
        if pezzi[f"capo {i}"][0].intersect(pezzi["variabile"][0]).val().Volume() > 1:
            errori.append(f"capo {i}: tocca il variabile")
    for nome in ("capo 1", "capo 2"):
        if pezzi[nome][0].intersect(sc).val().Volume() > 1:
            errori.append(f"{nome}: il foro non è passante")
    lato = min(LY / 2 - VAR_W / 2, LY / 2 - VAR_W / 2)
    print(f"Variabile: asse a {c.Z_ASSE:.1f} mm dal fondo interno (come il motore), {lato:.0f} mm dalle pareti laterali")
    tubo = (X_GIUNTO + c.GIUNTO_L / 2) * 2 + 2 * (c.PARETE + c.BOSS_SPORGENZA)
    print(f"Tubo isolante: circa {tubo:.0f} mm più lo spazio tra le due borchie")
    return errori


def main():
    imposta()
    sc = scatola()
    pi = piastra()
    st = c.staffa()
    zc = c.HZ + c.GUARNIZIONE
    co = c.coperchio(zc)
    gu = c.guarnizione(c.HZ)
    pezzi = ingombri()
    errori = controlla(sc, pi, pezzi)
    if st.intersect(sc).val().Volume() > 1:
        errori.append("la staffa entra nella scatola")
    for e in errori:
        print("ATTENZIONE:", e)
    c.QUI = QUI
    c.esporta({
        "variabile-scatola": c.sul_piatto(sc),
        "variabile-piastra": c.sul_piatto(pi),
        "variabile-coperchio": c.sul_piatto(co.rotate((0, 0, 0), (1, 0, 0), 180)),
        "variabile-guarnizione": c.sul_piatto(gu),
        "variabile-staffa-palo": c.sul_piatto(st.rotate((0, 0, 0), (1, 0, 0), 180)),
    })
    if "--niente-anteprime" not in sys.argv:
        anteprime(sc, pi, st, co, gu, pezzi)
    return 1 if errori else 0


def anteprime(sc, pi, st, co, gu, pezzi):
    import numpy as np
    import anteprime as a
    cart = os.path.join(QUI, "anteprime")
    t = a.triangoli
    finti = [(t(o), col) for o, col in pezzi.values()]
    interno = [(t(sc), a.COLORI["scatola"]), (t(pi), a.COLORI["piastra"]), (t(st), a.COLORI["staffa"])] + finti
    a.figura(interno, os.path.join(cart, "variabile-interno.png"), 42, -135,
             "Scatola del variabile: variabile sulla piastra a griglia, capi dell'anello nel blocco in alto")
    sez = [(t(a.taglia(o, LY / 2)), col) for o, col in [(sc, a.COLORI["scatola"]), (pi, a.COLORI["piastra"]),
                                                        (st, a.COLORI["staffa"])]]
    sez += [(t(a.taglia(o, LY / 2)), col) for o, col in pezzi.values()]
    a.figura(sez, os.path.join(cart, "variabile-sezione.png"), 8, 90,
             "Sezione sull'asse: giunto 8→6, variabile, blocco dell'anello sopra la staffa allungata")

    # Insieme sul palo: scatola del motore sotto (borchia in alto), scatola del variabile sopra (borchia in basso)
    import importlib
    importlib.reload(c)
    sc_m, st_m = c.scatola(), c.staffa()
    co_m = c.coperchio(c.HZ + c.GUARNIZIONE)
    pezzi_m = c.ingombri()
    ly_m = c.LY
    palo = c.cilindro_x(-450, ly_m / 2, c.centro_palo(), 950, c.PALO_D)
    spazio = 12.0                                  # tra le facce delle due borchie
    off = 2 * (c.PARETE + c.BOSS_SPORGENZA) + spazio

    def motore(tri):                               # come anteprime.installato: parete frontale in alto
        return np.stack([tri[..., 2], tri[..., 1], -tri[..., 0]], axis=-1)

    def variabile(tri):                            # girata: borchia in basso, stesso asse, stessa distanza dal palo
        return np.stack([tri[..., 2], ly_m / 2 - (tri[..., 1] - LY / 2), tri[..., 0] + off], axis=-1)

    gruppi = [(motore(t(o)), col) for o, col in [(sc_m, a.COLORI["scatola"]), (co_m, a.COLORI["coperchio"]),
                                                 (st_m, a.COLORI["staffa"]), (palo, a.COLORI["palo"]),
                                                 (pezzi_m["tubo isolante"][0], "#f1f3f4")]]
    gruppi += [(variabile(t(o)), col) for o, col in [(sc, a.COLORI["scatola"]), (co, a.COLORI["coperchio"]),
                                                     (st, a.COLORI["staffa"])] +
               [(pezzi[n][0], pezzi[n][1]) for n in ("capo 1", "capo 2", "tubo isolante")]]
    a.figura(gruppi, os.path.join(cart, "variabile-sul-palo.png"), 12, -35,
             "Sul palo: motore sotto, variabile sopra, tubo isolante tra le due borchie, capi dell'anello in cima")

if __name__ == "__main__":
    sys.exit(main())
