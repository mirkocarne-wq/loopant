"""Anteprime PNG del contenitore: piccolo rasterizzatore con z-buffer in numpy (nessun OpenGL richiesto)."""
import math
import os

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

COLORI = {"scatola": "#dfe3e8", "piastra": "#f6ae2d", "coperchio": "#86b6f6", "guarnizione": "#d93025"}


def triangoli(oggetto, spost=(0, 0, 0), tol=0.2):
    v, t = oggetto.val().tessellate(tol, 0.3)
    v = np.array([(p.x, p.y, p.z) for p in v]) + np.array(spost)
    return v[np.array(t)]


def rasterizza(gruppi, elev, azim, larghezza=1600, altezza=1150, margine=40):
    # Vista ortografica: rotazione attorno a Z (azimut) e poi inclinazione (elevazione)
    a, e = math.radians(azim), math.radians(elev)
    vista = np.array([math.cos(e) * math.cos(a), math.cos(e) * math.sin(a), math.sin(e)])  # verso l'osservatore
    destra = np.array([-math.sin(a), math.cos(a), 0.0])
    su = np.cross(vista, destra)
    luce = 0.55 * vista + 0.45 * np.array([0.2, -0.5, 0.85])
    luce /= np.linalg.norm(luce)

    tutti = np.vstack([g[0].reshape(-1, 3) for g in gruppi])
    px, py = tutti @ destra, tutti @ su
    scala = min((larghezza - 2 * margine) / (px.max() - px.min()), (altezza - 2 * margine) / (py.max() - py.min()))
    ox = larghezza / 2 - scala * (px.max() + px.min()) / 2
    oy = altezza / 2 + scala * (py.max() + py.min()) / 2

    zbuf = np.full((altezza, larghezza), -np.inf)
    img = np.ones((altezza, larghezza, 3))
    for tri, colore in gruppi:
        base = np.array(matplotlib.colors.to_rgb(colore))
        n = np.cross(tri[:, 1] - tri[:, 0], tri[:, 2] - tri[:, 0])
        n /= np.linalg.norm(n, axis=1, keepdims=True) + 1e-12
        n[(n @ vista) < 0] *= -1  # faccia verso l'osservatore
        intens = 0.35 + 0.65 * np.clip(n @ luce, 0, 1)
        X = ox + scala * (tri @ destra)
        Y = oy - scala * (tri @ su)
        Z = tri @ vista
        for k in range(len(tri)):
            x, y, z = X[k], Y[k], Z[k]
            x0, x1 = max(int(x.min()), 0), min(int(x.max()) + 1, larghezza - 1)
            y0, y1 = max(int(y.min()), 0), min(int(y.max()) + 1, altezza - 1)
            if x1 < x0 or y1 < y0:
                continue
            gx, gy = np.meshgrid(np.arange(x0, x1 + 1) + 0.5, np.arange(y0, y1 + 1) + 0.5)
            d = (y[1] - y[2]) * (x[0] - x[2]) + (x[2] - x[1]) * (y[0] - y[2])
            if abs(d) < 1e-9:
                continue
            w0 = ((y[1] - y[2]) * (gx - x[2]) + (x[2] - x[1]) * (gy - y[2])) / d
            w1 = ((y[2] - y[0]) * (gx - x[2]) + (x[0] - x[2]) * (gy - y[2])) / d
            w2 = 1 - w0 - w1
            m = (w0 >= -1e-6) & (w1 >= -1e-6) & (w2 >= -1e-6)
            if not m.any():
                continue
            zz = w0 * z[0] + w1 * z[1] + w2 * z[2]
            zb = zbuf[y0:y1 + 1, x0:x1 + 1]
            m &= zz > zb
            zb[m] = zz[m]
            img[y0:y1 + 1, x0:x1 + 1][m] = base * intens[k]
    # Contorni: salti di profondità
    vuoto = ~np.isfinite(zbuf)
    zf = np.where(vuoto, zbuf[~vuoto].min() - 50 if (~vuoto).any() else 0, zbuf)
    salto = np.zeros_like(zf, dtype=bool)
    soglia = 1.5
    salto[:, 1:] |= np.abs(np.diff(zf, axis=1)) > soglia
    salto[1:, :] |= np.abs(np.diff(zf, axis=0)) > soglia
    img[salto] *= 0.35
    return img


def figura(gruppi, file, elev, azim, titolo):
    img = rasterizza(gruppi, elev, azim)
    fig = plt.figure(figsize=(img.shape[1] / 150, img.shape[0] / 150 + 0.4), dpi=150)
    ax = fig.add_axes([0, 0, 1, img.shape[0] / (img.shape[0] + 60)])
    ax.imshow(img, interpolation="antialiased")
    ax.set_axis_off()
    fig.text(0.5, 0.985, titolo, ha="center", va="top", fontsize=13)
    fig.savefig(file, facecolor="white")
    plt.close(fig)
    print("anteprima:", os.path.basename(file))


def disegna(qui, parti, pezzi, z_coperchio, spessore_coperchio):
    cart = os.path.join(qui, "anteprime")
    os.makedirs(cart, exist_ok=True)
    finti = [(triangoli(o), c) for o, c in pezzi.values()]
    finti_senza_oled = [(triangoli(o), c) for n, (o, c) in pezzi.items() if n != "OLED"]
    sc = triangoli(parti["scatola"])
    pi = triangoli(parti["piastra"])
    co = triangoli(parti["coperchio"])
    gu = triangoli(parti["guarnizione"])

    interno = [(sc, COLORI["scatola"]), (pi, COLORI["piastra"])] + finti_senza_oled
    figura(interno, os.path.join(cart, "montaggio.png"), 42, -135,
           "Interno: SERVO42D con riduttore sulla squadra, millefori, piastra")
    figura(interno, os.path.join(cart, "montaggio-retro.png"), 40, 45,
           "Interno dal retro: PG7 per i 24 V e valvola di sfiato")
    figura(interno + [(triangoli(parti["guarnizione"], (0, 0, 40)), COLORI["guarnizione"]),
                      (triangoli(parti["coperchio"], (0, 0, 80)), COLORI["coperchio"]),
                      (triangoli(pezzi["OLED"][0], (0, 0, 80)), pezzi["OLED"][1])],
           os.path.join(cart, "esploso.png"), 24, -135, "Esploso: scatola, guarnizione, coperchio con l'OLED")
    figura([(sc, COLORI["scatola"]), (gu, COLORI["guarnizione"]), (co, COLORI["coperchio"])] + finti,
           os.path.join(cart, "chiuso.png"), 30, -140, "Chiuso: finestra dell'OLED, passaggio dell'albero, PG7")
    figura([(co, COLORI["coperchio"]), (triangoli(pezzi["OLED"][0]), pezzi["OLED"][1])],
           os.path.join(cart, "coperchio-interno.png"), -55, -100, "Coperchio da dentro: OLED sulle colonnine")
