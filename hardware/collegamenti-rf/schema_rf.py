#!/usr/bin/env python3
"""Schema dei collegamenti RF della magnetic loop (RX e TX): percorso del segnale, circuito equivalente,
cablaggio del loop di accoppiamento.  Uso: python3 schema_rf.py -> schema-rf.png e .svg
"""
import math
import os

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Arc, Circle, FancyArrowPatch, Rectangle

QUI = os.path.dirname(os.path.abspath(__file__))
RAME, COAX, ROSSO, BLU, GRIGIO = "#b87333", "#212121", "#c62828", "#1a237e", "#757575"


def scatola(ax, x0, y0, x1, y1, testo, fc="#eceff1", fs=8.5, va="center"):
    ax.add_patch(Rectangle((x0, y0), x1 - x0, y1 - y0, fc=fc, ec="black", lw=0.9, zorder=3))
    ax.text((x0 + x1) / 2, (y0 + y1) / 2 if va == "center" else y1 - 1.2, testo, ha="center",
            va="center" if va == "center" else "top", fontsize=fs, zorder=4)


def ferrite(ax, x, y, verticale=True):
    w, h = (2.2, 3.6) if verticale else (3.6, 2.2)
    ax.add_patch(Rectangle((x - w / 2, y - h / 2), w, h, fc="#424242", ec="black", lw=0.5, zorder=5))


def nota(ax, testo, xy, xyt, colore="#333333", ha="left", fs=8):
    ax.annotate(testo, xy=xy, xytext=xyt, fontsize=fs, ha=ha, va="center", color=colore,
                arrowprops=dict(arrowstyle="-|>", mutation_scale=7, lw=0.6, color=colore, shrinkA=1, shrinkB=1),
                zorder=6)


def bobina(ax, x, y0, y1, giri=4, lato=1, colore="black"):
    """Bobina verticale fatta di semicerchi."""
    h = (y1 - y0) / giri
    for i in range(giri):
        ax.add_patch(Arc((x, y0 + h * (i + 0.5)), h, h, theta1=-90 if lato > 0 else 90,
                         theta2=90 if lato > 0 else 270, lw=1.4, color=colore))


def condensatore_var(ax, x, y, h=6, colore="black", orizzontale=True):
    z = 6
    if orizzontale:   # armature verticali, terminali a sinistra e a destra
        ax.plot([x - 1, x - 1], [y - h / 2, y + h / 2], color=colore, lw=2, zorder=z)
        ax.plot([x + 1, x + 1], [y - h / 2, y + h / 2], color=colore, lw=2, zorder=z)
    else:
        ax.plot([x - h / 2, x + h / 2], [y + 1, y + 1], color=colore, lw=2, zorder=z)
        ax.plot([x - h / 2, x + h / 2], [y - 1, y - 1], color=colore, lw=2, zorder=z)
    ax.add_patch(FancyArrowPatch((x - h / 2 - 1, y - h / 2 - 1), (x + h / 2 + 1, y + h / 2 + 1), arrowstyle="-|>",
                                 mutation_scale=7, lw=0.9, color=colore, zorder=z))


def pannello_antenna(ax):
    ax.set_xlim(-6, 106)
    ax.set_ylim(-20, 122)
    ax.set_aspect("equal")
    ax.axis("off")
    ax.set_title("1 · Collegamenti come sono sull'antenna", fontsize=11, loc="left")
    # anello grande con i capi in basso
    cx, cy, r = 45, 70, 34
    ax.add_patch(Arc((cx, cy), 2 * r, 2 * r, theta1=-80, theta2=260, lw=4, color=RAME, zorder=2))
    xa = cx + r * math.cos(math.radians(-80))
    xb = cx + r * math.cos(math.radians(260))
    ya = cy + r * math.sin(math.radians(-80))
    for x in (xa, xb):
        ax.plot([x, x], [ya, 30], color=RAME, lw=4, zorder=2)
    # scatola del variabile con il condensatore tra i due capi
    scatola(ax, 30, 16, 60, 32, "", fc="#f5f5f5")
    ax.text(45, 17.8, "scatola del variabile", fontsize=7.5, ha="center", va="bottom", color=GRIGIO, zorder=4)
    ax.plot([xb, xb, 43.4], [30, 25, 25], color=RAME, lw=2.2, zorder=4)
    ax.plot([xa, xa, 46.6], [30, 25, 25], color=RAME, lw=2.2, zorder=4)
    condensatore_var(ax, 45, 25, h=5)
    # scatola del motore e tubo isolante
    scatola(ax, 32, 0, 58, 10, "scatola del motore\n(SERVO42D o NEMA11)", fs=7.5)
    ax.plot([45, 45], [10, 16], color="#9e9e9e", lw=3, ls=(0, (2, 1)), zorder=2)
    nota(ax, "tubo / asta isolante:\nnessun collegamento elettrico", (45, 13), (63, 8), fs=7.5)
    # loop di accoppiamento in cima, con la scatoletta del connettore
    lx, ly, lr = 45, 89, 9
    ax.add_patch(Arc((lx, ly), 2 * lr, 2 * lr, theta1=100, theta2=440, lw=2, color=COAX, zorder=3))
    x1 = lx + lr * math.cos(math.radians(80))
    x2 = lx + lr * math.cos(math.radians(100))
    y1 = ly + lr * math.sin(math.radians(80))
    for x in (x1, x2):
        ax.plot([x, x], [y1, 99], color=COAX, lw=2, zorder=3)
    scatola(ax, 40.5, 99, 49.5, 102.5, "SO-239", fs=6.5, fc="#cfd8dc")
    # coassiale fino alla radio, con ferriti vicino all'antenna e in casa
    ax.plot([49.5, 92, 92, 92], [100.7, 100.7, -5, -5], color=COAX, lw=2.2, zorder=2)
    for y in (96, 91, 86, 81):
        ferrite(ax, 92, y)
    ferrite(ax, 92, 2)
    scatola(ax, 80, -18, 104, -6, "RADIO\n(in casa)", fc="#e3f2fd", fs=8.5)
    nota(ax, "ferriti sul coassiale\n(mix 31) vicino all'antenna", (93.5, 88), (60, 116), fs=7.5)
    nota(ax, "coassiale 50 Ω:\nRG-58 (RX) · RG-213 (TX)", (92, 40), (60, 50), fs=7.5)
    # 24 V e ESP-NOW
    scatola(ax, -4, -18, 18, -6, "alimentatore 24 V\n(in casa)", fc="#fff8e1", fs=7.5)
    ax.plot([7, 7, 32], [-6, 5, 5], color="#5d4037", lw=1.4, zorder=2)
    ferrite(ax, 7, -2)
    ferrite(ax, 26, 5, verticale=False)
    ax.text(-3, 11, "24 V: solo alimentazione,\nnon è RF", fontsize=7.5, color="#5d4037")
    ax.text(22, -15, "pannello ↔ antenna:\nESP-NOW, senza fili", fontsize=7.5, color=GRIGIO, ha="left")
    # messaggi chiave
    nota(ax, "il coassiale finisce qui:\nal loop PICCOLO", (45, 101), (3, 116), colore=ROSSO, fs=8)
    nota(ax, "nessun contatto tra loop piccolo\ne anello grande: accoppiamento magnetico", (52, 82), (52, 66),
         colore=ROSSO, fs=7.8)
    nota(ax, "i due capi dell'anello vanno\nai due terminali del condensatore", (xb, 34), (-4, 40), colore=ROSSO,
         fs=7.8)


def pannello_circuito(ax):
    ax.set_xlim(0, 100)
    ax.set_ylim(0, 46)
    ax.set_aspect("equal")
    ax.axis("off")
    ax.set_title("2 · Circuito equivalente", fontsize=11, loc="left")
    # radio e coassiale
    scatola(ax, 1, 13, 15, 33, "radio\n50 Ω", fc="#e3f2fd")
    ax.plot([15, 30], [31, 31], color=COAX, lw=2)
    ax.plot([15, 30], [15, 15], color=COAX, lw=2)
    ax.text(22.5, 33, "coassiale", fontsize=7.5, ha="center")
    # loop piccolo L1
    bobina(ax, 30, 15, 31, giri=4, lato=1)
    ax.text(30, 11.5, "L1\nloop piccolo", fontsize=7.8, ha="center", va="top")
    # accoppiamento
    for dx in (-0.6, 0.6):
        ax.plot([38.5 + dx, 38.5 + dx], [14, 32], color="#555555", lw=1)
    ax.text(38.5, 35, "accoppiamento\nmagnetico (k)", fontsize=7.5, ha="center", va="bottom", color=ROSSO)
    # anello L2 + perdite R + condensatore C, chiuso
    bobina(ax, 47, 15, 31, giri=4, lato=-1, colore=RAME)
    ax.text(47, 11.5, "L2\nanello grande", fontsize=7.8, ha="center", va="top")
    ax.plot([47, 47, 62], [31, 38, 38], color=RAME, lw=1.6)
    ax.add_patch(Rectangle((62, 36.5), 8, 3, fc="white", ec="black", lw=1))
    ax.text(66, 41, "R: perdite + radiazione\n(piccola: pochi mΩ–Ω)", fontsize=7, ha="center", va="bottom")
    ax.plot([70, 82, 82], [38, 38, 26], color=RAME, lw=1.6)
    condensatore_var(ax, 82, 23, h=6, orizzontale=False)
    ax.plot([82, 82, 47, 47], [20, 8, 8, 15], color=RAME, lw=1.6)
    ax.text(86, 23, "C\nvariabile", fontsize=7.8, va="center")
    ax.text(50, 2.5, "L2 e C sono un circuito risonante: il motore varia C finché f = 1 / (2π·√(L2·C)) è la "
            "frequenza voluta.\nIl loop piccolo preleva il segnale e adatta l'impedenza a 50 Ω.",
            fontsize=7.6, ha="center", va="bottom")


def pannello_cablaggio(ax):
    ax.set_xlim(0, 100)
    ax.set_ylim(0, 48)
    ax.set_aspect("equal")
    ax.axis("off")
    ax.set_title("3 · Cablaggio del loop piccolo al connettore", fontsize=11, loc="left")
    # (a) tubetto o filo
    cx, cy, r = 22, 22, 13
    ax.add_patch(Arc((cx, cy), 2 * r, 2 * r, theta1=100, theta2=440, lw=2.5, color=RAME))
    xa, xb = cx + r * math.cos(math.radians(80)), cx + r * math.cos(math.radians(100))
    ya = cy + r * math.sin(math.radians(80))
    ax.add_patch(Circle((cx, 41), 3.2, fc="#cfd8dc", ec="black", lw=0.9))
    ax.add_patch(Circle((cx, 41), 0.8, fc="#ffd54f", ec="black", lw=0.6))
    ax.plot([xa, xa, cx + 0.8], [ya, 41, 41], color=RAME, lw=1.6)
    ax.plot([xb, xb, cx - 3.2], [ya, 38.5, 38.5], color=RAME, lw=1.6)
    ax.text(cx + 5, 44.5, "SO-239", fontsize=7.5)
    nota(ax, "capo A → centrale", (cx + 0.8, 41), (cx + 12, 37), fs=7.5)
    nota(ax, "capo B → massa (flangia)", (cx - 3.2, 38.5), (cx + 12, 32), fs=7.5)
    ax.text(cx, 3, "(a) tubetto di rame Ø 6 o filo grosso:\nsemplice, va bene per iniziare", fontsize=7.6,
            ha="center")
    # (b) Faraday schermato in coassiale
    cx, cy, r = 72, 22, 13
    ax.add_patch(Arc((cx, cy), 2 * r, 2 * r, theta1=-80, theta2=260, lw=4.5, color=COAX))
    ax.add_patch(Arc((cx, cy), 2 * r, 2 * r, theta1=-80, theta2=260, lw=1.0, color="#ffd54f"))
    # interruzione della calza in alto (resta il centrale)
    ax.add_patch(Arc((cx, cy), 2 * r, 2 * r, theta1=85, theta2=95, lw=6, color="white"))
    ax.add_patch(Arc((cx, cy), 2 * r, 2 * r, theta1=84, theta2=96, lw=1.0, color="#ffd54f"))
    nota(ax, "in alto: 1 cm senza calza,\nil centrale continua", (cx, cy + r), (cx + 10, 44), fs=7.5)
    xa = cx + r * math.cos(math.radians(-80))
    xb = cx + r * math.cos(math.radians(260))
    ya = cy + r * math.sin(math.radians(-80))
    ax.plot([xb, xb], [ya, 4], color=COAX, lw=3)
    ax.plot([xa, xa], [ya, 6], color=COAX, lw=3)
    nota(ax, "un capo: centrale del loop → centrale\ndel coassiale verso la radio", (xb, 6), (cx - 32, 9), fs=7.2)
    nota(ax, "l'altro capo: centrale e calza del loop\ninsieme → calza del coassiale", (xa, 7), (cx + 6, 3),
         fs=7.2)
    ax.text(cx, 46.5, "(b) loop schermato in coassiale (Faraday)", fontsize=7.6, ha="center")
    ax.text(cx, cy, "meno sensibile ai\ndisturbi elettrici\n(balcone in città)", fontsize=7.2, color=GRIGIO,
            ha="center", va="center")


def main():
    fig = plt.figure(figsize=(17, 10.6), dpi=150)
    pannello_antenna(fig.add_axes([0.0, 0.02, 0.5, 0.92]))
    pannello_circuito(fig.add_axes([0.5, 0.52, 0.49, 0.42]))
    pannello_cablaggio(fig.add_axes([0.5, 0.04, 0.49, 0.44]))
    fig.suptitle("Collegamenti RF della magnetic loop (RX e TX)", fontsize=14, x=0.02, ha="left", y=0.985)
    fig.text(0.5, 0.005, "Terminali del condensatore · RX (variabile): statore e telaio, oppure i due statori se "
             "isolati · TX (stantuffo): tubo esterno (collare e piattina) e pistone (treccia).   Loop piccolo: circa "
             "1/5 dell'anello (20 cm RX, 30 cm TX), da regolare per il ROS minimo.", ha="center", fontsize=8.5,
             color="#333333")
    for est in ("png", "svg"):
        fig.savefig(os.path.join(QUI, f"schema-rf.{est}"), facecolor="white")
    print("schema-rf.png / .svg")


if __name__ == "__main__":
    main()
