#!/usr/bin/env python3
"""Gruppo palo e braccio in PVC Ø 40 dell'antenna RX: tavola con quote, punti delle fascette e distinta (A4 verticale).

Le quote vengono dagli stessi script delle scatole e della cima (assieme.py), così restano coerenti.
Uso:  python3 gruppo_palo.py   -> gruppo-palo.pdf e anteprime/gruppo-palo.png
"""
import importlib
import math
import os
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, Rectangle

QUI = os.path.dirname(os.path.abspath(__file__))
BLU = "#1a237e"

PALO_D = 40.0
PALO_SOTTO = 300.0               # palo sotto la scatola del motore, per il fissaggio al balcone (da decidere)
TEE_Z_SOCKET = 22.0              # raccordo a T: dall'asse al fondo della bicchieratura del ramo (misurare sul pezzo)
SPINTA_VENTO = 25.0              # N sull'anello a circa 100 km/h (stima da verificare)


def quote():
    import contenitore as c
    off = 2 * (c.PARETE + c.BOSS_SPORGENZA) + 12.0
    m = dict(lx=c.LX, staffa=c.STAFFA_X, blocchi=list(c.BLOCCHI_X), cp=c.centro_palo())
    import scatola_variabile as v
    v.imposta()
    vv = dict(lx=v.LX, staffa=c.STAFFA_X, blocchi=list(c.BLOCCHI_X), blocco=v.BLOCCO_S + v.COLLARE)
    importlib.reload(c)
    import assieme
    import cima
    _, _, pos = assieme.costruisci()
    g = cima.geometria(pos["xa"], pos["yc"], pos["z_lt"])
    z_tee = g["z_a"]
    # quote verticali nel sistema dell'assieme (Z in alto), poi riferite all'asse del raccordo a T
    q = {
        "tee": z_tee,
        "fascette variabile": [b + off for b in vv["blocchi"]],
        "fascette motore": [-b for b in m["blocchi"]],
        "scatola variabile": (-c.PARETE - c.BOSS_SPORGENZA + off, vv["lx"] + c.PARETE + vv["blocco"] + off),
        "scatola motore": (-(m["lx"] + c.PARETE), c.PARETE + c.BOSS_SPORGENZA),
        "anello": (pos["z_lt"] - 1000 - 6.35 + 0, pos["z_lt"] + 6.35),
        "piano anello": pos["xa"], "cp": m["cp"], "x_fine": g["x_fine"],
    }
    q["fondo palo"] = q["scatola motore"][0] - PALO_SOTTO
    return q


def freccia_v(ax, x, y0, y1, testo, sinistra=True, fs=7):
    ax.add_patch(FancyArrowPatch((x, y0), (x, y1), arrowstyle="<|-|>", mutation_scale=6, lw=0.6, color=BLU))
    ax.text(x + (-1.2 if sinistra else 1.2), (y0 + y1) / 2, testo, rotation=90, fontsize=fs, color=BLU,
            ha="right" if sinistra else "left", va="center")


def main():
    q = quote()
    s = 0.1                                            # scala 1:10
    fig = plt.figure(figsize=(210 / 25.4, 297 / 25.4))
    ax = fig.add_axes([0, 0, 1, 1])
    ax.set_xlim(0, 210)
    ax.set_ylim(0, 297)
    ax.set_aspect("equal")
    ax.axis("off")
    ax.add_patch(Rectangle((8, 8), 194, 281, fc="none", ec="black", lw=1))
    X0, Y0 = 62, 30 - q["fondo palo"] * s              # posizione dell'asse del palo sulla tavola

    def P(xa, za):                                     # assieme (x dal palo verso chi guarda, z in alto) -> tavola
        return (X0 + (xa - q["cp"]) * s, Y0 + za * s)

    # palo, raccordo a T, braccio
    zt = q["tee"]
    pb, pt = P(q["cp"] - PALO_D / 2, q["fondo palo"]), P(q["cp"] + PALO_D / 2, zt + 30)
    ax.add_patch(Rectangle(pb, pt[0] - pb[0], pt[1] - pb[1], fc="#eceff1", ec="black", lw=0.7))
    a0, a1 = P(q["cp"], zt - PALO_D / 2), P(q["x_fine"], zt + PALO_D / 2)
    ax.add_patch(Rectangle(a0, a1[0] - a0[0], a1[1] - a0[1], fc="#eceff1", ec="black", lw=0.7))
    t0, t1 = P(q["cp"] - 24, zt - 32), P(q["cp"] + 24, zt + 32)
    ax.add_patch(Rectangle(t0, t1[0] - t0[0], t1[1] - t0[1], fc="#b0bec5", ec="black", lw=0.7))
    # scatole e anello (sagome)
    for nome, col in (("scatola motore", "#dfe3e8"), ("scatola variabile", "#dfe3e8")):
        z0, z1 = q[nome]
        b0, b1 = P(q["cp"] + PALO_D / 2 + 22, z0), P(q["cp"] + PALO_D / 2 + 22 + 100, z1)
        ax.add_patch(Rectangle(b0, b1[0] - b0[0], b1[1] - b0[1], fc=col, ec="#555555", lw=0.6))
        ax.text((b0[0] + b1[0]) / 2, (b0[1] + b1[1]) / 2, nome.replace("scatola ", "scatola del\n"), fontsize=6,
                ha="center", va="center", color="#444444")
    za0, za1 = q["anello"]
    p0, p1 = P(q["piano anello"], q["scatola variabile"][1]), P(q["piano anello"], za1)
    ax.plot([p0[0], p1[0]], [p0[1], p1[1]], color="#b87333", lw=2.2)
    ax.text(p1[0] + 1.5, (p0[1] + p1[1]) / 2, "anello Ø 1 m (di taglio)", rotation=90, fontsize=6.5, color="#8d4f1f",
            va="center")
    s0 = P(q["x_fine"] - 25, zt - 10)
    ax.add_patch(Rectangle((s0[0], s0[1] - 4), 3.5, 9, fc="#cfd8dc", ec="black", lw=0.5))
    ax.text(s0[0] + 5, s0[1] + 6, "sostegno (cima.py)", fontsize=6, va="bottom")

    # fascette: tacche sul palo
    righe = []
    for nome in ("fascette variabile", "fascette motore"):
        for z in q[nome]:
            l0, l1 = P(q["cp"] - PALO_D / 2 - 3, z), P(q["cp"] + PALO_D / 2 + 3, z)
            ax.plot([l0[0], l1[0]], [l0[1], l1[1]], color="#c62828", lw=1.2)
            righe.append((zt - z, nome))
    # quote dall'asse del raccordo a T verso il basso
    xq = P(q["cp"] - PALO_D / 2, 0)[0] - 4
    livelli = sorted([(zt - z, n) for z, n in [(z, "fascetta") for z in q["fascette variabile"] + q["fascette motore"]]]
                     + [(zt - q["scatola motore"][0], "fondo scatola motore"), (zt - q["fondo palo"], "fondo palo")])
    for i, (d, n) in enumerate(livelli):
        x = xq - 6 * i
        freccia_v(ax, x, P(0, zt)[1], P(0, zt - d)[1], f"{d:.0f}", fs=6.5)
        ax.plot([x - 1, P(q["cp"] - PALO_D / 2, 0)[0]], [P(0, zt - d)[1]] * 2, color=BLU, lw=0.3)
    ax.plot([xq - 6 * len(livelli), P(q["cp"] - PALO_D / 2, 0)[0]], [P(0, zt)[1]] * 2, color=BLU, lw=0.3)
    # braccio
    yb = P(0, zt + PALO_D / 2)[1] + 6
    xa0, xa1 = P(q["cp"], 0)[0], P(q["x_fine"], 0)[0]
    ax.add_patch(FancyArrowPatch((xa0, yb), (xa1, yb), arrowstyle="<|-|>", mutation_scale=6, lw=0.6, color=BLU))
    braccio = q["x_fine"] - q["cp"]
    ax.text((xa0 + xa1) / 2, yb + 1.5, f"{braccio:.0f} dall'asse del palo", fontsize=6.5, color=BLU, ha="center",
            va="bottom")

    # testo: distinta, tagli, rigidità
    L_palo = zt - q["fondo palo"]
    taglio_braccio = braccio - TEE_Z_SOCKET
    ei_pvc = 3000 * math.pi / 64 * (40 ** 4 - 34 ** 4)          # N·mm² (PVC 40×3, E ≈ 3 GPa)
    ei_legno = 10000 * math.pi / 64 * 32 ** 4                    # tondino di legno Ø 32 dentro (E ≈ 10 GPa)
    sbalzo = zt - (q["fascette motore"][0])                      # dalla fascetta più alta del fissaggio (stima)
    sbalzo = 1200.0
    d_pvc = SPINTA_VENTO * sbalzo ** 3 / (3 * ei_pvc)
    d_rinf = SPINTA_VENTO * sbalzo ** 3 / (3 * (ei_pvc + ei_legno))
    testo = [
        "GRUPPO PALO E BRACCIO IN PVC Ø 40 (antenna RX) · scala 1:10 · quote in mm",
        "",
        "Distinta",
        f"1  palo PVC Ø 40 × 3 (PN16, pressione): circa {L_palo:.0f} fino all'asse del T",
        "   (taglio = questa quota meno la bicchieratura del T; gli ultimi",
        f"   {PALO_SOTTO:.0f} sotto la scatola del motore servono al fissaggio, da decidere)",
        "2  raccordo a T 90° Ø 40 da incollare (non il T da scarico a 87°30')",
        f"3  braccio PVC Ø 40 × 3: taglio circa {taglio_braccio:.0f} ( {braccio:.0f} meno {TEE_Z_SOCKET:.0f} dall'asse",
        "   del T al fondo della bicchieratura: misurare il raccordo)",
        "4  tappo Ø 40 sul T in alto · colla per PVC",
        "5  sostegno, cappello, fondo, anello di supporto (cima.py) · bullone M5×60",
        "6  2 staffe a V stampate (una per scatola) · 4 fascette stringitubo inox",
        "7  rinforzo isolante dentro il palo: tondino di legno Ø 32 trattato,",
        "   o tubo in vetroresina (mai metallo nel tratto dell'anello)",
        "",
        "Tacche rosse: centri delle fascette stringitubo (dall'asse del T verso il basso):",
    ]
    for d, n in sorted(righe):
        testo.append(f"   {d:6.0f}  {n.replace('fascette ', 'staffa della scatola del ')}")
    testo += [
        f"   {zt - q['scatola motore'][0]:6.0f}  fondo della scatola del motore",
        f"   {L_palo:6.0f}  fondo del palo (fissaggio al balcone da decidere)",
        "",
        f"Rigidità (stima da verificare): sbalzo di circa {sbalzo / 1000:.1f} m, {SPINTA_VENTO:.0f} N di vento sull'anello",
        f"   PVC 40×3 da solo: la cima si sposta di circa {d_pvc:.0f} mm  ->  troppo",
        f"   con il tondino di legno Ø 32 dentro: circa {d_rinf:.0f} mm",
        "   Fissare il palo al balcone in due punti il più distanti possibile.",
    ]
    ax.text(110, 282, "\n".join(testo), fontsize=6.4, va="top", ha="left", family="DejaVu Sans", linespacing=1.35)
    ax.text(105, 12, "loopant · gruppo palo RX · 27/11/2026 · rev. A", fontsize=7, ha="center")
    os.makedirs(os.path.join(QUI, "anteprime"), exist_ok=True)
    fig.savefig(os.path.join(QUI, "gruppo-palo.pdf"))
    fig.savefig(os.path.join(QUI, "anteprime", "gruppo-palo.png"), dpi=200, facecolor="white")
    print(f"palo fino all'asse del T: {L_palo:.0f} mm; braccio dall'asse: {braccio:.0f} mm")
    for d, n in sorted(righe):
        print(f"  fascetta a {d:.0f} mm sotto l'asse del T ({n})")
    print(f"  fondo scatola motore a {zt - q['scatola motore'][0]:.0f}, fondo palo a {L_palo:.0f}")
    print(f"rigidità: PVC {d_pvc:.0f} mm, con legno {d_rinf:.0f} mm")


if __name__ == "__main__":
    sys.exit(main())
