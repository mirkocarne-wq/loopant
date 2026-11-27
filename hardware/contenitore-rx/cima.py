#!/usr/bin/env python3
"""Pezzi stampabili per la cima dell'antenna RX (PROPOSTA da confermare).

- sostegno: si infila sulla punta del braccio in PVC (Ø 40, bullone M5 di traverso), ha la sede per la cima
  dell'anello di rame e, dietro, una scatoletta con il connettore SO-239 rivolto verso il palo; dal fondo della
  scatoletta escono in basso i due capi del loop di accoppiamento;
- cappello: chiude la cima dell'anello nella sede (4 viti M3 in inserti a caldo);
- fondo: chiude la scatoletta da sotto (4 viti M3), con i fori per i capi del loop;
- anello di supporto del loop di accoppiamento;
- raccordo: T, braccio e tappo in un pezzo solo, stampato in due metà (A e B) che stringono la cima del palo in PVC
  Ø 40 con viti M5; una vite passa anche nel palo (niente rotazioni). Il sostegno si infila sulla punta del braccio.
Coordinate dell'assieme (assieme.py): Z in alto, X dal palo verso chi guarda, Y orizzontale.

Uso:  python3 cima.py   -> stl/cima-*.stl, step/cima-*.step (posizioni prese dall'assieme)
"""
import math
import os
import sys

import cadquery as cq

import contenitore as c
import scatola_variabile as v

QUI = os.path.dirname(os.path.abspath(__file__))

BRACCIO_D = 40.0                 # braccio Ø 40 (parte del raccordo stampato; va bene anche un tubo in PVC da 40)
PALO_D = 40.0                    # palo in PVC (indicazione dell'utente): misurare il diametro vero con il calibro
TUBO_ANELLO = v.TUBO_ANELLO      # stesso tubo della scatola del variabile (un solo parametro)
SEDE_LATO = TUBO_ANELLO / 2 + 7.0  # semilarghezza (X) del blocco della sede e del cappello
ACC_TUBO = 6.0                   # loop di accoppiamento in tubetto di rame da 6 mm (o RG-58)
ACC_FORO = 6.6
ACC_INTERASSE = 12.0             # tra i due capi del loop all'uscita dal fondo
SO239_FORO, SO239_FORI = 16.0, 18.2   # foro centrale e interasse dei 4 fori della flangia quadrata da 25,4 mm
DISTANZA_BRACCIO = TUBO_ANELLO / 2 + 6.0 + 20.0  # dall'asse dell'anello all'asse del braccio (6 mm tra le sedi)
SPORGENZA_BRACCIO = 7.0          # il braccio finisce oltre il piano dell'anello
X_INDIETRO = 24.0 + SEDE_LATO + 1.0  # faccia posteriore del sostegno dietro il piano dell'anello (scatoletta + sede)
SCATOLETTA = (24.0, 32.0, 43.0)  # scatoletta: profondità (X), larghezza (Y), altezza (Z)
PARETE = 3.0

# --- Supporto del loop di accoppiamento (anello di plastica con la gola per il conduttore) ---
ACCOPPIAMENTO_D = 200.0          # diametro del loop piccolo (asse del conduttore), circa 1/5 dell'anello
SUP_LARGO, SUP_SPESSO = 12.0, 8.0  # sezione dell'anello di supporto (radiale × assiale)
LINGUETTA_SOPRA = 8.0            # linguetta sopra l'anello di supporto, fino al fondo della scatoletta
STELO_L = 30.0                   # stelo sotto il fondo con le asole di regolazione
CORSA_REGOLAZIONE = 22.0         # di quanto si può abbassare il loop piccolo (per il ROS minimo)
# --- Raccordo stampato (T + braccio + tappo), due metà divise sul piano verticale che contiene palo e braccio ---
RACC_D = 56.0                    # diametro esterno della bicchieratura sul palo
RACC_GIU = 75.0                  # bicchieratura sotto l'asse del braccio
PALO_DENTRO = 22.0               # cima del palo sopra l'asse del braccio (battuta contro il tappo)
RACC_TAPPO = 6.0                 # spessore del tappo sopra il palo
GIOCO_PALO = 0.4                 # sul diametro del foro del palo
FESSURA = 0.6                    # tra le due metà: stringendo le viti si serra il palo
PINNA = (22.0, 24.0)             # pinna posteriore per le viti di serraggio: sporgenza dal foro, spessore (Y)
GOCCIA = 22.5                    # foro del palo a goccia tronca (stampa senza supporti), dall'asse
DADO_M5, DADO_M5_H = 9.4, 4.6    # sede esagonale del dado M5 (sui vertici) e profondità
ANGOLI_FASCETTE = [-90, -50, -10, 30, 150, 190, 230]  # gradi nel piano del loop (0 = +Y, 90 = in alto)


def geometria(xa, yc, z_lt):
    """Posizioni principali: xa piano dell'anello, yc centro, z_lt asse del tubo in cima all'anello."""
    z_a = z_lt + DISTANZA_BRACCIO
    x0 = xa - X_INDIETRO
    sx, sy, sz = SCATOLETTA
    sc = dict(x0=x0, x1=x0 + sx, y0=yc - sy / 2, y1=yc + sy / 2, z0=z_lt - 28.0, z1=z_lt - 28.0 + sz)
    x_acc = (sc["x0"] + PARETE + sc["x1"] - PARETE) / 2   # piano del loop di accoppiamento
    return dict(z_a=z_a, x0=x0, x_fine=xa + SPORGENZA_BRACCIO, sc=sc, x_acc=x_acc)


def _box(x0, x1, y0, y1, z0, z1, r=0.0):
    return c.scatola_arrotondata(x0, y0, z0, x1, y1, z1, r)


def _cil_y(x, y0, z, lungo, d):
    return cq.Workplane("XZ").workplane(offset=-y0).center(x, z).circle(d / 2).extrude(-lungo)


def viti_cappello(xa, yc, z_lt):
    return [(xa + dx, yc + dy) for dx in (-(TUBO_ANELLO / 2 + 4.5), TUBO_ANELLO / 2 + 4.5) for dy in (-9.0, 9.0)]


def viti_fondo(g, yc):
    sc = g["sc"]
    return [(sc["x0"] + PARETE + 3.0, yc + dy) for dy in (-10.0, 10.0)] + \
           [(sc["x1"] - PARETE - 3.0, yc + dy) for dy in (-10.0, 10.0)]


def geometria_acc(g, yc, abbassa=0.0):
    """Loop di accoppiamento: centro, raggio, piano. abbassa = regolazione verso il basso (0 = più in alto)."""
    zf = g["sc"]["z0"] - PARETE                       # faccia di sotto del fondo della scatoletta
    r = ACCOPPIAMENTO_D / 2
    cima_sup = zf - LINGUETTA_SOPRA - abbassa         # bordo esterno in cima all'anello di supporto
    return dict(xc=g["x_acc"], yc=yc, zc=cima_sup - r - SUP_LARGO / 2, r=r, zf=zf, cima_sup=cima_sup)


def anello_accoppiamento(xa, yc, z_lt, abbassa=0.0):
    """Anello di supporto del loop piccolo: gola a scatto sul davanti, fessure per fascette, linguetta in cima."""
    g = geometria(xa, yc, z_lt)
    a = geometria_acc(g, yc, abbassa)
    xc, zc, r = a["xc"], a["zc"], a["r"]
    x0 = xc - 6.0                                     # faccia posteriore; davanti a xc + 2: la gola è a scatto
    s = c.cilindro_x(x0, yc, zc, SUP_SPESSO, 2 * r + SUP_LARGO).cut(
        c.cilindro_x(x0 - 1, yc, zc, SUP_SPESSO + 2, 2 * r - SUP_LARGO))
    # linguetta in cima, più spessa davanti per contenere i canali dei capi
    zt = a["cima_sup"] + LINGUETTA_SOPRA
    s = s.union(_box(x0, xc + 5, yc - 16, yc + 16, zc + r - 4, zt, 1.5))
    # tutti i tagli in una volta sola (tagli in sequenza lasciavano un canale chiuso a metà)
    utensili = [cq.Workplane("XY").add(cq.Solid.makeTorus(r, ACC_FORO / 2, cq.Vector(xc, yc, zc), cq.Vector(1, 0, 0)))]
    for dy in (-ACC_INTERASSE / 2, ACC_INTERASSE / 2):
        # Ø 7: non tangente alla gola (Ø 6,6), che altrimenti dà una maglia aperta nello STL
        utensili.append(c.cilindro_z(xc, yc + dy, zc + r - 6, zt - (zc + r - 6) + 1, ACC_FORO + 0.4))
    for dy in (-12.0, 12.0):                          # 2 viti M3 verso lo stelo del fondo
        utensili.append(cq.Workplane("YZ").workplane(offset=x0 - 1).center(yc + dy, a["cima_sup"] + 4).circle(1.7)
                        .extrude(xc + 5 - x0 + 2))
    for ang in ANGOLI_FASCETTE:                       # coppie di fessure per le fascette
        for rr in (r - 4.6, r + 4.6):
            utensili.append(cq.Workplane("XY").box(SUP_SPESSO + 2, 4.5, 2.0).translate((0, 0, rr))
                            .rotate((0, 0, 0), (1, 0, 0), ang - 90).translate((x0 + SUP_SPESSO / 2, yc, zc)))
    taglio = utensili[0]
    for u in utensili[1:]:
        taglio = taglio.union(u)
    s = s.cut(taglio)
    return s


def sostegno(xa, yc, z_lt):
    g = geometria(xa, yc, z_lt)
    z_a, x0, xf, sc = g["z_a"], g["x0"], g["x_fine"], g["sc"]
    rb = BRACCIO_D / 2
    # collare attorno alla punta del braccio e blocco pieno fino alla cima dell'anello
    collare = c.cilindro_x(x0, yc, z_a, xf + 5 - x0, BRACCIO_D + 12)
    blocco = _box(x0, xa + SEDE_LATO, yc - 16, yc + 16, z_lt, z_a + rb + 6, 2.0)
    s = collare.union(blocco).union(_box(sc["x0"], sc["x1"], sc["y0"], sc["y1"], sc["z0"], sc["z1"], 2.0))
    # sede del braccio (aperta verso il palo), bullone M5 di traverso
    s = s.cut(c.cilindro_x(x0 - 1, yc, z_a, xf - x0 + 1, BRACCIO_D + 0.5))
    s = s.cut(_cil_y(xa - 20, yc - 40, z_a, 80, 5.5))
    # mezza sede della cima dell'anello (l'altra metà è nel cappello)
    s = s.cut(_cil_y(xa, yc - 30, z_lt, 60, TUBO_ANELLO + 0.6))
    for x, y in viti_cappello(xa, yc, z_lt):
        s = s.cut(c.cilindro_z(x, y, z_lt - 1, 8, c.INSERTO_D))
    # scatoletta: vano aperto in basso, SO-239 sulla parete verso il palo, colonnine per le viti del fondo
    vano = _box(sc["x0"] + PARETE, sc["x1"] - PARETE, sc["y0"] + PARETE, sc["y1"] - PARETE,
                sc["z0"] - 1, sc["z1"] - PARETE)
    s = s.cut(vano)
    for x, y in viti_fondo(g, yc):
        s = s.union(_box(x - 3, x + 3, y - 3, y + 3, sc["z0"], sc["z0"] + 8))
        s = s.cut(c.cilindro_z(x, y, sc["z0"] - 1, 8, c.INSERTO_D))
    zs = z_lt - 8.0
    s = s.cut(cq.Workplane("YZ").workplane(offset=sc["x0"] - 1).center(yc, zs).circle(SO239_FORO / 2)
              .extrude(PARETE + 2))
    for dy in (-SO239_FORI / 2, SO239_FORI / 2):
        for dz in (-SO239_FORI / 2, SO239_FORI / 2):
            s = s.cut(cq.Workplane("YZ").workplane(offset=sc["x0"] - 1).center(yc + dy, zs + dz).circle(1.6)
                      .extrude(PARETE + 2))
    return s


def cappello(xa, yc, z_lt):
    s = _box(xa - SEDE_LATO, xa + SEDE_LATO, yc - 16, yc + 16, z_lt - TUBO_ANELLO / 2 - 5, z_lt - 0.5, 2.0)
    s = s.cut(_cil_y(xa, yc - 30, z_lt, 60, TUBO_ANELLO + 0.6))
    for x, y in viti_cappello(xa, yc, z_lt):
        s = s.cut(c.cilindro_z(x, y, z_lt - 20, 30, 3.4))
        s = s.cut(c.cilindro_z(x, y, z_lt - TUBO_ANELLO / 2 - 6, 3.5, 6.2))
    return s


def fondo(xa, yc, z_lt):
    g = geometria(xa, yc, z_lt)
    sc = g["sc"]
    s = _box(sc["x0"], sc["x1"], sc["y0"], sc["y1"], sc["z0"] - PARETE, sc["z0"], 2.0)
    for x, y in viti_fondo(g, yc):
        s = s.cut(c.cilindro_z(x, y, sc["z0"] - PARETE - 1, PARETE + 2, 3.4))
    for dy in (-ACC_INTERASSE / 2, ACC_INTERASSE / 2):
        s = s.cut(c.cilindro_z(g["x_acc"], yc + dy, sc["z0"] - PARETE - 1, PARETE + 2, ACC_FORO))
    s = s.cut(c.cilindro_z(sc["x1"] - PARETE - 3.0, yc, sc["z0"] - PARETE - 1, PARETE + 2, 2.5))  # scolo
    # stelo con due asole verticali per la linguetta dell'anello di supporto (regolazione verso il basso)
    a = geometria_acc(g, yc)
    xs = a["xc"] + 5.0
    zf = a["zf"]
    s = s.union(_box(xs, xs + 4, yc - 16, yc + 16, zf - STELO_L, zf + 0.1, 1.0))
    for dy in (-12.0, 12.0):
        zm = a["cima_sup"] + 4 - CORSA_REGOLAZIONE / 2
        s = s.cut(cq.Workplane("YZ").workplane(offset=xs - 1).center(yc + dy, zm)
                  .slot2D(CORSA_REGOLAZIONE + 3.4, 3.4, 90).extrude(6))
    return s


def viti_raccordo(g, cp):
    """Viti M5 lungo Y: (x, z, semilunghezza in Y della faccia piana, nome)."""
    za = g["z_a"]
    xp = cp - PALO_D / 2 - PINNA[0] / 2 - 1.0
    return [(xp, za - RACC_GIU + 12, PINNA[1] / 2, "pinna in basso"),
            (xp, za + PALO_DENTRO - 4, PINNA[1] / 2, "pinna in alto"),
            (cp, za - 45.0, RACC_D / 2 + 5.0, "attraverso il palo"),
            (cp + RACC_D / 2 + 10.0, za, BRACCIO_D / 2 + 1.0, "radice del braccio")]


def raccordo(xa, yc, z_lt, cp):
    """T stampato: bicchieratura sul palo chiusa in alto, braccio Ø 40 fino alla punta, pinna posteriore. Due metà."""
    g = geometria(xa, yc, z_lt)
    za, xf = g["z_a"], g["x_fine"]
    z0, z1 = za - RACC_GIU, za + PALO_DENTRO + RACC_TAPPO
    s = cq.Workplane("XY").workplane(offset=z0).center(cp, yc).circle(RACC_D / 2).extrude(z1 - z0)
    s = s.union(c.cilindro_x(cp, yc, za, xf - cp, BRACCIO_D))
    # raccordo conico tra bicchieratura e braccio (resta fuori dal sostegno, che parte da g["x0"])
    s = s.union(cq.Workplane("XY").add(cq.Solid.makeCone(RACC_D / 2, BRACCIO_D / 2, 40.0,
                                                         cq.Vector(cp + 18.0, yc, za), cq.Vector(1, 0, 0))))
    s = s.union(_box(cp - PALO_D / 2 - PINNA[0] - 2, cp - PALO_D / 2, yc - PINNA[1] / 2, yc + PINNA[1] / 2, z0, z1, 2.0))
    # borchie per la vite che passa nel palo (la parete sopra la goccia è troppo sottile per la sede del dado)
    for x, z, sy, nome in viti_raccordo(g, cp):
        if nome == "attraverso il palo":
            s = s.union(_cil_y(x, yc - sy, z, 2 * sy, 16.0))
    # foro del palo a goccia tronca verso ±Y (ogni metà si stampa sulla faccia di taglio)
    r = (PALO_D + GIOCO_PALO) / 2
    t = r / math.sqrt(2)
    xg = r * math.sqrt(2) - GOCCIA
    pts = [(t, t), (xg, GOCCIA), (-xg, GOCCIA), (-t, t), (-t, -t), (-xg, -GOCCIA), (xg, -GOCCIA), (t, -t)]
    foro = (cq.Workplane("XY").workplane(offset=z0 - 1).center(cp, yc).circle(r).extrude(za + PALO_DENTRO - z0 + 1)
            .union(cq.Workplane("XY").workplane(offset=z0 - 1).polyline([(cp + px, yc + py) for px, py in pts]).close()
                   .extrude(za + PALO_DENTRO - z0 + 1)))
    s = s.cut(foro)
    for x, z, sy, _ in viti_raccordo(g, cp):
        s = s.cut(_cil_y(x, yc - 40, z, 80, 5.5))
        # facce piane per testa e dado, sede esagonale del dado sulla metà B (+Y)
        s = s.cut(_cil_y(x, yc - 40, z, 40 - sy, 11.0)).cut(_cil_y(x, yc + sy, z, 40, 11.0))
        s = s.cut(cq.Workplane("XZ").workplane(offset=-(yc + sy - DADO_M5_H)).center(x, z).polygon(6, DADO_M5)
                  .extrude(-(DADO_M5_H + 1)))
    s = s.cut(cq.Workplane("XY").box(400, FESSURA, 400).translate((cp + 100, yc, za)))
    meta_a = s.intersect(cq.Workplane("XY").box(400, 100, 400).translate((cp + 100, yc - 50, za)))
    meta_b = s.intersect(cq.Workplane("XY").box(400, 100, 400).translate((cp + 100, yc + 50, za)))
    return meta_a, meta_b


def pezzi(xa, yc, z_lt, cp=None):
    p = {"sostegno": sostegno(xa, yc, z_lt), "cappello": cappello(xa, yc, z_lt), "fondo": fondo(xa, yc, z_lt),
         "anello accoppiamento": anello_accoppiamento(xa, yc, z_lt)}
    if cp is not None:
        p["raccordo A"], p["raccordo B"] = raccordo(xa, yc, z_lt, cp)
    return p


def main():
    import assieme
    _, _, pos = assieme.costruisci()
    p = pezzi(pos["xa"], pos["yc"], pos["z_lt"], pos["cp"])
    c.QUI = QUI
    c.esporta({
        # faccia posteriore (verso il palo) sul piatto: la sede del braccio resta verticale
        "cima-sostegno": c.sul_piatto(p["sostegno"].rotate((0, 0, 0), (0, 1, 0), -90)),
        "cima-cappello": c.sul_piatto(p["cappello"]),           # sede verso l'alto
        "cima-fondo": c.sul_piatto(p["fondo"].rotate((0, 0, 0), (1, 0, 0), 180)),   # stelo verso l'alto
        # anello di supporto piatto, gola verso l'alto
        "cima-anello-accoppiamento": c.sul_piatto(p["anello accoppiamento"].rotate((0, 0, 0), (0, 1, 0), -90)),
        # metà del raccordo sulla faccia di taglio (A verso -Y, B verso +Y)
        "cima-raccordo-a": c.sul_piatto(p["raccordo A"].rotate((0, 0, 0), (1, 0, 0), -90)),
        "cima-raccordo-b": c.sul_piatto(p["raccordo B"].rotate((0, 0, 0), (1, 0, 0), 90)),
    })
    return 0


if __name__ == "__main__":
    sys.exit(main())
