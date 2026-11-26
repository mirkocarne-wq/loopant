#!/usr/bin/env python3
"""Disegno in sezione del condensatore a stantuffo dell'antenna TX, con i collegamenti all'anello.

Misure del progetto base (docs/tecnico/04-tx-condensatore-stantuffo.md), STIME DA VERIFICARE.
Uso:  python3 disegno.py   -> disegno-condensatore-tx.png e .svg nella stessa cartella
"""
import os

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, Polygon, Rectangle
from matplotlib.path import Path
from matplotlib.patches import PathPatch

QUI = os.path.dirname(os.path.abspath(__file__))

# --- Misure (mm) ---
EST_D, EST_S, EST_L = 28.0, 1.0, 220.0       # tubo esterno 28×1
PTFE_DI, PTFE_DE = 22.0, 25.0                # manicotto
PTFE_SPORGE = 20.0                           # oltre la bocca del tubo esterno
PIST_D, PIST_S, PIST_L = 22.0, 1.0, 220.0    # pistone 22×1
FERMO = 10.0                                 # 5 rondelle PTFE da 2 mm sul fondo
CORSA = 150.0
INSERITO = 60.0                              # posizione disegnata: punta del pistone a 60 mm dal fondo
ASTA_D, ASTA_L = 8.0, 160.0

RAME, RAME_SCURO = "#d08a4a", "#8a4a1a"
PTFE, PTFE_BORDO = "#eaf6ff", "#5b8db8"
ISOLANTE = "#f5f0d0"
GRIGIO = "#9e9e9e"


def rett(ax, x0, x1, y0, y1, fc, ec="black", lw=0.8, hatch=None, z=2, ls="-"):
    ax.add_patch(Rectangle((x0, y0), x1 - x0, y1 - y0, fc=fc, ec=ec, lw=lw, hatch=hatch, zorder=z, ls=ls))


def tubo(ax, x0, x1, d_est, s, fc, hatch=None, z=2, ls="-", ec="black"):
    r = d_est / 2
    rett(ax, x0, x1, r - s, r, fc, hatch=hatch, z=z, ls=ls, ec=ec)
    rett(ax, x0, x1, -r, -r + s, fc, hatch=hatch, z=z, ls=ls, ec=ec)


def nota(ax, testo, xy, xytesto, **kw):
    ax.annotate(testo, xy=xy, xytext=xytesto, fontsize=9.5, ha=kw.pop("ha", "left"), va="center",
                arrowprops=dict(arrowstyle="-", color="#333333", lw=0.7, shrinkA=0, shrinkB=2), zorder=10, **kw)


def quota(ax, x0, x1, y, testo, dy=4):
    ax.add_patch(FancyArrowPatch((x0, y), (x1, y), arrowstyle="<|-|>", mutation_scale=9, lw=0.8, color="#1a237e",
                                 zorder=9))
    for x in (x0, x1):
        ax.plot([x, x], [y - 3, y + 3], color="#1a237e", lw=0.6, zorder=9)
    ax.text((x0 + x1) / 2, y + dy, testo, ha="center", va="bottom" if dy > 0 else "top", fontsize=9,
            color="#1a237e", zorder=10)


def geometria(ax):
    """Disegna tutti i pezzi (senza quote e richiami); restituisce le posizioni utili."""

    # Tappo di fondo con sfiato, tubo esterno, fermo di rondelle, manicotto
    rett(ax, -3, 0, -EST_D / 2 - 1, EST_D / 2 + 1, RAME, hatch="////")
    ax.add_patch(Rectangle((-3.2, -EST_D / 2 - 1.2), 3.4, 2.2, fc="white", ec="white", zorder=3))  # sfiato
    tubo(ax, 0, EST_L, EST_D, EST_S, RAME, hatch="////")
    for k in range(5):
        rett(ax, k * 2, k * 2 + 2, 4, PTFE_DE / 2, PTFE, ec=PTFE_BORDO, lw=0.6)
        rett(ax, k * 2, k * 2 + 2, -PTFE_DE / 2, -4, PTFE, ec=PTFE_BORDO, lw=0.6)
    tubo(ax, FERMO, EST_L + PTFE_SPORGE, PTFE_DE, (PTFE_DE - PTFE_DI) / 2, PTFE, z=3, ec=PTFE_BORDO)

    # Pistone con disco saldato dentro la punta, tappo stampato in coda per l'asta isolante
    xp0 = INSERITO
    xp1 = xp0 + PIST_L
    tubo(ax, xp0, xp1, PIST_D, PIST_S, RAME, hatch="\\\\\\\\", z=4)
    rett(ax, xp0, xp0 + 2, -PIST_D / 2 + PIST_S, PIST_D / 2 - PIST_S, RAME, hatch="\\\\\\\\", z=4)
    rett(ax, xp1 - 15, xp1 + 3, -PIST_D / 2 + PIST_S, PIST_D / 2 - PIST_S, ISOLANTE, z=4)       # tappo isolante
    # pistone tutto fuori (tratteggiato): punta a FERMO + CORSA
    tubo(ax, FERMO + CORSA, FERMO + CORSA + PIST_L, PIST_D, PIST_S, "none", z=1, ls="--", ec=GRIGIO)

    # Asta isolante e attuatore lineare (carrello, guida, NEMA11)
    xa1 = xp1 + 3 + ASTA_L
    rett(ax, xp1 + 3, xa1, -ASTA_D / 2, ASTA_D / 2, ISOLANTE, z=4)
    rett(ax, xa1, xa1 + 30, -22, 22, "#cfd8dc", z=4)
    rett(ax, xa1 - 10, xa1 + 230, -34, -22, "#eceff1", z=3)
    rett(ax, xa1 + 230, xa1 + 270, -14, 14, "#424242", z=4)
    ax.text(xa1 + 250, 0, "NEMA11", color="white", fontsize=8, ha="center", va="center", rotation=90, zorder=5)

    # Collare e piattina sul tubo esterno -> capo 1 dell'anello
    xc = 40
    rett(ax, xc - 8, xc + 8, EST_D / 2, EST_D / 2 + 4, RAME_SCURO, z=5)
    rett(ax, xc - 1, xc + 1, EST_D / 2 + 4, 52, RAME_SCURO, z=5)
    rett(ax, xc - 11, xc + 11, 52, 140, RAME, hatch="////", z=4)                       # capo 1 (tubo 22)
    # Treccia piatta dalla coda del pistone -> capo 2 dell'anello, con ansa più lunga della corsa
    xt = xp1 - 8
    rett(ax, xt - 6, xt + 6, PIST_D / 2, PIST_D / 2 + 4, RAME_SCURO, z=5)               # collare sul pistone
    xcapo2 = xp1 + 140
    verts = [(xt, PIST_D / 2 + 4), (xt, 70), (xt + 10, 118), (xt + 70, 118), (xcapo2 - 40, 118),
             (xcapo2 - 10, 100), (xcapo2 - 11, 52)]
    codes = [Path.MOVETO, Path.CURVE4, Path.CURVE4, Path.CURVE4, Path.CURVE4, Path.CURVE4, Path.CURVE4]
    ax.add_patch(PathPatch(Path(verts, codes), fc="none", ec=RAME_SCURO, lw=5, zorder=5, capstyle="round"))
    ax.add_patch(PathPatch(Path(verts, codes), fc="none", ec="#e0b080", lw=2.2, zorder=6, ls=(0, (2, 1.5))))
    rett(ax, xcapo2 - 11, xcapo2 + 11, 52, 140, RAME, hatch="////", z=4)               # capo 2 (tubo 22)
    return dict(xp0=xp0, xp1=xp1, xa1=xa1, xc=xc, xt=xt, xcapo2=xcapo2)


def main():
    fig = plt.figure(figsize=(17, 9.6), dpi=150)
    ax = fig.add_axes([0.0, 0.34, 1.0, 0.6])
    ax.set_aspect("equal")
    ax.axis("off")
    g = geometria(ax)
    xp0, xp1, xa1, xc, xt, xcapo2 = g["xp0"], g["xp1"], g["xa1"], g["xc"], g["xt"], g["xcapo2"]

    # distanza della treccia dalla bocca del tubo esterno
    ax.add_patch(FancyArrowPatch((EST_L + 1, EST_D / 2 + 1), (xt - 1, 60), arrowstyle="<->", mutation_scale=8,
                                 lw=0.7, color="#c62828", ls="--", zorder=8))
    ax.text((EST_L + xt) / 2 - 22, 46, "≥ 25–30 mm\n(4–5 kV di picco)", color="#c62828", fontsize=8.5,
            ha="center", zorder=10)

    # Quote
    quota(ax, 0, EST_L, -48, "tubo esterno 28×1, circa 220")
    quota(ax, EST_L, EST_L + PTFE_SPORGE, 22, "PTFE sporge\n20", dy=3)
    quota(ax, 0, FERMO, -32, "fermo 10", dy=-3)
    quota(ax, xp0, xp1, -64, "pistone 22×1, circa 220")
    quota(ax, FERMO, FERMO + CORSA, -82, "corsa 150 (servono circa 110–140 per 20 e 40 m)")
    quota(ax, xp1 + 3, xa1, -48, "asta isolante Ø 8, circa 160")

    # Richiami
    nota(ax, "tappo a saldare con sfiato da 2 mm (in basso)", (-2, -EST_D / 2), (-70, -40), ha="right")
    nota(ax, "5 rondelle PTFE da 2 mm: fermo isolante,\nalmeno 10 mm tra punta del pistone e fondo", (5, 8),
         (-70, 30), ha="right")
    nota(ax, "manicotto PTFE continuo 22/25\n(non rondelle impilate)", (120, PTFE_DE / 2 - 0.7), (120, 70),
         ha="center")
    nota(ax, "disco di rame saldato DENTRO la punta,\nspigolo arrotondato", (xp0 + 1, 3), (-70, 0), ha="right")
    nota(ax, "collare e piattina 20–25 × 1 mm", (xc + 6, EST_D / 2 + 3), (-70, 62), ha="right")
    nota(ax, "capo 1 dell'anello (tubo 22)", (xc - 11, 120), (-70, 120), ha="right")
    nota(ax, "treccia piatta di rame stagnato,\ncirca 20 mm (10–16 mm²),\nansa più lunga della corsa", (xt + 60, 118),
         (xt + 30, 160), ha="center")
    nota(ax, "capo 2 dell'anello (tubo 22)", (xcapo2 + 11, 120), (xcapo2 + 40, 120))
    nota(ax, "collare sulla coda del pistone", (xt + 6, PIST_D / 2 + 3), (xt + 60, 34))
    nota(ax, "tappo isolante: unisce pistone e asta", (xp1 - 5, -7), (xp1 + 30, -100))
    nota(ax, "carrello dell'attuatore lineare (vite T6×1, corsa 150)", (xa1 + 15, -22), (xa1 + 40, -100))
    nota(ax, "pistone tutto fuori (tratteggio)", (FERMO + CORSA + PIST_L - 20, -PIST_D / 2), (FERMO + CORSA + 120, -30))

    ax.set_xlim(-250, xa1 + 290)
    ax.set_ylim(-110, 175)
    # Dettagli ingranditi
    for pos, (x0, x1, y0, y1), titolo in [
            ([0.03, 0.04, 0.44, 0.27], (-6, 75, -17, 17), "Dettaglio del fondo: tappo con sfiato, 5 rondelle PTFE (fermo 10 mm), "
                                                           "punta del pistone con il disco"),
            ([0.53, 0.04, 0.44, 0.27], (195, 265, -17, 17), "Dettaglio della bocca: il PTFE sporge 20 mm oltre il tubo esterno")]:
        axi = fig.add_axes(pos)
        axi.set_aspect("equal")
        axi.set_xticks([]); axi.set_yticks([])
        for lato in axi.spines.values():
            lato.set_color("#bbbbbb")
        geometria(axi)
        axi.set_xlim(x0, x1)
        axi.set_ylim(y0, y1)
        axi.set_title(titolo, fontsize=9.5)
    ax.set_title("Condensatore a stantuffo dell'antenna TX: sezione e collegamenti all'anello "
                 "(misure del progetto base, stime da verificare)", fontsize=12.5)
    fig.text(0.5, 0.008, "Rame: tratteggio arancio · PTFE: azzurro · Isolanti (asta, tappo): giallo chiaro · "
             "Pistone disegnato con la punta a 60 mm dal fondo", ha="center", fontsize=9, color="#555555")
    for est in ("png", "svg"):
        fig.savefig(os.path.join(QUI, f"disegno-condensatore-tx.{est}"), facecolor="white")
    print("disegno-condensatore-tx.png / .svg")


if __name__ == "__main__":
    main()
