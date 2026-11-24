#!/usr/bin/env python3
"""Contenitore stampabile dell'unità antenna RX (CadQuery).

Dentro: SERVO42D (NEMA17 con il driver sul retro) + riduttore planetario 20:1 fissati con una squadra a 90°
alla piastra interna, millefori con ESP32 DevKit, SN65HVD230 e convertitore 24→5 V, display OLED 0,96"
sotto il coperchio, dietro una finestra. Il motore sta sull'asse centrale della scatola: l'albero esce al centro
della parete frontale (tubo isolante da 8 mm, con paraolio) verso la scatola del variabile, che resta separata.
Fissaggio al palo con una staffa a V e fascette stringitubo: palo verticale, fondo contro il palo, parete frontale
(albero) in alto, pressacavo dei 24 V e sfiato in basso, coperchio con l'OLED verso chi guarda.

Tutte le misure dei componenti sono STIME DA VERIFICARE con il calibro (vedi README.md).

Uso:  python3 contenitore.py           -> stl/, step/ e anteprime/ (PNG)
      python3 contenitore.py --niente-anteprime
"""
import math
import os
import sys

import cadquery as cq

QUI = os.path.dirname(os.path.abspath(__file__))

# ============================================================================================================
# PARAMETRI (mm). Coordinate: origine nell'angolo interno della scatola, sul piano del fondo;
# X lungo l'asse del motore (0 = parete frontale, quella dell'albero), Y in larghezza, Z in altezza.
# ============================================================================================================

# --- Gruppo motore: lunghezza complessiva (misura dell'utente, circa) ---
LUNGHEZZA_GRUPPO = 150.0         # dal davanti del giunto al retro del SERVO42D: giunto, riduttore, squadra, motore
X_GIUNTO = 12.0                  # spazio tra la parete frontale interna e il davanti del giunto
SPAZIO_CAVI = 27.0               # dietro il SERVO42D, per i connettori

# --- Scatola ---
LX = float(math.ceil(X_GIUNTO + LUNGHEZZA_GRUPPO + SPAZIO_CAVI))  # lunghezza interna, dal gruppo motore
LY = 135.0                       # larghezza interna; l'altezza HZ si calcola dal motore
PARETE = 3.0                     # spessore delle pareti
FONDO = 3.0                      # spessore del fondo
R_ESTERNO = 6.0                  # raggio degli spigoli verticali
BORDO = 5.0                      # sporgenza verso l'interno del bordo superiore (appoggio della guarnizione)
BORDO_H = 8.0                    # altezza del bordo
INSERTO_D, INSERTO_P = 4.0, 7.0  # foro per inserti a caldo M3 (Ø 4,0 per inserti M3×5,7 Ø esterno 4,6)

# --- Coperchio e guarnizione ---
COPERCHIO = 4.0                  # spessore
GUARNIZIONE = 1.5                # spessore della guarnizione piatta (TPU 85A–95A o foglio di silicone)
GONNA_H, GONNA_S = 5.0, 1.8      # bordino di centraggio che entra nell'apertura
VITE_PASSANTE = 3.4              # fori per viti M3 nel coperchio e nella guarnizione

# --- Piastra interna (stampata a parte, avvitata a 4 colonnine del fondo) ---
COLONNINA_H = 6.0                # spazio sotto la piastra per i dadi delle viti della squadra
PIASTRA = 4.0
COLONNINE = [(22, 22), (LX - 22, 22), (22, LY - 22), (LX - 22, LY - 22)]

# --- Gruppo motore (da misurare) ---
Y_ASSE = LY / 2                  # motore sull'asse centrale della scatola
H_ASSE = 30.0                    # altezza dell'asse sopra la base della squadra (dipende dalla squadra)
# Dislivello tra variabile e motore appoggiati sullo stesso piano: l'asse del variabile è 20 mm più alto.
# Il motore sale dello stesso tanto su un rialzo della piastra, così l'albero e il variabile restano concentrici
# (con la scatola del variabile fatta allo stesso modo: stessa staffa, stesso fondo, stessa piastra).
# Con un valore negativo il rialzo va sotto il variabile e qui resta 0.
DISLIVELLO = 20.0
RIALZO_MOTORE = max(DISLIVELLO, 0.0)
# La squadra sta tra riduttore e motore: il riduttore è avvitato davanti (verso la parete frontale), il motore
# dietro, e l'albero del motore passa nel foro della squadra.
SQUADRA_VERSO = -1               # base della squadra: -1 verso il riduttore (com'è la squadra), 1 sotto il motore
SQUADRA_S = 3.0                  # spessore della squadra
SQUADRA_BASE = 35.0              # profondità della base della squadra
MOTORE_L, NEMA = 40.0, 42.3
DRIVER_L = 22.0                  # SERVO42D con il suo guscio sul retro del motore
ALBERO_D, ALBERO_L = 8.0, 20.0   # albero di uscita del riduttore
GIUNTO_D, GIUNTO_L = 20.0, 25.0  # giunto: metà sull'albero, metà sul tubo isolante
RIDUTTORE_D = 36.0
# Il riduttore prende la lunghezza che resta del gruppo (ripartizione da verificare)
RIDUTTORE_L = LUNGHEZZA_GRUPPO - (GIUNTO_L / 2 + ALBERO_L + SQUADRA_S + MOTORE_L + DRIVER_L)
X_RIDUTTORE = X_GIUNTO + GIUNTO_L / 2 + ALBERO_L  # faccia d'uscita del riduttore
X_SQUADRA = X_RIDUTTORE + RIDUTTORE_L  # faccia della squadra verso il riduttore
# Base della squadra lungo X; con la base sotto il riduttore il motore sbalza e serve l'appoggio
BASE_SQUADRA = ((X_SQUADRA, X_SQUADRA + SQUADRA_BASE) if SQUADRA_VERSO > 0
                else (X_SQUADRA + SQUADRA_S - SQUADRA_BASE, X_SQUADRA + SQUADRA_S))
SUPPORTO_MOTORE = SQUADRA_VERSO < 0  # appoggio sotto il corpo del NEMA17, con fessure per una fascetta
Z_ASSE = COLONNINA_H + PIASTRA + RIALZO_MOTORE + H_ASSE   # asse del motore sopra il fondo interno
HZ = float(math.ceil(Z_ASSE + NEMA / 2 + BORDO_H + BORDO + 2))  # altezza interna: il motore passa sotto il bordo
_XA = (BASE_SQUADRA[0] + BASE_SQUADRA[1]) / 2
ASOLE_SQUADRA = [(_XA, Y_ASSE - 13), (_XA, Y_ASSE + 13)]  # centri delle asole
ASOLA_L, ASOLA_W = 20.0, 4.5     # asole lungo X per viti M4 (permettono di spostare la squadra)

# --- Passaggio dell'albero (parete frontale) ---
BOSS_R, BOSS_SPORGENZA = 13.0, 6.0
TUBO_ISOLANTE = 8.0              # tubo isolante tra il giunto del riduttore e il variabile
GIOCO_FORO = 0.4                 # i fori stampati vengono più stretti: 8 mm nominali -> 8,4 nel modello
FORO_ALBERO = TUBO_ISOLANTE + GIOCO_FORO
PARAOLIO_D, PARAOLIO_P = 16.2, 5.0  # sede per paraolio 8×16×5 (labbro verso l'esterno)

# --- Pressacavi e sfiato ---
PG7 = 12.5                       # foro per pressacavo PG7 (cavo 3–6,5 mm)
SFIATO = 12.3                    # valvola di sfiato a membrana M12×1,5
FORI_FRONTE = [("PG7 finecorsa", LY - 35, 30.0, PG7)]                                # (nome, y, z, Ø)
FORI_RETRO = [("PG7 24 V", LY - 35, 28.0, PG7), ("sfiato M12", 35.0, 22.0, SFIATO)]

# --- Fissaggio al palo: staffa stampata a parte, avvitata al fondo, con due blocchi a V e fascette ---
PALO_D = 40.0                    # palo delle anteprime
PALO_D_MAX = 50.0                # la V accetta pali da circa 20 mm fino a questo diametro
STAFFA_SP = 5.0                  # spessore della piastra della staffa
STAFFA_LARGH = 76.0              # larghezza (lungo Y)
STAFFA_X = (14.0, LX - 14.0)     # estensione lungo X (lungo il palo)
BLOCCHI_X = [48.0, LX - 48.0]    # centri dei due blocchi a V
BLOCCO_L = 26.0                  # lunghezza di ciascun blocco lungo il palo
TUNNEL = (14.0, 4.0)             # passaggio della fascetta stringitubo inox (banda fino a 12 mm)
VITI_STAFFA = [(24.0, LY / 2 - 28), (24.0, LY / 2 + 28), (LX - 24.0, LY / 2 - 28), (LX - 24.0, LY / 2 + 28)]
INSERTO_M4_D, INSERTO_M4_P = 5.6, 7.0  # inserti a caldo M4 nel fondo, ciechi (il fondo resta chiuso)

# --- Elettronica: due strisce di millefori 35×90 (una 7×9 tagliata a metà), ai lati del motore ---
# A: ESP32 DevKit; B: SN65HVD230 e convertitore 24→5 V (vicino al PG7 dei 24 V)
MILLEFORI = (90.0, 35.0, 1.6)
MILLEFORI_X0 = 45.0
MILLEFORI_Y0 = [BORDO + 2.0, LY - BORDO - 2.0 - 35.0]
MILLEFORI_FORI = 2.5             # distanza dei fori di fissaggio dal bordo
DISTANZIALE_H, DISTANZIALE_R, DISTANZIALE_FORO = 8.0, 3.0, 1.8  # viti M2 autofilettanti

# --- Display OLED 0,96" ELEGOO (SSD1306) sotto il coperchio ---
# Con il palo verticale e la parete frontale in alto, il lato lungo del display va lungo Y e i pin verso X = 0.
OLED_X, OLED_Y = 50.0, LY / 2    # centro della zona visibile
OLED_VISIBILE = (25.0, 14.0)     # apertura, lato lungo × lato corto (zona attiva 21,7×10,9 + margine)
OLED_DY_PCB = 1.5                # centro del circuito rispetto alla zona attiva, verso i pin (da verificare)
OLED_FORI = (23.0, 23.5)         # interassi dei fori M2 del circuito (da verificare)
OLED_VETRO = 1.8                 # dalla faccia del vetro alla faccia del circuito (altezza delle colonnine)
VETRINO = (31.0, 17.0, 1.6)      # lastrina in plexiglass (lato lungo × corto × spessore), incollata da dentro
COLONNINA_OLED_R = 2.0

SCRITTA = "loopant RX"

# ============================================================================================================


def scatola_arrotondata(x0, y0, z0, x1, y1, z1, r):
    s = cq.Workplane("XY").box(x1 - x0, y1 - y0, z1 - z0, centered=False).translate((x0, y0, z0))
    return s.edges("|Z").fillet(r) if r > 0 else s


def cilindro_z(x, y, z0, h, d):
    return cq.Workplane("XY").workplane(offset=z0).center(x, y).circle(d / 2).extrude(h)


def cilindro_x(x0, y, z, lungo, d):
    return cq.Workplane("YZ").workplane(offset=x0).center(y, z).circle(d / 2).extrude(lungo)


def viti_coperchio():
    m = 1.0  # sulla mezzeria del bordo (che va da -PARETE a +BORDO)
    c = 1.5
    return [(c, c), (LX - c, c), (c, LY - c), (LX - c, LY - c),
            (LX / 2, m), (LX / 2, LY - m), (m, LY / 2), (LX - m, LY / 2)]


def contorno_esterno(z0, z1):
    return scatola_arrotondata(-PARETE, -PARETE, z0, LX + PARETE, LY + PARETE, z1, R_ESTERNO)


def apertura(z0, z1, gioco=0.0):
    return scatola_arrotondata(BORDO + gioco, BORDO + gioco, z0, LX - BORDO - gioco, LY - BORDO - gioco, z1, 2.0)


def scatola():
    s = contorno_esterno(-FONDO, HZ).cut(scatola_arrotondata(0, 0, 0, LX, LY, HZ + 1, R_ESTERNO - PARETE))

    # Bordo superiore verso l'interno, con smusso a 45° sotto (si stampa senza supporti)
    zb = HZ - BORDO_H
    bordo = scatola_arrotondata(0, 0, zb, LX, LY, HZ, R_ESTERNO - PARETE).cut(apertura(zb - 1, HZ + 1))
    smusso = (cq.Workplane("XY").workplane(offset=zb - BORDO).center(LX / 2, LY / 2).rect(LX, LY)
              .workplane(offset=BORDO).rect(LX, LY).loft())
    cono = (cq.Workplane("XY").workplane(offset=zb - BORDO).center(LX / 2, LY / 2).rect(LX, LY)
            .workplane(offset=BORDO).rect(LX - 2 * BORDO, LY - 2 * BORDO).loft())
    s = s.union(bordo).union(smusso.cut(cono).intersect(scatola_arrotondata(0, 0, -1, LX, LY, HZ, R_ESTERNO - PARETE)))
    for x, y in viti_coperchio():
        s = s.cut(cilindro_z(x, y, HZ - INSERTO_P, INSERTO_P + 1, INSERTO_D))

    # Colonnine della piastra con fori per inserti M3
    for x, y in COLONNINE:
        s = s.union(cilindro_z(x, y, -0.1, COLONNINA_H + 0.1, 9.0))
        s = s.cut(cilindro_z(x, y, COLONNINA_H - 5.5, 6, INSERTO_D))

    # Passaggio dell'albero: borchia esterna a "D" con mento a 45°, foro e sede del paraolio
    z_asse = Z_ASSE
    xb = -PARETE - BOSS_SPORGENZA
    borchia = cilindro_x(xb, Y_ASSE, z_asse, BOSS_SPORGENZA + 0.1, 2 * BOSS_R)
    borchia = borchia.union(cq.Workplane("XY").box(BOSS_SPORGENZA + 0.1, 2 * BOSS_R, BOSS_R, centered=False)
                            .translate((xb, Y_ASSE - BOSS_R, z_asse - BOSS_R)))
    mento = (cq.Workplane("XZ").polyline([(-PARETE + 0.1, z_asse - BOSS_R), (xb, z_asse - BOSS_R),
                                          (-PARETE + 0.1, z_asse - BOSS_R - BOSS_SPORGENZA - 0.1)]).close()
             .extrude(-2 * BOSS_R).translate((0, Y_ASSE - BOSS_R, 0)))
    s = s.union(borchia).union(mento)
    s = s.cut(cilindro_x(xb - 1, Y_ASSE, z_asse, BOSS_SPORGENZA + PARETE + 2, FORO_ALBERO))
    s = s.cut(cilindro_x(xb - 1, Y_ASSE, z_asse, PARAOLIO_P + 1, PARAOLIO_D))

    # Pressacavi e sfiato
    for _, y, z, d in FORI_FRONTE:
        s = s.cut(cilindro_x(-PARETE - 1, y, z, PARETE + 2, d))
    for _, y, z, d in FORI_RETRO:
        s = s.cut(cilindro_x(LX - 1, y, z, PARETE + 2, d))

    # Rinforzi interni con inserti M4 ciechi per la staffa del palo
    for x, y in VITI_STAFFA:
        s = s.union(cilindro_z(x, y, -0.1, 5.1, 11.0))
        s = s.cut(cilindro_z(x, y, -FONDO - 1, INSERTO_M4_P + 1, INSERTO_M4_D))
    return s


def staffa():
    """Staffa per il palo, nella posizione di montaggio (sotto il fondo). Il palo corre lungo X."""
    z1 = -FONDO                       # faccia contro la scatola
    zp = z1 - STAFFA_SP               # faccia verso il palo
    y0, y1 = LY / 2 - STAFFA_LARGH / 2, LY / 2 + STAFFA_LARGH / 2
    st = scatola_arrotondata(STAFFA_X[0], y0, zp, STAFFA_X[1], y1, z1, 6.0)
    zv = zp - TUNNEL[1] - 3.0         # vertice della V
    vh = PALO_D_MAX / (2 * math.sqrt(2)) + 2.0  # altezza delle facce della V
    zb = zv - vh
    for xb in BLOCCHI_X:
        b = cq.Workplane("XY").box(BLOCCO_L, STAFFA_LARGH, zp - zb + 0.1, centered=False).translate(
            (xb - BLOCCO_L / 2, y0, zb))
        b = b.edges("|X").fillet(4.0)
        v = (cq.Workplane("YZ").polyline([(LY / 2, zv), (LY / 2 - 60, zv - 60), (LY / 2 + 60, zv - 60)]).close()
             .extrude(BLOCCO_L + 2).translate((xb - BLOCCO_L / 2 - 1, 0, 0)))
        tunnel = cq.Workplane("XY").box(TUNNEL[0], STAFFA_LARGH + 2, TUNNEL[1], centered=False).translate(
            (xb - TUNNEL[0] / 2, y0 - 1, zp - TUNNEL[1]))
        st = st.union(b.cut(v)).cut(tunnel)
    for x, y in VITI_STAFFA:
        st = st.cut(cilindro_z(x, y, zp - 1, STAFFA_SP + 2, 4.5))
        st = st.cut(cilindro_z(x, y, zp - 1, 1 + 3.0, 8.5))  # testa M4 incassata dal lato del palo
    return st


def centro_palo():
    zp = -FONDO - STAFFA_SP
    zv = zp - TUNNEL[1] - 3.0
    return zv - PALO_D / 2 * math.sqrt(2)


def piastra():
    z0 = COLONNINA_H
    p = scatola_arrotondata(BORDO + 1, BORDO + 1, z0, LX - BORDO - 1, LY - BORDO - 1, z0 + PIASTRA, 4.0)
    for x, y in COLONNINE:
        p = p.cut(cilindro_z(x, y, z0 - 1, PIASTRA + 2, 3.4))
        p = p.cut(cilindro_z(x, y, z0 + PIASTRA - 3.2, 4, 6.2))  # testa della vite incassata
    # Rialzo sotto la squadra (dislivello con il variabile); le asole lo attraversano: viti M4 lunghe con i dadi
    # sotto la piastra
    if RIALZO_MOTORE > 0:
        p = p.union(scatola_arrotondata(BASE_SQUADRA[0] - 3, Y_ASSE - 24, z0 + PIASTRA - 0.1,
                                        BASE_SQUADRA[1] + 3, Y_ASSE + 24, z0 + PIASTRA + RIALZO_MOTORE, 3.0))
    for x, y in ASOLE_SQUADRA:
        p = p.cut(cq.Workplane("XY").workplane(offset=z0 - 1).center(x, y).slot2D(ASOLA_L + ASOLA_W, ASOLA_W)
                  .extrude(PIASTRA + RIALZO_MOTORE + 2))
    # Appoggio sotto il corpo del motore (0,5 mm di gioco) e fessure per la fascetta che lo tiene
    if SUPPORTO_MOTORE:
        xm = X_SQUADRA + SQUADRA_S + MOTORE_L / 2
        cima = Z_ASSE - NEMA / 2 - 0.5
        p = p.union(scatola_arrotondata(xm - 9, Y_ASSE - 18, z0 + PIASTRA - 0.1, xm + 9, Y_ASSE + 18, cima, 2.0))
        for lato in (-1, 1):
            p = p.cut(cq.Workplane("XY").workplane(offset=z0 - 1).center(xm, Y_ASSE + lato * 23.3).rect(5, 2.2)
                      .extrude(PIASTRA + 2))
    # Distanziali delle due strisce di millefori
    w, h, _ = MILLEFORI
    f = MILLEFORI_FORI
    for y0 in MILLEFORI_Y0:
        for x, y in [(f, f), (w - f, f), (f, h - f), (w - f, h - f)]:
            xx, yy = MILLEFORI_X0 + x, y0 + y
            p = p.union(cilindro_z(xx, yy, z0 + PIASTRA - 0.1, DISTANZIALE_H + 0.1, 2 * DISTANZIALE_R))
            p = p.cut(cilindro_z(xx, yy, z0 + PIASTRA + 1, DISTANZIALE_H, DISTANZIALE_FORO))
    # Coppie di fessure per fascette (cavi del SERVO42D e dell'alimentazione)
    for x, y in [(LX - 16, 30), (LX - 16, LY - 30), (MILLEFORI_X0 - 12, 30), (MILLEFORI_X0 - 12, LY - 30)]:
        for dx in (-4, 4):
            p = p.cut(cq.Workplane("XY").workplane(offset=z0 - 1).center(x + dx, y).rect(2, 4.5).extrude(PIASTRA + 2))
    return p


def fori_oled():
    """Fori del circuito dell'OLED (il centro del circuito è spostato verso i pin, cioè verso X = 0)."""
    fx, fy = OLED_FORI  # lungo il lato lungo, lungo il lato corto
    return [(OLED_X - OLED_DY_PCB + sb * fy / 2, OLED_Y + sa * fx / 2) for sa in (-1, 1) for sb in (-1, 1)]


def coperchio(z_basso):
    """Coperchio nella posizione di montaggio (faccia interna a z_basso)."""
    c = contorno_esterno(z_basso, z_basso + COPERCHIO)
    gonna = apertura(z_basso - GONNA_H, z_basso + 0.1, gioco=0.4).cut(
        apertura(z_basso - GONNA_H - 1, z_basso + 1, gioco=0.4 + GONNA_S))
    c = c.union(gonna)
    for x, y in viti_coperchio():
        c = c.cut(cilindro_z(x, y, z_basso - 1, COPERCHIO + 2, VITE_PASSANTE))

    # Finestra dell'OLED con smusso esterno, sede del vetrino da dentro, colonnine del circuito.
    # Display ruotato: lato lungo lungo Y, alto del display (pin) verso X = 0 (in alto con il palo verticale).
    lv, cv = OLED_VISIBILE
    finestra = (cq.Workplane("XY").workplane(offset=z_basso - 0.1).center(OLED_X, OLED_Y).rect(cv, lv)
                .workplane(offset=COPERCHIO + 0.2).rect(cv + 2 * COPERCHIO, lv + 2 * COPERCHIO).loft())
    c = c.cut(finestra)
    c = c.cut(scatola_arrotondata(OLED_X - VETRINO[1] / 2, OLED_Y - VETRINO[0] / 2, z_basso - 0.1,
                                  OLED_X + VETRINO[1] / 2, OLED_Y + VETRINO[0] / 2, z_basso + VETRINO[2], 1.0))
    for x, y in fori_oled():
        c = c.union(cilindro_z(x, y, z_basso - OLED_VETRO, OLED_VETRO + 0.1, 2 * COLONNINA_OLED_R))
        c = c.cut(cilindro_z(x, y, z_basso - OLED_VETRO - 1, OLED_VETRO + 1 + 2.5, 1.7))

    # Scritta incisa sopra, leggibile con la parete frontale in alto
    try:
        testo = (cq.Workplane("XY").text(SCRITTA, 12, 1.0, kind="bold")
                 .rotate((0, 0, 0), (0, 0, 1), 90).translate((LX - 32, LY / 2, z_basso + COPERCHIO - 0.6)))
        c = c.cut(testo)
    except Exception as e:  # font mancante: il coperchio va bene lo stesso
        print("scritta non incisa:", e)
    return c


def guarnizione(z_basso):
    g = contorno_esterno(z_basso, z_basso + GUARNIZIONE).cut(apertura(z_basso - 1, z_basso + GUARNIZIONE + 1))
    for x, y in viti_coperchio():
        g = g.cut(cilindro_z(x, y, z_basso - 1, GUARNIZIONE + 2, VITE_PASSANTE + 0.2))
    return g


def ingombri():
    """Componenti finti, solo per le anteprime e per controllare gli spazi."""
    zp = COLONNINA_H + PIASTRA
    za = Z_ASSE
    zs = zp + RIALZO_MOTORE           # base della squadra
    xs = X_SQUADRA
    pezzi = {}
    sq = (cq.Workplane("XY").box(SQUADRA_S, 44, H_ASSE + 22, centered=False).translate((xs, Y_ASSE - 22, zs))
          .union(cq.Workplane("XY").box(SQUADRA_BASE, 44, SQUADRA_S, centered=False)
                 .translate((BASE_SQUADRA[0], Y_ASSE - 22, zs))))
    pezzi["squadra"] = (sq, "#9aa0a6")
    pezzi["riduttore"] = (cilindro_x(X_RIDUTTORE, Y_ASSE, za, RIDUTTORE_L, RIDUTTORE_D), "#3c4043")
    x = xs + SQUADRA_S
    pezzi["motore"] = (cq.Workplane("XY").box(MOTORE_L, NEMA, NEMA, centered=False)
                       .translate((x, Y_ASSE - NEMA / 2, za - NEMA / 2)).edges("|X").chamfer(3), "#202124")
    x += MOTORE_L
    pezzi["servo42d"] = (cq.Workplane("XY").box(DRIVER_L, NEMA, NEMA, centered=False)
                         .translate((x, Y_ASSE - NEMA / 2, za - NEMA / 2)).edges("|X").chamfer(3), "#1a73e8")
    pezzi["albero"] = (cilindro_x(X_RIDUTTORE - ALBERO_L, Y_ASSE, za, ALBERO_L, ALBERO_D), "#bdc1c6")
    pezzi["giunto"] = (cilindro_x(X_GIUNTO, Y_ASSE, za, GIUNTO_L, GIUNTO_D), "#c5a572")
    pezzi["tubo isolante"] = (cilindro_x(-PARETE - BOSS_SPORGENZA - 30, Y_ASSE, za,
                                         30 + PARETE + BOSS_SPORGENZA + 10, TUBO_ISOLANTE), "#f1f3f4")
    w, h, t = MILLEFORI
    zm = zp + DISTANZIALE_H
    ya, yb = MILLEFORI_Y0
    pezzi["millefori A"] = (cq.Workplane("XY").box(w, h, t, centered=False).translate((MILLEFORI_X0, ya, zm)),
                            "#2e7d32")
    pezzi["millefori B"] = (cq.Workplane("XY").box(w, h, t, centered=False).translate((MILLEFORI_X0, yb, zm)),
                            "#2e7d32")
    pezzi["ESP32"] = (cq.Workplane("XY").box(52, 28.5, 12, centered=False)
                      .translate((MILLEFORI_X0 + 19, ya + (h - 28.5) / 2, zm + t + 8.5)), "#111111")
    pezzi["SN65HVD230"] = (cq.Workplane("XY").box(32, 22, 6, centered=False)
                           .translate((MILLEFORI_X0 + 5, yb + (h - 22) / 2, zm + t)), "#1565c0")
    pezzi["24→5 V"] = (cq.Workplane("XY").box(43, 21, 14, centered=False)
                       .translate((MILLEFORI_X0 + 42, yb + (h - 21) / 2, zm + t)), "#0d47a1")
    zc = HZ + GUARNIZIONE
    pezzi["OLED"] = (cq.Workplane("XY").box(27.8, 27.3, 1.6, centered=False)
                     .translate((OLED_X - OLED_DY_PCB - 13.9, OLED_Y - 13.65, zc - OLED_VETRO - 1.6))
                     .union(cq.Workplane("XY").box(19.3, 26.7, OLED_VETRO, centered=False)
                            .translate((OLED_X - 9.65, OLED_Y - 13.35, zc - OLED_VETRO))), "#fbbc04")
    return pezzi


def controlla(sc, pi, pezzi):
    """Controlli di ingombro: i componenti non devono toccare pareti, bordo e piastra."""
    interno = scatola_arrotondata(0, 0, COLONNINA_H + PIASTRA, LX, LY, HZ - BORDO_H - BORDO, R_ESTERNO - PARETE)
    errori = []
    for nome in ("riduttore", "motore", "servo42d", "millefori A", "millefori B", "ESP32", "SN65HVD230", "24→5 V"):
        s = pezzi[nome][0]
        fuori = s.cut(interno).val().Volume()
        if fuori > 1:
            errori.append(f"{nome}: {fuori:.0f} mm³ fuori dallo spazio utile")
    nomi = ("motore", "servo42d", "riduttore", "squadra", "millefori A", "millefori B", "ESP32", "SN65HVD230",
            "24→5 V")
    for nome in nomi:
        if pezzi[nome][0].intersect(sc).val().Volume() > 1:
            errori.append(f"{nome}: tocca la scatola")
        if nome != "squadra" and pezzi[nome][0].intersect(pi).val().Volume() > 1:
            errori.append(f"{nome}: tocca la piastra")
    for i, a in enumerate(nomi):
        for b in nomi[i + 1:]:
            if {a, b} <= {"millefori A", "ESP32"} or {a, b} <= {"millefori B", "SN65HVD230", "24→5 V"}:
                continue
            if {a, b} <= {"motore", "servo42d", "riduttore", "squadra"}:
                continue
            if pezzi[a][0].intersect(pezzi[b][0]).val().Volume() > 1:
                errori.append(f"{a} e {b} si toccano")
    # spazio dietro il SERVO42D per i connettori
    retro = LX - (X_SQUADRA + SQUADRA_S + MOTORE_L + DRIVER_L)
    if retro < 20:
        errori.append(f"dietro il SERVO42D restano solo {retro:.0f} mm per i cavi")
    tra = min(Y_ASSE - NEMA / 2 - (MILLEFORI_Y0[0] + MILLEFORI[1]), MILLEFORI_Y0[1] - (Y_ASSE + NEMA / 2))
    if tra < 4:
        errori.append(f"tra motore e millefori solo {tra:.1f} mm")
    print(f"Spazio dietro il SERVO42D: {retro:.0f} mm; tra motore e millefori: {tra:.1f} mm")
    print(f"Gruppo {LUNGHEZZA_GRUPPO:.0f} mm: riduttore stimato {RIDUTTORE_L:.1f} mm; scatola lunga {LX:.0f} mm interni")
    print(f"Asse del motore: {Z_ASSE:.1f} mm sopra il fondo interno, {Z_ASSE + FONDO:.1f} mm sopra il fondo esterno "
          f"(rialzo {RIALZO_MOTORE:.0f} mm); altezza interna {HZ:.0f} mm")
    if SQUADRA_VERSO > 0 and H_ASSE - NEMA / 2 < SQUADRA_S:
        errori.append("il motore non sta sopra la base della squadra: H_ASSE troppo piccolo")
    if SQUADRA_VERSO < 0 and H_ASSE - RIDUTTORE_D / 2 < SQUADRA_S:
        errori.append("il riduttore non sta sopra la base della squadra: H_ASSE troppo piccolo")
    if FORO_ALBERO < TUBO_ISOLANTE:
        errori.append("foro dell'albero più stretto del tubo isolante")
    return errori


def esporta(oggetti):
    os.makedirs(os.path.join(QUI, "stl"), exist_ok=True)
    os.makedirs(os.path.join(QUI, "step"), exist_ok=True)
    for nome, o in oggetti.items():
        cq.exporters.export(o, os.path.join(QUI, "stl", nome + ".stl"), tolerance=0.05, angularTolerance=0.15)
        cq.exporters.export(o, os.path.join(QUI, "step", nome + ".step"))
        bb = o.val().BoundingBox()
        print(f"{nome:12s} {bb.xlen:6.1f} × {bb.ylen:6.1f} × {bb.zlen:5.1f} mm")


def sul_piatto(o):
    """Sposta il pezzo con l'angolo dell'ingombro nell'origine (z = 0 sul piatto)."""
    bb = o.val().BoundingBox()
    return o.translate((-bb.xmin, -bb.ymin, -bb.zmin))


def main():
    sc = scatola()
    pi = piastra()
    st = staffa()
    zc = HZ + GUARNIZIONE
    co = coperchio(zc)
    gu = guarnizione(HZ)
    pezzi = ingombri()
    errori = controlla(sc, pi, pezzi)
    if st.intersect(sc).val().Volume() > 1:
        errori.append("la staffa entra nella scatola")
    for e in errori:
        print("ATTENZIONE:", e)

    # Pezzi orientati per la stampa
    stampa = {
        "scatola": sul_piatto(sc),                                            # fondo sul piatto
        "piastra": sul_piatto(pi),
        "coperchio": sul_piatto(co.rotate((0, 0, 0), (1, 0, 0), 180)),       # faccia esterna sul piatto
        "guarnizione": sul_piatto(gu),
        "staffa-palo": sul_piatto(st.rotate((0, 0, 0), (1, 0, 0), 180)),     # faccia verso la scatola sul piatto
    }
    esporta(stampa)

    if "--niente-anteprime" not in sys.argv:
        import anteprime
        palo = cilindro_x(-80, LY / 2, centro_palo(), LX + 160, PALO_D)
        anteprime.disegna(QUI, {"scatola": sc, "piastra": pi, "coperchio": co, "guarnizione": gu, "staffa": st,
                                "palo": palo}, pezzi)
    return 1 if errori else 0


if __name__ == "__main__":
    sys.exit(main())
