"""Anteprime PNG del contenitore: piccolo rasterizzatore con z-buffer in numpy (nessun OpenGL richiesto)."""
import math
import os

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

COLORI = {"scatola": "#dfe3e8", "piastra": "#f6ae2d", "coperchio": "#86b6f6", "guarnizione": "#d93025",
          "staffa": "#7cb342", "palo": "#8d8d8d"}


def triangoli(oggetto, spost=(0, 0, 0), tol=0.2):
    v, t = oggetto.val().tessellate(tol, 0.3)
    if not t:  # pezzo vuoto (per esempio tutto dall'altra parte della sezione)
        return np.zeros((0, 3, 3))
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


def taglia(oggetto, y):
    """Tiene la metà con Y < y (sezione sull'asse del motore)."""
    import cadquery as cq
    return oggetto.intersect(cq.Workplane("XY").box(2000, 1000, 2000).translate((0, y - 500, 0)))


def installato(tri):
    """Dalla posizione di stampa a quella sul palo verticale: parete frontale (X = 0) in alto."""
    return np.stack([tri[..., 2], tri[..., 1], -tri[..., 0]], axis=-1)


def disegna(qui, parti, pezzi):
    cart = os.path.join(qui, "anteprime")
    os.makedirs(cart, exist_ok=True)
    finti_senza_oled = [(triangoli(o), c) for n, (o, c) in pezzi.items() if n != "OLED"]
    oled = (triangoli(pezzi["OLED"][0]), pezzi["OLED"][1])
    t = {n: triangoli(o) for n, o in parti.items()}
    c = COLORI

    interno = [(t["scatola"], c["scatola"]), (t["piastra"], c["piastra"]), (t["staffa"], c["staffa"])] + finti_senza_oled
    figura(interno, os.path.join(cart, "montaggio.png"), 42, -135,
           "Interno: riduttore, squadra e SERVO42D al centro; strisce di millefori ai lati")
    figura(interno, os.path.join(cart, "montaggio-retro.png"), 40, 45,
           "Interno dal retro: PG7 dei 24 V e valvola di sfiato")
    figura(interno + [(triangoli(parti["guarnizione"], (0, 0, 40)), c["guarnizione"]),
                      (triangoli(parti["coperchio"], (0, 0, 80)), c["coperchio"]),
                      (triangoli(pezzi["OLED"][0], (0, 0, 80)), pezzi["OLED"][1])],
           os.path.join(cart, "esploso.png"), 24, -135, "Esploso: staffa, scatola, guarnizione, coperchio con l'OLED")
    chiuso = [(t["scatola"], c["scatola"]), (t["guarnizione"], c["guarnizione"]), (t["coperchio"], c["coperchio"]),
              (t["staffa"], c["staffa"]), oled] + finti_senza_oled
    figura([(installato(a), b) for a, b in chiuso + [(t["palo"], c["palo"])]],
           os.path.join(cart, "sul-palo.png"), 12, -35,
           "Sul palo: albero in alto verso il variabile, 24 V e sfiato in basso")
    figura([(installato(a), b) for a, b in chiuso + [(t["palo"], c["palo"])]],
           os.path.join(cart, "sul-palo-retro.png"), 20, 150, "Staffa a V con le fascette stringitubo")
    ya = pezzi["albero"][0].val().BoundingBox().center.y
    sezione = [(triangoli(taglia(parti[n], ya)), c[n]) for n in ("scatola", "piastra", "staffa", "coperchio",
                                                                  "guarnizione")]
    sezione += [(triangoli(taglia(o, ya)), col) for n, (o, col) in pezzi.items()]
    figura(sezione, os.path.join(cart, "sezione.png"), 8, 90,
           "Sezione sull'asse: riduttore, squadra e motore sul rialzo di 20 mm; giunto e tubo nel paraolio")
    figura([(t["staffa"], c["staffa"])], os.path.join(cart, "staffa-palo.png"), -40, -60,
           "Staffa: due blocchi a V, passaggi per le fascette, 4 viti M4 nel fondo")
    figura([(t["coperchio"], c["coperchio"]), oled],
           os.path.join(cart, "coperchio-interno.png"), -55, -100, "Coperchio da dentro: OLED sulle colonnine")
