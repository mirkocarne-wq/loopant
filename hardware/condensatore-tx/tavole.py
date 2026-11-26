#!/usr/bin/env python3
"""Tavole per il tornitore: pezzi del condensatore a stantuffo dell'antenna TX (A4 orizzontale, PDF e PNG).

Misure del progetto base (docs/tecnico/04-tx-condensatore-stantuffo.md), da verificare. I diametri di accoppiamento
si adattano ai tubi veri, misurati dal tornitore: nelle note sono scritti come formule.
Uso:  python3 tavole.py   -> tavole-condensatore-tx.pdf e tavola-*.png
"""
import math
import os
import textwrap

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
from matplotlib.patches import Circle, FancyArrowPatch, Polygon, Rectangle

QUI = os.path.dirname(os.path.abspath(__file__))
DATA = "26/11/2026"
BLU = "#1a237e"
RAME, PTFE, POM, OTTONE = "#f6e2cf", "#e3f2fd", "#fff8d6", "#f3e7b8"

# --- Misure (mm) ---
EST_DE, EST_S, EST_L = 28.0, 1.0, 220.0
PIST_DE, PIST_S, PIST_L = 22.0, 1.0, 220.0
PTFE_DI, PTFE_DE, PTFE_L = 22.3, 25.8, 230.0
FLANGIA_D, FLANGIA_S, SPORGENZA = 32.0, 3.0, 20.0


# ======================================================================= strumenti di disegno
class Tavola:
    def __init__(self, pdf, numero, titolo, materiale, quantita, scala, note):
        self.pdf = pdf
        self.fig = plt.figure(figsize=(297 / 25.4, 210 / 25.4))
        a = self.ax = self.fig.add_axes([0, 0, 1, 1])
        a.set_xlim(0, 297)
        a.set_ylim(0, 210)
        a.set_aspect("equal")
        a.axis("off")
        a.add_patch(Rectangle((10, 10), 277, 190, fc="none", ec="black", lw=1.1))
        x0, y0, w, h = 150, 10, 137, 34
        a.add_patch(Rectangle((x0, y0), w, h, fc="white", ec="black", lw=1.0))
        a.plot([x0, x0 + w], [y0 + 23, y0 + 23], color="black", lw=0.6)
        a.plot([x0, x0 + w], [y0 + 11.5, y0 + 11.5], color="black", lw=0.6)
        a.plot([x0 + 92, x0 + 92], [y0, y0 + 23], color="black", lw=0.6)
        a.text(x0 + 3, y0 + 28.5, f"loopant · condensatore a stantuffo TX · tav. {numero}", fontsize=8, va="center")
        a.text(x0 + w - 3, y0 + 28.5, f"{DATA} · rev. A", fontsize=7.5, va="center", ha="right")
        a.text(x0 + 3, y0 + 17.2, titolo, fontsize=10.5, va="center", weight="bold")
        a.text(x0 + 3, y0 + 5.7, materiale, fontsize=7.8, va="center")
        a.text(x0 + 95, y0 + 17.2, f"Quantità: {quantita}", fontsize=8, va="center")
        a.text(x0 + 95, y0 + 5.7, f"Scala {scala}", fontsize=8, va="center")
        righe = ["Quote in mm · tolleranze generali ISO 2768-m · spigoli vivi smussati 0,3×45° salvo indicazione."]
        for par in note:
            righe += textwrap.wrap(par, 92)
        a.text(13, 12, "\n".join(righe), fontsize=7.0, va="bottom", ha="left", linespacing=1.3)

    def chiudi(self, nome):
        self.fig.savefig(os.path.join(QUI, f"tavola-{nome}.png"), dpi=170, facecolor="white")
        self.pdf.savefig(self.fig)
        plt.close(self.fig)


def poli(ax, pts, fc, hatch="////", lw=0.9, ls="-", ec="black"):
    ax.add_patch(Polygon(pts, closed=True, fc=fc, ec=ec, lw=lw, hatch=hatch, ls=ls))


def asse_orizz(ax, x0, x1, y):
    ax.plot([x0, x1], [y, y], color="#555555", lw=0.55, ls=(0, (14, 3, 2, 3)))


def interruzione(ax, x, y0, y1):
    """Linea di interruzione a zig-zag."""
    n = 6
    ys = [y0 + (y1 - y0) * i / n for i in range(n + 1)]
    xs = [x + (1.2 if i % 2 else -1.2) for i in range(n + 1)]
    ax.plot(xs, ys, color="black", lw=0.6)


def hquota(ax, x1, x2, y, testo, y_rif=None, fs=8, sopra=True):
    for x, yr in ((x1, y_rif), (x2, y_rif)):
        if yr is not None:
            ax.plot([x, x], [yr, y + (1.5 if y > yr else -1.5)], color=BLU, lw=0.45)
    ax.add_patch(FancyArrowPatch((x1, y), (x2, y), arrowstyle="<|-|>", mutation_scale=7, lw=0.6, color=BLU))
    ax.text((x1 + x2) / 2, y + (1.6 if sopra else -1.6), testo, fontsize=fs, color=BLU, ha="center",
            va="bottom" if sopra else "top")


def vquota(ax, y1, y2, x, testo, x_rif=None, fs=8, sinistra=True):
    for y, xr in ((y1, x_rif), (y2, x_rif)):
        if xr is not None:
            ax.plot([xr, x + (1.5 if x > xr else -1.5)], [y, y], color=BLU, lw=0.45)
    ax.add_patch(FancyArrowPatch((x, y1), (x, y2), arrowstyle="<|-|>", mutation_scale=7, lw=0.6, color=BLU))
    ax.text(x + (-1.6 if sinistra else 1.6), (y1 + y2) / 2, testo, fontsize=fs, color=BLU, rotation=90,
            ha="right" if sinistra else "left", va="center")


def richiamo(ax, testo, punto, posto, ha="left", fs=7.6):
    ax.annotate(testo, xy=punto, xytext=posto, fontsize=fs, ha=ha, va="center",
                arrowprops=dict(arrowstyle="-|>", mutation_scale=6, lw=0.5, color="#333333", shrinkA=1, shrinkB=0))


def titoletto(ax, x, y, testo):
    ax.text(x, y, testo, fontsize=8.5, ha="center", va="center", style="italic", color="#333333")


def tubo_spezzato(ax, X, Y, s, tratti, profilo, fc, hatch):
    """Disegna un pezzo assialsimmetrico in sezione completa, a vista interrotta.
    tratti: lista di (x0, x1) in coordinate del pezzo, mostrati uno dopo l'altro con 18 mm di spazio fra loro.
    profilo(x0, x1): poligono (x, r) della metà superiore del tratto [x0, x1], r = 0 sull'asse.
    Restituisce la funzione di trasformazione pezzo -> tavola."""
    mappe = []
    xp = X
    for x0, x1 in tratti:
        mappe.append((x0, x1, xp))
        xp += (x1 - x0) * s + 18

    def T(x, r):
        for x0, x1, xs in mappe:
            if x0 - 1e-6 <= x <= x1 + 1e-6:
                return (xs + (x - x0) * s, Y + r * s)
        raise ValueError(x)

    for i, (x0, x1, xs) in enumerate(mappe):
        for k in (1, -1):
            pts = [(xs + (x - x0) * s, Y + k * r * s) for x, r in profilo(x0, x1)]
            poli(ax, pts, fc, hatch)
        if i:
            xa = mappe[i - 1][2] + (mappe[i - 1][1] - mappe[i - 1][0]) * s
            r = max(abs(r) for _, r in profilo(x0, x1))
            interruzione(ax, xa + 0.5, Y - r * s - 3, Y + r * s + 3)
            interruzione(ax, xs - 0.5, Y - r * s - 3, Y + r * s + 3)
    xfine = mappe[-1][2] + (mappe[-1][1] - mappe[-1][0]) * s
    asse_orizz(ax, X - 6, xfine + 6, Y)
    return T


def anello_tubo(x0, x1, r_e, r_i):
    return [(x0, r_i), (x0, r_e), (x1, r_e), (x1, r_i)]


# ======================================================================= tavole
def tav_distinta(pdf):
    tv = Tavola(pdf, 0, "Distinta e montaggio", "—", "—", "—", [
        "Misure del progetto base, da verificare: i diametri di accoppiamento si adattano ai tubi veri (vedi le note "
        "di ogni tavola).",
        "Montaggio: tubo esterno con tappo saldato, 5 rondelle PTFE da 2 mm sul fondo, manicotto con lo spallamento "
        "sulla bocca, pistone con disco saldato in punta e tappo isolante in coda."])
    a = tv.ax
    righe = [("Tav.", "Pezzo", "Materiale", "Q.tà", "Lavorazione"),
             ("1", "Manicotto dielettrico con spallamento", "PTFE vergine", "1", "tornitura, foro lucido"),
             ("2", "Pistone (tubo 22×1 del cliente)", "rame", "1", "taglio, sedi, 2 fori, lucidatura"),
             ("3", "Tubo esterno (tubo 28×1 del cliente)", "rame", "1", "taglio, bocca raccordata"),
             ("4", "Disco della punta", "rame Cu-ETP", "1", "tornitura"),
             ("4", "Tappo isolante pistone ↔ asta", "POM-C", "1", "tornitura, fori"),
             ("5", "Collari A (pistone) e B (tubo esterno)", "ottone CuZn39Pb3", "1 + 1", "tornitura, taglio, filetti M4")]
    xs = [16, 30, 112, 160, 180]
    y = 188
    for k, r in enumerate(righe):
        for xi, c in zip(xs, r):
            a.text(xi, y, c, fontsize=8.5, weight="bold" if k == 0 else "normal", va="center")
        a.plot([14, 283], [y - 4.5, y - 4.5], color="#aaaaaa", lw=0.5)
        y -= 9.5
    a.text(16, y - 2, "Fuori distinta, a cura del cliente: tappo a saldare Ø 28 con sfiato Ø 2, rondelle PTFE da 2 mm, "
           "asta in vetroresina Ø 8, viti M3 e M4,\ntreccia piatta di rame stagnato, piattina 20–25 × 1 mm, saldature.",
           fontsize=7.8, va="top")
    # schema del montaggio, sezione semplificata (scala 0,9)
    s, X, Y = 0.8, 24, 80

    def p(x, r):
        return (X + x * s, Y + r * s)
    for k in (1, -1):
        poli(a, [p(0, 13 * k), p(0, 14 * k), p(220, 14 * k), p(220, 13 * k)], RAME)
        poli(a, [p(-2, -15 * k), p(0, -15 * k), p(0, 15 * k), p(-2, 15 * k)], RAME)
        poli(a, [p(10, 11.15 * k), p(10, 12.9 * k), p(217, 12.9 * k), p(217, 16 * k), p(220, 16 * k),
                 p(220, 12.9 * k), p(230, 12.9 * k), p(230, 11.15 * k)], PTFE, "xx")
        poli(a, [p(0, 4 * k), p(10, 4 * k), p(10, 12.9 * k), p(0, 12.9 * k)], PTFE, "...")
        poli(a, [p(60, 10 * k), p(60, 11 * k), p(280, 11 * k), p(280, 10 * k)], RAME, "\\\\\\\\")
        poli(a, [p(60, 0), p(63, 0), p(63, 10 * k), p(60, 10 * k)], RAME, "\\\\\\\\")
        poli(a, [p(265, 4 * k), p(265, 10 * k), p(283, 10 * k), p(283, 8 * k), p(303, 8 * k), p(303, 4 * k)], POM, "")
    asse_orizz(a, X - 8, X + 310 * s, Y)
    for x, r, testo in [(110, 14, "tubo esterno · tav. 3"), (224, 16, "manicotto · tav. 1"),
                        (5, -13, "rondelle PTFE (fermo 10)"), (61.5, -6, "disco · tav. 4"),
                        (170, -11, "pistone · tav. 2"), (290, -8, "tappo isolante · tav. 4")]:
        q = p(x, r)
        richiamo(a, testo, q, (q[0], Y + (30 if r > 0 else -30)), ha="center")
    tv.chiudi("0-distinta")


def tav_manicotto(pdf):
    tv = Tavola(pdf, 1, "Manicotto dielettrico", "PTFE vergine (barra o tubo semilavorato)", "1",
                "2,5:1 (vista interrotta)", [
                    "Foro Ø 22,3 = Ø esterno del pistone misurato + 0,3: il pistone deve scorrere senza gioco "
                    "apprezzabile. Esterno Ø 25,8 = Ø interno del tubo esterno misurato − 0,2: deve entrare a mano, "
                    "senza aria in mezzo.",
                    "Foro lucido Ra ≤ 1,6 senza righe. Parete sottile e lunga: lavorare con supporto, senza scaldare.",
                    "Lo spallamento appoggia sulla bocca del tubo esterno: il manicotto ne sporge di 20 (17 + 3)."])
    a = tv.ax
    ri, re, rf = PTFE_DI / 2, PTFE_DE / 2, FLANGIA_D / 2

    def prof(x0, x1):
        if x0 == 0:
            return [(0.5, ri), (0, ri + 0.5), (0, re - 0.5), (0.5, re), (17, re), (17, rf - 0.5), (17.5, rf),
                    (20, rf), (20, re), (x1, re), (x1, ri)]
        return [(x0, ri), (x0, re), (x1 - 0.5, re), (x1, re - 0.5), (x1, ri + 0.5), (x1 - 0.5, ri)]
    s, Y = 2.5, 128
    T = tubo_spezzato(a, 38, Y, s, [(0, 45), (200, 230)], prof, PTFE, "xx")
    ytop = Y + rf * s
    hquota(a, *[T(0, 0)[0], T(17, 0)[0]], ytop + 10, "17", y_rif=Y + re * s)
    hquota(a, T(17, 0)[0], T(20, 0)[0], ytop + 20, "3", y_rif=ytop)
    hquota(a, T(0, 0)[0], T(230, 0)[0], Y - rf * s - 12, "230 (vista interrotta)", y_rif=Y - re * s, sopra=False)
    vquota(a, Y - rf * s, Y + rf * s, T(18.5, 0)[0], "Ø 32")
    vquota(a, Y - re * s, Y + re * s, T(35, 0)[0], "Ø 25,8")
    vquota(a, Y - ri * s, Y + ri * s, T(215, 0)[0], "Ø 22,3")
    richiamo(a, "invito 0,5×45° nel foro, alle due estremità", T(0.2, ri + 0.3), (20, 188))
    richiamo(a, "raccordo R 0,5 sugli spigoli esterni", T(17.2, rf - 0.2), (115, 188))
    richiamo(a, "lato che sporge dal tubo esterno", T(0, -re), (25, 62))
    richiamo(a, "lato del fondo: appoggia sul fermo di rondelle", T(230, -re), (170, 62))
    tv.chiudi("1-manicotto")


def tav_pistone(pdf):
    tv = Tavola(pdf, 2, "Pistone: lavorazioni sul tubo", "tubo di rame 22×1, fornito dal cliente", "1",
                "3:1 (vista interrotta)", [
                    "Taglio a 220 ± 0,5 con facce in squadra. Esterno lucidato Ra ≤ 0,8 su tutta la lunghezza, senza "
                    "ammaccature né righe: scorre nel manicotto di PTFE.",
                    "Sedi Ø 20,0 H8 alesate solo quanto basta a pulire l'interno del tubo: in punta per il disco "
                    "(tav. 4, saldato a stagno), in coda per il codolo del tappo isolante (tav. 4).",
                    "Bordo esterno della punta raccordato R 1 (campo elettrico elevato)."])
    a = tv.ax
    re, ri = PIST_DE / 2, PIST_DE / 2 - PIST_S

    def prof(x0, x1):
        if x0 == 0:
            arco = [(1 - math.sin(t), re - 1 + math.cos(t)) for t in [math.pi / 2 * i / 6 for i in range(7)]]
            arco = [(1 - math.cos(t), re - 1 + math.sin(t)) for t in [math.pi / 2 * i / 6 for i in range(7)]]
            return [(0, ri)] + arco + [(x1, re), (x1, ri)]
        return [(x0, ri), (x0, re), (x1, re), (x1, ri)]
    s, Y = 3.0, 125
    T = tubo_spezzato(a, 38, Y, s, [(0, 30), (190, 220)], prof, RAME, "////")
    # disco in punta (tratteggio: pezzo della tav. 4)
    for k in (1, -1):
        poli(a, [T(0, 0), T(3, 0), T(3, k * ri), T(0, k * ri)], "none", "", ls="--", ec="#666666")
    # fori Ø 3,4 radiali in coda
    for k in (1, -1):
        c = T(212.5, k * (ri + re) / 2)
        a.add_patch(Rectangle((c[0] - 1.7 * s, c[1] - 0.6 * s), 3.4 * s, 1.2 * s, fc="white", ec="black", lw=0.6))
    ytop, ybot = Y + re * s, Y - re * s
    hquota(a, T(0, 0)[0], T(220, 0)[0], ybot - 14, "220 ± 0,5 (vista interrotta)", y_rif=ybot, sopra=False)
    hquota(a, T(0, 0)[0], T(3, 0)[0], ytop + 10, "3", y_rif=Y + ri * s)
    hquota(a, T(205, 0)[0], T(220, 0)[0], ytop + 10, "15", y_rif=Y + ri * s)
    hquota(a, T(212.5, 0)[0], T(220, 0)[0], ytop + 20, "7,5", y_rif=ytop)
    vquota(a, ybot, ytop, T(18, 0)[0], "Ø 22 (tubo)")
    vquota(a, Y - ri * s, Y + ri * s, T(10, 0)[0], "Ø 20,0 H8")
    vquota(a, Y - ri * s, Y + ri * s, T(200, 0)[0], "Ø 20,0 H8")
    richiamo(a, "R 1 sul bordo esterno della punta", T(0.3, re - 0.3), (18, 188))
    richiamo(a, "2 fori Ø 3,4 radiali a 180° (viti M3 nel tappo isolante)", T(212.5, -re), (130, 64))
    richiamo(a, "disco della punta (tav. 4), saldato a stagno nella sede", T(1.5, -4), (18, 63))
    richiamo(a, "coda: tappo isolante e collare A della treccia", T(220, -re), (175, 54))
    tv.chiudi("2-pistone")


def tav_tubo(pdf):
    tv = Tavola(pdf, 3, "Tubo esterno: lavorazioni", "tubo di rame 28×1, fornito dal cliente", "1",
                "3:1 (vista interrotta)", [
                    "Taglio a 220 ± 0,5 con facce in squadra. Interno pulito, sgrassato e senza bave.",
                    "Bocca raccordata R 1 dentro e fuori e lucidata (campo elettrico elevato).",
                    "Sul lato del fondo il cliente salda il tappo a saldare Ø 28 con il foro di sfiato Ø 2."])
    a = tv.ax
    re, ri = EST_DE / 2, EST_DE / 2 - EST_S

    def prof(x0, x1):
        if x0 == 0:
            return anello_tubo(0, x1, re, ri)
        return [(x0, ri), (x0, re), (x1 - 0.5, re), (x1, re - 0.5), (x1, ri + 0.5), (x1 - 0.5, ri)]
    s, Y = 3.0, 125
    T = tubo_spezzato(a, 38, Y, s, [(0, 25), (190, 220)], prof, RAME, "////")
    ytop, ybot = Y + re * s, Y - re * s
    hquota(a, T(0, 0)[0], T(220, 0)[0], ybot - 12, "220 ± 0,5 (vista interrotta)", y_rif=ybot, sopra=False)
    vquota(a, ybot, ytop, T(12, 0)[0], "Ø 28 (tubo)")
    vquota(a, Y - ri * s, Y + ri * s, T(205, 0)[0], "Ø 26 (interno: misurare)")
    richiamo(a, "lato del fondo: tappo a saldare con sfiato Ø 2 (del cliente)", T(0, re), (18, 188))
    richiamo(a, "bocca: R 1 dentro e fuori, lucidata", T(220, re - 0.3), (160, 188))
    tv.chiudi("3-tubo-esterno")


def tav_disco_tappo(pdf):
    tv = Tavola(pdf, 4, "Disco della punta e tappo isolante", "disco: rame Cu-ETP · tappo: POM-C (acetalica)", "1 + 1",
                "4:1 il disco, 3:1 il tappo", [
                    "Disco: Ø 20 f7 nella sede Ø 20,0 H8 del pistone, saldato a stagno; faccia verso il fondo raccordata "
                    "R 1,5.",
                    "Tappo: codolo Ø 20 f7 nella sede di coda del pistone, 2 fori filettati M3 radiali allineati ai fori "
                    "del pistone; foro Ø 8 H8 passante per l'asta in vetroresina, fermata da una spina Ø 3 trasversale."])
    a = tv.ax
    # --- disco (4:1)
    s, X, Y = 4.0, 48, 125
    r = 10.0
    arco = [(1.5 - 1.5 * math.cos(t), r - 1.5 + 1.5 * math.sin(t)) for t in [math.pi / 2 * i / 6 for i in range(7)]]
    for k in (1, -1):
        poli(a, [(X + x * s, Y + k * y * s) for x, y in [(0, 0)] + arco + [(3, r), (3, 0)]], RAME, "////")
    asse_orizz(a, X - 8, X + 3 * s + 8, Y)
    vquota(a, Y - r * s, Y + r * s, X + 3 * s + 14, "Ø 20 f7", x_rif=X + 3 * s, sinistra=False)
    hquota(a, X, X + 3 * s, Y + r * s + 9, "3", y_rif=Y + r * s)
    richiamo(a, "R 1,5", (X + 0.3 * s, Y + (r - 0.3) * s), (X - 26, Y + r * s + 10))
    titoletto(a, X + 6, 66, "Disco della punta (4:1)")
    # --- tappo (3:1)
    s, X, Y = 3.0, 142, 122
    cod_d, cod_l, sp_d, sp_s, mo_d, mo_l, asta = 20.0, 15.0, 22.0, 3.0, 16.0, 20.0, 8.0
    L = cod_l + sp_s + mo_l
    prof = [(0, asta / 2), (0, cod_d / 2 - 0.5), (0.5, cod_d / 2), (cod_l, cod_d / 2), (cod_l, sp_d / 2),
            (cod_l + sp_s, sp_d / 2), (cod_l + sp_s, mo_d / 2), (L, mo_d / 2), (L, asta / 2)]
    for k in (1, -1):
        poli(a, [(X + x * s, Y + k * y * s) for x, y in prof], POM, "\\\\\\\\")
        # fori M3 radiali (dal codolo al foro centrale) e foro della spina nel mozzo
        for xc, d, y0, y1 in ((7.5, 3.0, asta / 2, cod_d / 2), (L - 6, 3.0, asta / 2, mo_d / 2)):
            yb, yt = sorted((Y + k * y0 * s, Y + k * y1 * s))
            a.add_patch(Rectangle((X + (xc - d / 2) * s, yb), d * s, yt - yb, fc="white", ec="black", lw=0.6))
    asse_orizz(a, X - 8, X + L * s + 8, Y)
    hquota(a, X, X + L * s, Y - sp_d / 2 * s - 12, f"{L:.0f}", y_rif=Y - sp_d / 2 * s, sopra=False)
    hquota(a, X, X + cod_l * s, Y + sp_d / 2 * s + 9, "15", y_rif=Y + cod_d / 2 * s)
    hquota(a, X + cod_l * s, X + (cod_l + sp_s) * s, Y + sp_d / 2 * s + 17, "3", y_rif=Y + sp_d / 2 * s)
    hquota(a, X, X + 7.5 * s, Y - sp_d / 2 * s - 24, "7,5", y_rif=Y - cod_d / 2 * s, sopra=False)
    hquota(a, X + (L - 6) * s, X + L * s, Y - sp_d / 2 * s - 24, "6", y_rif=Y - mo_d / 2 * s, sopra=False)
    vquota(a, Y - cod_d / 2 * s, Y + cod_d / 2 * s, X + 3 * s, "Ø 20 f7", fs=7.5)
    vquota(a, Y - sp_d / 2 * s, Y + sp_d / 2 * s, X + (cod_l + 1.5) * s, "Ø 22", fs=7.5)
    vquota(a, Y - asta / 2 * s, Y + asta / 2 * s, X + (cod_l + sp_s + 7) * s, "Ø 8 H8 passante", fs=7)
    vquota(a, Y - mo_d / 2 * s, Y + mo_d / 2 * s, X + L * s + 12, "Ø 16", x_rif=X + L * s, sinistra=False, fs=7.5)
    richiamo(a, "2 fori M3 radiali a 180°", (X + 7.5 * s, Y + cod_d / 2 * s), (X - 4, 188), ha="center")
    richiamo(a, "foro Ø 3 trasversale (spina dell'asta)", (X + (L - 6) * s, Y + mo_d / 2 * s), (X + 92, 188))
    titoletto(a, X + L * s / 2, 66, "Tappo isolante pistone ↔ asta (3:1)")
    tv.chiudi("4-disco-tappo")


def tav_collari(pdf):
    tv = Tavola(pdf, 5, "Collari di collegamento", "ottone CuZn39Pb3", "1 per tipo (A e B)", "2:1", [
        "Collare A sul pistone (per la treccia), collare B sul tubo esterno (per la piattina). Foro = Ø esterno del tubo "
        "misurato +0,1/+0,2. Larghezza 12.",
        "Taglio radiale da 1,5. Vite di serraggio M4 tangenziale attraverso il taglio: foro passante Ø 4,5 da un lato, "
        "filetto M4 dall'altro.",
        "Spianatura opposta al taglio, con foro filettato M4 profondo 6 per il capocorda. Spigoli raccordati R 0,5."])
    a = tv.ax
    s = 2.0
    for nome, di, de, cx, xl in (("A (pistone)", 22.0, 34.0, 62, 112), ("B (tubo esterno)", 28.0, 40.0, 190, 244)):
        cy = 120
        Ri, Re = di / 2 * s, de / 2 * s
        a.add_patch(Circle((cx, cy), Re, fc=OTTONE, ec="black", lw=0.9))
        a.add_patch(Circle((cx, cy), Ri, fc="white", ec="black", lw=0.9))
        a.add_patch(Rectangle((cx - 0.75 * s, cy + Ri - 0.3), 1.5 * s, Re - Ri + 0.6, fc="white", ec="none"))
        for dx in (-0.75 * s, 0.75 * s):
            a.plot([cx + dx, cx + dx], [cy + Ri, cy + math.sqrt(Re ** 2 - dx ** 2)], color="black", lw=0.7)
        yv = cy + (Ri + Re) / 2
        a.plot([cx - 10 * s, cx + 10 * s], [yv, yv], color="#333333", lw=0.6, ls=(0, (4, 2)))
        fl = Re - 2 * s
        semi = math.sqrt(Re ** 2 - fl ** 2)
        a.add_patch(Rectangle((cx - semi, cy - Re - 0.5), 2 * semi, Re - fl + 0.5, fc="white", ec="none"))
        a.plot([cx - semi, cx + semi], [cy - fl, cy - fl], color="black", lw=0.8)
        for dx in (-2 * s, 2 * s):
            a.plot([cx + dx, cx + dx], [cy - fl, cy - fl + 6 * s], color="#333333", lw=0.5, ls=(0, (3, 2)))
        asse_orizz(a, cx - Re - 6, cx + Re + 6, cy)
        a.plot([cx, cx], [cy - Re - 6, cy + Re + 6], color="#555555", lw=0.55, ls=(0, (14, 3, 2, 3)))
        hquota(a, cx - Re, cx + Re, cy - Re - 12, f"Ø {de:.0f}", y_rif=cy, sopra=False)
        hquota(a, cx - Ri, cx + Ri, cy + 4, f"Ø {di:.0f} +0,1/+0,2", fs=7.2)
        richiamo(a, "taglio 1,5", (cx, cy + Re - 1), (cx + 12, cy + Re + 14))
        richiamo(a, "vite M4 tangenziale", (cx - 8 * s, yv), (cx - 12, cy + Re + 14), ha="right")
        richiamo(a, "spianatura, foro M4 prof. 6", (cx + 3 * s, cy - fl), (cx, cy - Re - 23), ha="center")
        # vista laterale
        a.add_patch(Rectangle((xl, cy - Re), 12 * s, 2 * Re, fc=OTTONE, ec="black", lw=0.8, hatch=""))
        a.plot([xl, xl + 12 * s], [cy + Ri, cy + Ri], color="#666666", lw=0.5, ls=(0, (3, 2)))
        a.plot([xl, xl + 12 * s], [cy - Ri, cy - Ri], color="#666666", lw=0.5, ls=(0, (3, 2)))
        hquota(a, xl, xl + 12 * s, cy + Re + 6, "12", y_rif=cy + Re)
        titoletto(a, cx + 20, 47, f"Collare {nome}")
    tv.chiudi("5-collari")


def main():
    with PdfPages(os.path.join(QUI, "tavole-condensatore-tx.pdf")) as pdf:
        tav_distinta(pdf)
        tav_manicotto(pdf)
        tav_pistone(pdf)
        tav_tubo(pdf)
        tav_disco_tappo(pdf)
        tav_collari(pdf)
    print("tavole-condensatore-tx.pdf")


if __name__ == "__main__":
    main()
