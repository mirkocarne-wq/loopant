#!/usr/bin/env python3
"""Assieme dell'antenna RX completa: palo, scatola del motore, scatola del variabile, tubo isolante, anello di rame,
loop di accoppiamento e supporto in cima (questi ultimi due sono una PROPOSTA da confermare).

Coordinate dell'assieme: Z verso l'alto lungo il palo, X dal palo verso chi guarda il coperchio, Y in orizzontale.
Uso:  python3 assieme.py   -> anteprime/assieme-*.png e step/assieme.step
"""
import math
import os
import sys

import cadquery as cq

import contenitore as c
import scatola_variabile as v

QUI = os.path.dirname(os.path.abspath(__file__))

# --- Anello e parti in cima (proposta) ---
ANELLO_D = 1000.0                # diametro dell'anello (asse del tubo)
COLLO = 60.0                     # tratto diritto dei capi sopra il blocco della scatola del variabile
ACCOPPIAMENTO_D = 200.0          # loop di accoppiamento, circa 1/5 dell'anello (proposta, da provare)
ACCOPPIAMENTO_TUBO = 6.0         # coassiale o tubetto di rame
SPAZIO_BORCHIE = 12.0            # tra le due borchie (tubo isolante)
PALO_SOTTO = 300.0               # palo sotto la scatola del motore (nell'immagine)
PALO_SOPRA = 35.0                # palo sopra il raccordo a T (tappo)


def colorato(nome, oggetto, colore):
    return (nome, oggetto, colore)


def costruisci():
    parti = []
    # --- Scatola del motore (parametri predefiniti di contenitore.py) ---
    rot_m = lambda o: o.rotate((0, 0, 0), (0, 1, 0), 90)       # parete frontale in alto
    zc_m = c.HZ + c.GUARNIZIONE
    ly_m, lx_m = c.LY, c.LX
    cp = c.centro_palo()
    pezzi_m = c.ingombri()
    parti += [("scatola motore", rot_m(c.scatola()), "#dfe3e8"),
              ("coperchio motore", rot_m(c.coperchio(zc_m)), "#86b6f6"),
              ("guarnizione motore", rot_m(c.guarnizione(c.HZ)), "#d93025"),
              ("staffa motore", rot_m(c.staffa()), "#7cb342"),
              ("tubo isolante", rot_m(pezzi_m["tubo isolante"][0]), "#f1f3f4")]
    pg7 = [(y, z) for nome, y, z, d in c.FORI_RETRO if nome.startswith("PG7")][0]
    off = 2 * (c.PARETE + c.BOSS_SPORGENZA) + SPAZIO_BORCHIE

    # --- Scatola del variabile (stesse parti, girata: borchia in basso, stesso asse, stessa distanza dal palo) ---
    v.imposta()
    zc_v = c.HZ + c.GUARNIZIONE

    def rot_v(o):
        return o.rotate((0, 0, 0), (1, 0, 1), 180).translate((0, ly_m / 2 + v.LY / 2, off))
    pezzi_v = v.ingombri()
    parti += [("scatola variabile", rot_v(v.scatola()), "#dfe3e8"),
              ("coperchio variabile", rot_v(c.coperchio(zc_v)), "#86b6f6"),
              ("guarnizione variabile", rot_v(c.guarnizione(c.HZ)), "#d93025"),
              ("staffa variabile", rot_v(c.staffa()), "#7cb342")]
    for n in ("capo 1", "capo 2"):
        parti.append((n, rot_v(pezzi_v[n][0]), "#b87333"))

    # --- Anello di rame: piano a X = Z_CAPI, capi in basso a ±INTERASSE/2 ---
    r = ANELLO_D / 2
    xa = v.Z_CAPI
    z_collare = v.LX + c.PARETE + v.BLOCCO_S + v.COLLARE + off
    z_capi = z_collare + COLLO                       # dove i capi diritti incontrano il cerchio
    zc = z_capi + math.sqrt(r * r - (v.INTERASSE_CAPI / 2) ** 2)
    yc = ly_m / 2
    anello = cq.Workplane("XY").add(cq.Solid.makeTorus(r, v.TUBO_ANELLO / 2, cq.Vector(xa, yc, zc), cq.Vector(1, 0, 0)))
    anello = anello.cut(cq.Workplane("XY").box(40, v.INTERASSE_CAPI, 80).translate((xa, yc, zc - r + 20)))
    parti.append(("anello", anello, "#b87333"))

    # --- Cima (proposta): braccio in PVC Ø 40 con raccordo a T sul palo, sostegno stampato con la scatoletta
    # dell'SO-239, loop di accoppiamento che esce dal fondo della scatoletta ---
    import cima
    z_lt = zc + r
    g = cima.geometria(xa, yc, z_lt)
    z_braccio = g["z_a"]
    for nome, o in cima.pezzi(xa, yc, z_lt).items():
        parti.append((f"cima {nome}", o, "#eceff1" if nome != "cappello" else "#cfd8dc"))
    braccio = (cq.Workplane("YZ").workplane(offset=cp).center(yc, z_braccio).circle(cima.BRACCIO_D / 2)
               .extrude(g["x_fine"] - cp))
    raccordo = (cq.Workplane("XY").workplane(offset=z_braccio - 32).center(cp, yc).circle(c.PALO_D / 2 + 4).extrude(64)
                .union(cq.Workplane("YZ").workplane(offset=cp).center(yc, z_braccio).circle(cima.BRACCIO_D / 2 + 4)
                       .extrude(c.PALO_D / 2 + 30)))
    parti.append(("braccio in PVC", braccio, "#b0bec5"))
    parti.append(("raccordo a T", raccordo, "#90a4ae"))
    rc = ACCOPPIAMENTO_D / 2
    xc = g["x_acc"]
    z_uscita = g["sc"]["z0"] - cima.PARETE - 10       # tratto diritto dei capi sotto il fondo
    dy = cima.ACC_INTERASSE / 2
    zcc = z_uscita - math.sqrt(rc * rc - dy * dy)
    acc = cq.Workplane("XY").add(cq.Solid.makeTorus(rc, ACCOPPIAMENTO_TUBO / 2, cq.Vector(xc, yc, zcc),
                                                    cq.Vector(1, 0, 0)))
    acc = acc.cut(cq.Workplane("XY").box(20, 2 * dy, 30).translate((xc, yc, zcc + rc)))
    for k in (-1, 1):
        acc = acc.union(cq.Workplane("XY").workplane(offset=z_uscita - 2).center(xc, yc + k * dy)
                        .circle(ACCOPPIAMENTO_TUBO / 2).extrude(g["sc"]["z0"] + 5 - z_uscita))
    parti.append(("loop di accoppiamento", acc, "#424242"))

    # Coassiale dall'SO-239 (parete verso il palo) al palo e giù; cavo dei 24 V dal PG7 della scatola del motore
    z_fondo = -lx_m - PALO_SOTTO + 40
    y_cavi = yc + c.PALO_D / 2 + 6
    zs = z_lt - 8
    coax = (cq.Workplane("YZ").workplane(offset=cp + c.PALO_D / 2 + 4).center(yc, zs).circle(3.5)
            .extrude(g["sc"]["x0"] - 20 - (cp + c.PALO_D / 2 + 4))
            .union(cq.Workplane("XY").workplane(offset=z_fondo).center(cp + c.PALO_D / 2 + 4, yc).circle(3.5)
                   .extrude(zs - z_fondo)))
    parti.append(("coassiale", coax, "#212121"))
    x_pg, y_pg, z_pg = pg7[1], pg7[0], -lx_m - c.PARETE
    cavo24 = cq.Workplane("XY").workplane(offset=z_fondo).center(x_pg, y_pg).circle(3).extrude(z_pg - z_fondo)
    parti.append(("cavo 24 V", cavo24, "#37474f"))

    # Palo
    z0, z1 = -lx_m - PALO_SOTTO, z_braccio + PALO_SOPRA
    palo = cq.Workplane("XY").workplane(offset=z0).center(cp, yc).circle(c.PALO_D / 2).extrude(z1 - z0)
    parti.append(("palo", palo, "#8d8d8d"))
    pos = {"xa": xa, "yc": yc, "z_lt": zc + r, "cp": cp}
    quote = {"altezza dal fondo della scatola del motore alla cima dell'anello": zc + r + v.TUBO_ANELLO / 2 + lx_m,
             "piano dell'anello dall'asse del palo": xa - cp,
             "centro dell'anello sopra il fondo della scatola del motore": zc + lx_m}
    return parti, quote, pos


def main():
    parti, quote, _ = costruisci()
    for k, val in quote.items():
        print(f"{k}: {val:.0f} mm")
    # STEP dell'assieme, con i colori
    ass = cq.Assembly()
    for nome, o, col in parti:
        ass.add(o, name=nome, color=cq.Color(col))
    os.makedirs(os.path.join(QUI, "step"), exist_ok=True)
    ass.save(os.path.join(QUI, "step", "assieme.step"))
    print("step/assieme.step")
    if "--niente-anteprime" in sys.argv:
        return 0

    import anteprime as a
    cart = os.path.join(QUI, "anteprime")

    def tri(o, nome):
        return a.triangoli(o, tol=1.5 if nome in ("anello", "loop di accoppiamento", "palo", "coassiale") else 0.4)
    tutto = [(tri(o, n), col) for n, o, col in parti]
    a.figura(tutto, os.path.join(cart, "assieme.png"), 12, -35,
             "Antenna RX: anello da 1 m sopra la scatola del variabile, scatola del motore sotto, sul palo")
    a.figura(tutto, os.path.join(cart, "assieme-fronte.png"), 0, 0,
             "Assieme visto di fronte (loop di accoppiamento e braccio in cima: proposta)")
    a.figura(tutto, os.path.join(cart, "assieme-lato.png"), 0, -90,
             "Assieme visto di lato: piano dell'anello, scatole e palo")
    vicino = [(tri(o, n), col) for n, o, col in parti
              if n not in ("anello", "loop di accoppiamento", "braccio in PVC", "raccordo a T", "palo",
                           "coassiale", "cavo 24 V") and not n.startswith("cima")]
    a.figura(vicino, os.path.join(cart, "assieme-scatole.png"), 18, -40,
             "Le due scatole: motore sotto, variabile sopra, tubo isolante tra le borchie, capi dell'anello")
    # Dettaglio della cima
    cima_parti = [(tri(o, n), col) for n, o, col in parti
                  if n.startswith("cima") or n in ("braccio in PVC", "raccordo a T")]
    # tratto dell'anello e del loop vicino alla cima (taglio con un box)
    xs = [p for p in parti if p[0] == "cima sostegno"][0][1].val().BoundingBox()
    finestra = cq.Workplane("XY").box(400, 300, 260).translate((xs.center.x - 60, xs.center.y, xs.zmax - 120))
    for n in ("anello", "loop di accoppiamento", "palo", "coassiale"):
        o = [p for p in parti if p[0] == n][0]
        cima_parti.append((a.triangoli(o[1].intersect(finestra), tol=0.4), o[2]))
    a.figura(cima_parti, os.path.join(cart, "assieme-cima.png"), 20, -50,
             "Cima: braccio in PVC Ø 40 con raccordo a T, sostegno con la sede dell'anello e la scatoletta SO-239, loop piccolo")
    return 0


if __name__ == "__main__":
    sys.exit(main())
