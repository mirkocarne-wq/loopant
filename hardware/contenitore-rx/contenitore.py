#!/usr/bin/env python3
"""Contenitore stampabile dell'unità antenna RX (CadQuery).

Dentro: SERVO42D (NEMA17 con il driver sul retro) + riduttore planetario 20:1 fissati con una squadra a 90°
alla piastra interna, millefori con ESP32 DevKit, SN65HVD230 e convertitore 24→5 V, display OLED 0,96"
sotto il coperchio, dietro una finestra. L'asse del riduttore esce dalla parete frontale (asta in ABS 8 mm
con paraolio) verso la scatola del variabile, che resta separata e lontana dall'elettronica.

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

# --- Scatola ---
LX, LY, HZ = 170.0, 135.0, 80.0  # misure interne (lunghezza, larghezza, altezza fino al bordo)
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
Y_ASSE = 33.0                    # posizione dell'asse in larghezza
H_ASSE = 30.0                    # altezza dell'asse sopra la piastra (dipende dalla squadra)
X_SQUADRA = 38.0                 # faccia della squadra (lato albero) dalla parete frontale
SQUADRA_S = 3.0                  # spessore della squadra
SQUADRA_BASE = 35.0              # profondità della base della squadra (verso il motore)
RIDUTTORE_L, RIDUTTORE_D = 40.0, 36.0
MOTORE_L, NEMA = 40.0, 42.3
DRIVER_L = 22.0                  # SERVO42D con il suo guscio sul retro del motore
ALBERO_D, ALBERO_L = 8.0, 20.0   # albero di uscita del riduttore
GIUNTO_D, GIUNTO_L = 20.0, 25.0
ASOLE_SQUADRA = [(X_SQUADRA + 17, Y_ASSE - 13), (X_SQUADRA + 17, Y_ASSE + 13)]  # centri delle asole
ASOLA_L, ASOLA_W = 20.0, 4.5     # asole lungo X per viti M4 (permettono di spostare la squadra)

# --- Passaggio dell'albero (parete frontale) ---
BOSS_R, BOSS_SPORGENZA = 13.0, 6.0
FORO_ALBERO = 9.0                # asta ABS 8 mm
PARAOLIO_D, PARAOLIO_P = 16.2, 5.0  # sede per paraolio 8×16×5 (labbro verso l'esterno)

# --- Pressacavi e sfiato ---
PG7 = 12.5                       # foro per pressacavo PG7 (cavo 3–6,5 mm)
SFIATO = 12.3                    # valvola di sfiato a membrana M12×1,5
FORI_FRONTE = [("PG7 finecorsa", LY - 35, 30.0, PG7)]                                # (nome, y, z, Ø)
FORI_RETRO = [("PG7 24 V", LY - 35, 28.0, PG7), ("sfiato M12", 35.0, 22.0, SFIATO)]

# --- Alette di fissaggio (sul fondo, lati lunghi) ---
ALETTA_X = [30.0, LX - 30.0]
ALETTA_L, ALETTA_SP, ALETTA_S, ALETTA_FORO = 22.0, 15.0, 5.0, 5.5

# --- Elettronica su millefori 90×70 ---
MILLEFORI = (90.0, 70.0, 1.6)
MILLEFORI_X0, MILLEFORI_Y0 = 42.0, LY - 6.0 - 70.0
MILLEFORI_FORI = 2.5             # distanza dei fori di fissaggio dal bordo
DISTANZIALE_H, DISTANZIALE_R, DISTANZIALE_FORO = 8.0, 3.0, 1.8  # viti M2 autofilettanti

# --- Display OLED 0,96" ELEGOO (SSD1306) sotto il coperchio ---
OLED_X, OLED_Y = 86.0, 95.0      # centro della zona visibile
OLED_VISIBILE = (25.0, 14.0)     # apertura (zona attiva 21,7×10,9 + margine)
OLED_DY_PCB = 1.5                # centro del circuito rispetto alla zona attiva, verso i pin (da verificare)
OLED_FORI = (23.0, 23.5)         # interassi dei fori M2 del circuito (da verificare)
OLED_VETRO = 1.8                 # dalla faccia del vetro alla faccia del circuito (altezza delle colonnine)
VETRINO = (31.0, 17.0, 1.6)      # lastrina in plexiglass incollata da dentro con silicone
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
    z_asse = COLONNINA_H + PIASTRA + H_ASSE
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

    # Alette di fissaggio
    for x in ALETTA_X:
        for lato in (-1, 1):
            y0 = -PARETE - ALETTA_SP if lato < 0 else LY + PARETE
            a = scatola_arrotondata(x - ALETTA_L / 2, y0, -FONDO,
                                    x + ALETTA_L / 2, y0 + ALETTA_SP, -FONDO + ALETTA_S, 3.0)
            # due rinforzi triangolari contro la parete, ai lati del foro (lasciano spazio alla rondella)
            yp = -PARETE if lato < 0 else LY + PARETE
            zt = -FONDO + ALETTA_S - 0.1
            for dx in (-ALETTA_L / 2 + 1.5, ALETTA_L / 2 - 4.5):
                rinforzo = (cq.Workplane("YZ").polyline([(yp, zt), (yp + lato * 7, zt), (yp, zt + 9)]).close()
                            .extrude(3).translate((x + dx, 0, 0)))
                a = a.union(rinforzo)
            yf = y0 + ALETTA_SP / 2 + lato * 1.0
            s = s.union(a).cut(cilindro_z(x, yf, -FONDO - 1, ALETTA_S + 2, ALETTA_FORO))
    return s


def piastra():
    z0 = COLONNINA_H
    p = scatola_arrotondata(BORDO + 1, BORDO + 1, z0, LX - BORDO - 1, LY - BORDO - 1, z0 + PIASTRA, 4.0)
    for x, y in COLONNINE:
        p = p.cut(cilindro_z(x, y, z0 - 1, PIASTRA + 2, 3.4))
        p = p.cut(cilindro_z(x, y, z0 + PIASTRA - 3.2, 4, 6.2))  # testa della vite incassata
    for x, y in ASOLE_SQUADRA:
        p = p.cut(cq.Workplane("XY").workplane(offset=z0 - 1).center(x, y).slot2D(ASOLA_L + ASOLA_W, ASOLA_W)
                  .extrude(PIASTRA + 2))
    # Distanziali della millefori
    w, h, _ = MILLEFORI
    f = MILLEFORI_FORI
    for x, y in [(f, f), (w - f, f), (f, h - f), (w - f, h - f)]:
        xx, yy = MILLEFORI_X0 + x, MILLEFORI_Y0 + y
        p = p.union(cilindro_z(xx, yy, z0 + PIASTRA - 0.1, DISTANZIALE_H + 0.1, 2 * DISTANZIALE_R))
        p = p.cut(cilindro_z(xx, yy, z0 + PIASTRA + 1, DISTANZIALE_H, DISTANZIALE_FORO))
    # Coppie di fessure per fascette (cavi del SERVO42D e dell'alimentazione)
    for x, y in [(LX - 18, 30), (LX - 18, 48), (MILLEFORI_X0 - 10, LY - 20), (LX - 18, LY - 20)]:
        for dx in (-4, 4):
            p = p.cut(cq.Workplane("XY").workplane(offset=z0 - 1).center(x + dx, y).rect(2, 4.5).extrude(PIASTRA + 2))
    # Alleggerimento sotto il motore (lontano dalle asole)
    p = p.cut(cq.Workplane("XY").workplane(offset=z0 - 1).center(X_SQUADRA + SQUADRA_BASE + 40, Y_ASSE)
              .slot2D(60, 22).extrude(PIASTRA + 2))
    return p


def coperchio(z_basso):
    """Coperchio nella posizione di montaggio (faccia interna a z_basso)."""
    c = contorno_esterno(z_basso, z_basso + COPERCHIO)
    gonna = apertura(z_basso - GONNA_H, z_basso + 0.1, gioco=0.4).cut(
        apertura(z_basso - GONNA_H - 1, z_basso + 1, gioco=0.4 + GONNA_S))
    c = c.union(gonna)
    for x, y in viti_coperchio():
        c = c.cut(cilindro_z(x, y, z_basso - 1, COPERCHIO + 2, VITE_PASSANTE))

    # Finestra dell'OLED con smusso esterno, sede del vetrino da dentro, colonnine del circuito
    wv, hv = OLED_VISIBILE
    finestra = (cq.Workplane("XY").workplane(offset=z_basso - 0.1).center(OLED_X, OLED_Y).rect(wv, hv)
                .workplane(offset=COPERCHIO + 0.2).rect(wv + 2 * COPERCHIO, hv + 2 * COPERCHIO).loft())
    c = c.cut(finestra)
    c = c.cut(scatola_arrotondata(OLED_X - VETRINO[0] / 2, OLED_Y - VETRINO[1] / 2, z_basso - 0.1,
                                  OLED_X + VETRINO[0] / 2, OLED_Y + VETRINO[1] / 2, z_basso + VETRINO[2], 1.0))
    fx, fy = OLED_FORI
    for sx in (-1, 1):
        for sy in (-1, 1):
            x, y = OLED_X + sx * fx / 2, OLED_Y + OLED_DY_PCB + sy * fy / 2
            c = c.union(cilindro_z(x, y, z_basso - OLED_VETRO, OLED_VETRO + 0.1, 2 * COLONNINA_OLED_R))
            c = c.cut(cilindro_z(x, y, z_basso - OLED_VETRO - 1, OLED_VETRO + 1 + 2.5, 1.7))

    # Scritta incisa sopra
    try:
        testo = (cq.Workplane("XY").workplane(offset=z_basso + COPERCHIO - 0.6).center(LX / 2, 28)
                 .text(SCRITTA, 12, 1.0, kind="bold"))
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
    za = zp + H_ASSE
    xs = X_SQUADRA
    pezzi = {}
    sq = (cq.Workplane("XY").box(SQUADRA_S, 44, H_ASSE + 22, centered=False).translate((xs, Y_ASSE - 22, zp))
          .union(cq.Workplane("XY").box(SQUADRA_BASE, 44, SQUADRA_S, centered=False).translate((xs, Y_ASSE - 22, zp))))
    pezzi["squadra"] = (sq, "#9aa0a6")
    x = xs + SQUADRA_S
    pezzi["riduttore"] = (cilindro_x(x, Y_ASSE, za, RIDUTTORE_L, RIDUTTORE_D), "#3c4043")
    x += RIDUTTORE_L
    pezzi["motore"] = (cq.Workplane("XY").box(MOTORE_L, NEMA, NEMA, centered=False)
                       .translate((x, Y_ASSE - NEMA / 2, za - NEMA / 2)).edges("|X").chamfer(3), "#202124")
    x += MOTORE_L
    pezzi["servo42d"] = (cq.Workplane("XY").box(DRIVER_L, NEMA, NEMA, centered=False)
                         .translate((x, Y_ASSE - NEMA / 2, za - NEMA / 2)).edges("|X").chamfer(3), "#1a73e8")
    pezzi["albero"] = (cilindro_x(xs - ALBERO_L, Y_ASSE, za, ALBERO_L, ALBERO_D), "#bdc1c6")
    pezzi["giunto"] = (cilindro_x(xs - ALBERO_L - 3, Y_ASSE, za, GIUNTO_L, GIUNTO_D), "#c5a572")
    pezzi["asta ABS"] = (cilindro_x(-PARETE - BOSS_SPORGENZA - 30, Y_ASSE, za, 30 + PARETE + BOSS_SPORGENZA + 10, 8),
                         "#f1f3f4")
    w, h, t = MILLEFORI
    zm = zp + DISTANZIALE_H
    pezzi["millefori"] = (cq.Workplane("XY").box(w, h, t, centered=False).translate((MILLEFORI_X0, MILLEFORI_Y0, zm)),
                          "#2e7d32")
    pezzi["ESP32"] = (cq.Workplane("XY").box(52, 28.5, 12, centered=False)
                      .translate((MILLEFORI_X0 + 6, MILLEFORI_Y0 + 36, zm + t + 8.5)), "#111111")
    pezzi["SN65HVD230"] = (cq.Workplane("XY").box(32, 22, 6, centered=False)
                           .translate((MILLEFORI_X0 + 6, MILLEFORI_Y0 + 6, zm + t)), "#1565c0")
    pezzi["24→5 V"] = (cq.Workplane("XY").box(43, 21, 14, centered=False)
                       .translate((MILLEFORI_X0 + 42, MILLEFORI_Y0 + 6, zm + t)), "#0d47a1")
    zc = HZ + GUARNIZIONE
    pezzi["OLED"] = (cq.Workplane("XY").box(27.3, 27.8, 1.6, centered=False)
                     .translate((OLED_X - 13.65, OLED_Y + OLED_DY_PCB - 13.9, zc - OLED_VETRO - 1.6))
                     .union(cq.Workplane("XY").box(26.7, 19.3, OLED_VETRO, centered=False)
                            .translate((OLED_X - 13.35, OLED_Y - 9.65, zc - OLED_VETRO))), "#fbbc04")
    return pezzi


def controlla(sc, pi, pezzi):
    """Controlli di ingombro: i componenti non devono toccare pareti, bordo e piastra."""
    interno = scatola_arrotondata(0, 0, COLONNINA_H + PIASTRA, LX, LY, HZ - BORDO_H - BORDO, R_ESTERNO - PARETE)
    errori = []
    for nome in ("riduttore", "motore", "servo42d", "millefori", "ESP32", "SN65HVD230", "24→5 V"):
        s = pezzi[nome][0]
        fuori = s.cut(interno).val().Volume()
        if fuori > 1:
            errori.append(f"{nome}: {fuori:.0f} mm³ fuori dallo spazio utile")
    for nome in ("motore", "servo42d", "riduttore", "ESP32", "24→5 V"):
        if pezzi[nome][0].intersect(sc).val().Volume() > 1 or pezzi[nome][0].intersect(pi).val().Volume() > 1:
            errori.append(f"{nome}: tocca la scatola o la piastra")
    # spazio dietro il SERVO42D per i connettori
    retro = LX - (X_SQUADRA + SQUADRA_S + RIDUTTORE_L + MOTORE_L + DRIVER_L)
    if retro < 20:
        errori.append(f"dietro il SERVO42D restano solo {retro:.0f} mm per i cavi")
    tra = (MILLEFORI_Y0) - (Y_ASSE + NEMA / 2)
    if tra < 4:
        errori.append(f"tra motore e millefori solo {tra:.1f} mm")
    print(f"Spazio dietro il SERVO42D: {retro:.0f} mm; tra motore e millefori: {tra:.1f} mm")
    return errori


def esporta(oggetti):
    os.makedirs(os.path.join(QUI, "stl"), exist_ok=True)
    os.makedirs(os.path.join(QUI, "step"), exist_ok=True)
    for nome, o in oggetti.items():
        cq.exporters.export(o, os.path.join(QUI, "stl", nome + ".stl"), tolerance=0.05, angularTolerance=0.15)
        cq.exporters.export(o, os.path.join(QUI, "step", nome + ".step"))
        bb = o.val().BoundingBox()
        print(f"{nome:12s} {bb.xlen:6.1f} × {bb.ylen:6.1f} × {bb.zlen:5.1f} mm")


def main():
    sc = scatola()
    pi = piastra()
    zc = HZ + GUARNIZIONE
    co = coperchio(zc)
    gu = guarnizione(HZ)
    pezzi = ingombri()
    errori = controlla(sc, pi, pezzi)
    for e in errori:
        print("ATTENZIONE:", e)

    # Pezzi orientati per la stampa (fondo sul piatto)
    stampa = {
        "scatola": sc.translate((PARETE, PARETE, FONDO)),
        "piastra": pi.translate((-BORDO - 1, -BORDO - 1, -COLONNINA_H)),
        "coperchio": co.rotate((0, 0, 0), (1, 0, 0), 180).translate((PARETE, LY + PARETE, zc + COPERCHIO)),
        "guarnizione": gu.translate((PARETE, PARETE, -HZ)),
    }
    esporta(stampa)

    if "--niente-anteprime" not in sys.argv:
        import anteprime
        anteprime.disegna(QUI, {"scatola": sc, "piastra": pi, "coperchio": co, "guarnizione": gu}, pezzi,
                          zc, COPERCHIO)
    return 1 if errori else 0


if __name__ == "__main__":
    sys.exit(main())
