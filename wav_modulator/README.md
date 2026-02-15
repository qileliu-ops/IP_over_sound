# wav_modulator — 比特流 → 调制 → WAV

将比特流经 FSK 调制后输出为 WAV 文件，便于与队友的「接收 WAV 再解调」流程对接。  
复用项目根目录的 `include/common.h`、`include/modem.h` 与 `src/modem.c`。

---

# Utilisation détaillée (français)

## Présentation

Le module **wav_modulator** transforme un **flux de bits** en fichier **WAV** par modulation FSK (0 → 1200 Hz, 1 → 2400 Hz). Il sert à produire un fichier audio à partir des bits obtenus par **tun_to_bits** (ou d’un fichier binaire de bits), pour le passer ensuite au partenaire qui fait la réception / démodulation.  
Il réutilise `common.h`, `modem.h` et `src/modem.c` du projet.

## Compilation

```bash
cd wav_modulator
make
```

L’exécutable généré s’appelle **bits_to_wav**.

## Utilisation

**Mode test (sans fichier d’entrée) :**

```bash
./bits_to_wav --test
```
- On peut changer les bits dans la fonction de test
- Utilise un flux de bits intégré (ou l’option courte `-t`).
- Génère **output/test.wav**.
- **Vérification du succès :** le programme affiche par exemple **`Test OK: N bits -> M samples -> output/test.wav`** et le fichier **output/test.wav** existe.

**À partir d’un fichier de bits (ex. sortie de tun_to_bits) :**

```bash
./bits_to_wav <fichier_bits.bin> <fichier_sortie.wav>
```

- **Entrée :** fichier binaire de bits (chaque octet = 8 bits, **bit de poids fort en premier**, comme le modem).
- **Sortie :** chemin du WAV (ex. `output/from_tun.wav`).

**Exemple avec le résultat de tun_to_bits :**

```bash
./bits_to_wav ../tun_to_bits/output/bits.bin output/from_tun.wav
```

**Vérification du succès :** affichage **`OK: N bits -> M samples -> output/...wav`** et présence du fichier WAV. Lecture possible avec :

```bash
aplay output/test.wav
# ou avec le lecteur audio du système
```

## Format de sortie

- **WAV :** PCM 16 bits, mono, 44100 Hz (comme `SAMPLE_RATE` dans `common.h`).
- **Modulation :** 0 → 1200 Hz, 1 → 2400 Hz, 1200 bps (identique au projet principal).

---

## 编译

```bash
cd wav_modulator
make
```

生成可执行文件 `bits_to_wav`。

## 用法

**内置测试（生成 `output/test.wav`）：**

```bash
./bits_to_wav --test
```

**从比特流文件生成 WAV：**

```bash
./bits_to_wav <input.bin> <output.wav>
```

- `input.bin`：原始字节文件，每字节 8 比特，**高位先发**（与 modem 约定一致）。
- `output.wav`：输出路径，可写为 `output/xxx.wav` 将结果放在本目录的 `output/` 下。

## 测试结果存放

生成的 WAV 建议放在本目录下的 **`output/`** 中，例如：

```bash
./bits_to_wav --test                    # 生成 output/test.wav
./bits_to_wav my_bits.bin output/out.wav
```

播放测试：

```bash
aplay output/test.wav
# 或使用系统播放器打开 output/test.wav
```

## 输出格式

- WAV：16-bit PCM，单声道，44100 Hz（与 `common.h` 中 `SAMPLE_RATE` 一致）。
- 调制参数：与主项目相同（0 → 1200 Hz，1 → 2400 Hz，1200 bps）。
