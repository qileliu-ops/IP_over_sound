# tun_to_bits — TUN 读包 → 封装成帧 → 帧转比特

与 `wav_modulator` 平行，实现发送链前半段。每步单独一个文件（含头文件），主程序用**同一可执行文件 + 不同参数**支持逐步独立运行与验证。

---

# Utilisation détaillée (français)

## Présentation

Le programme **tun_to_bits** permet d’exécuter, une par une, les quatre étapes suivantes : **créer le TUN** → **lire un paquet depuis le TUN** → **encapsuler le paquet en trame** → **convertir la trame en flux de bits**. Un seul exécutable est utilisé, avec des arguments différents pour chaque étape. Les données passent d’une étape à l’autre via des fichiers dans le dossier **output/**.

## Compilation

```bash
cd tun_to_bits
make
```

L’exécutable généré s’appelle **tun_to_bits**.

## Exécution pas à pas (ordre réel) et vérification

### Étape 1 : Créer le TUN

**Commande :**

```bash
sudo ./tun_to_bits --create tun0
```

**Vérification du succès :**

- Le programme affiche : **`TUN created successfully: tun0 (fd=<num>)`**
- Le programme se termine sans message d’erreur.
- Optionnel : `ip link show tun0` affiche l’interface tun0.

**En cas d’échec :** message **`TUN created failed`** — exécuter avec **sudo**.

---

### Étape 2 : Lire un paquet depuis le TUN

**Prérequis (une seule fois) :** donner une adresse à tun0 et l’activer :

```bash
sudo ip addr add 10.0.0.1/24 dev tun0
sudo ip link set tun0 up
```

(Si l’adresse est déjà assignée, la première commande peut afficher « Address already assigned » : on peut l’ignorer et ne lancer que la seconde si besoin.)

**Commande :**

```bash
sudo ./tun_to_bits --read tun0
```

Le programme **attend** qu’un paquet soit envoyé vers le TUN. Dans **un autre terminal**, envoyer un paquet vers le sous-réseau 10.0.0.0/24, par exemple :

```bash
ping -c 1 10.0.0.2
```

**Vérification du succès :**

- Le terminal qui exécute `--read` affiche : **`Read packet successfully: N bytes -> output/ip.bin`** (N > 0).
- Le fichier **output/ip.bin** existe et fait environ N octets (`ls -la output/ip.bin`).

**En cas d’échec :** le programme reste bloqué (aucun paquet n’arrive sur le TUN) ou affiche **`Read packet failed or no data`**. Vérifier que l’autre terminal envoie bien du trafic vers 10.0.0.x et que la route passe par tun0.

---

### Étape 3 : Encapsuler le paquet en trame (IP → trame)

**Commande :**

```bash
./tun_to_bits --encapsulate
```

(Pas besoin de root. Le fichier **output/ip.bin** doit exister, produit par l’étape 2.)

**Vérification du succès :**

- Affichage : **`Encapsulate successfully: IP N bytes -> Frame M bytes -> output/frame.bin`** (N et M positifs, avec M = 4 + N + 2).
- Le fichier **output/frame.bin** existe et fait M octets.

**En cas d’échec :** message **`Cannot read output/ip.bin (run --read first)`** ou **`encapsulate failed`**. Exécuter d’abord l’étape 2 avec succès.

---

### Étape 4 : Convertir la trame en bits

**Commande :**

```bash
./tun_to_bits --to-bits
```

(Pas besoin de root. Le fichier **output/frame.bin** doit exister, produit par l’étape 3.)

**Vérification du succès :**

- Affichage : **`Convert frame to bits successfully: Frame M bytes -> K bits -> output/bits.bin`** (K = M × 8).
- Le fichier **output/bits.bin** existe et fait (K+7)/8 octets.

**En cas d’échec :** message **`Cannot read output/frame.bin (run --encapsulate first)`**. Exécuter d’abord l’étape 3 avec succès.

---

### Enchaînement des étapes et résumé

| Ordre | Commande | Succès si |
|-------|----------|------------|
| 1 | `sudo ./tun_to_bits --create tun0` | Affichage **`TUN created successfully`**, programme se termine normalement. |
| 2 | (Dans l’autre terminal : `ping -c 1 10.0.0.2`) puis `sudo ./tun_to_bits --read tun0` | Affichage **`Read packet successfully: N bytes -> output/ip.bin`**, fichier **output/ip.bin** présent. |
| 3 | `./tun_to_bits --encapsulate` | Affichage **`Encapsulate successfully`**, fichier **output/frame.bin** présent. |
| 4 | `./tun_to_bits --to-bits` | Affichage **`Convert frame to bits successfully`**, fichier **output/bits.bin** présent. |

Une fois **output/bits.bin** obtenu, on peut l’utiliser avec le module de modulation (wav_modulator) pour produire un fichier WAV :

```bash
cd ../wav_modulator
./bits_to_wav ../tun_to_bits/output/bits.bin output/from_tun.wav
```


Chinese blow

---

| 文件 | 作用 |
|------|------|
| **tun_create.c/h** | 创建 TUN 设备（封装 tun_open） |
| **packet_read.c/h** | 从 TUN 读一个 IP 包到缓冲区（封装 tun_read） |
| **encapsulate.c/h** | 把 IP 包封装成帧（封装 protocol_encapsulate） |
| **frame_to_bits.c/h** | 将帧转为比特流 |
| **tun_to_bits.c** | 主程序：根据参数执行单步或全流程 |

底层仍复用项目中的 `tun_dev`、`protocol`、`utils`。

## 编译

```bash
cd tun_to_bits
make
```

生成可执行文件 `tun_to_bits`。

## 用法（同一可执行文件，不同参数）

**逐步运行，每步独立验证：**

| 参数 | 作用 | 输入 | 输出 | 验证 |
|------|------|------|------|------|
| `--create [tun_name]` | 仅创建 TUN | - | - | 打印 “TUN 创建成功” |
| `--read [tun_name]` | 仅从 TUN 读一包 | - | output/ip.bin | 打印 “读包成功: N 字节” |
| `--encapsulate` | 仅封装成帧 | output/ip.bin | output/frame.bin | 打印 “封装成功” |
| `--to-bits` | 仅帧转比特 | output/frame.bin | output/bits.bin | 打印 “转比特成功” |

**推荐顺序（先测 3、4 再测 1、2）：**

```bash
# 1) 不依赖 TUN：用内置 IP 测封装 + 转比特（会生成 output/frame.bin、output/bits.bin）
./tun_to_bits --test

# 2) 仅创建 TUN（需 root）
sudo ./tun_to_bits --create tun0

# 3) 仅读包（需 root，且需有流量发到 TUN，如另一终端 ping 10.0.0.2）
sudo ./tun_to_bits --read tun0

# 4) 用上一步的 output/ip.bin 封装成帧
./tun_to_bits --encapsulate

# 5) 用 output/frame.bin 转比特
./tun_to_bits --to-bits
```

**全流程（一次跑完，需 root）：**

```bash
sudo ./tun_to_bits [tun_name]
```

- 创建 TUN → 读一包 → 封装 → 转比特，并打印帧与比特 hex。

**测试模式（不需 root）：**

```bash
./tun_to_bits --test
```

- 使用内置或 **testdata/ip_packet.bin** 的 IP 包，封装、转比特，打印并写入 output/frame.bin、output/bits.bin。

## 数据流（逐步时）

```
--create     → 无文件
--read       → output/ip.bin
--encapsulate → 读 ip.bin  → output/frame.bin
--to-bits    → 读 frame.bin → output/bits.bin
```

可将 **output/bits.bin** 交给 wav_modulator：

```bash
../wav_modulator/bits_to_wav output/bits.bin output/out.wav
```
