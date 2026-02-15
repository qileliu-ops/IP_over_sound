# IP over Sound

通过声波在两台电脑之间传输 IP 数据包：本机 TUN 虚拟网卡上的 IP 包经 FSK 调制后由扬声器播放，对方麦克风录制后解调、成帧、校验并写回 TUN，实现“声波上的网络”。

## 架构概览

- **TUN**：与内核交换 IP 包（读/写虚拟网卡）
- **protocol**：帧封装/解封装（同步字 + 长度 + CRC）
- **modem**：FSK 调制/解调（比特 ↔ 波形）
- **audio**：PortAudio 声卡读/写

## 依赖

- Linux（TUN 与 `linux/if_tun.h`）
- PortAudio：`libportaudio-dev`（Ubuntu/Debian）
- 编译：`gcc`，链接 `-lpthread -lm -lportaudio`

## 编译

```bash
make
```

生成可执行文件 `ipo_sound`。

## 运行

1. **创建并配置 TUN**（需 root）  
   先运行一次程序让 TUN 被创建，或手动创建：
   ```bash
   sudo ip tuntap add dev tun0 mode tun
   ```
   然后配置 IP 与路由：
   ```bash
   sudo ./scripts/setup_tun.sh tun0 10.0.0.1
   ```

2. **启动程序**
   ```bash
   sudo ./ipo_sound [tun_name]
   ```
   默认使用 `tun0`，可选参数指定 TUN 设备名。

3. **对端**  
   另一台机器同样配置 TUN（如 `10.0.0.2/24`），运行 `ipo_sound`，两台机即可通过 10.0.0.0/24 网段经声波互通（需扬声器与麦克风相对、音量合适）。

## 注意事项

- 需要 **root** 或 `CAP_NET_ADMIN` 才能操作 TUN。
- 当前为**半双工**演示：收发可同时进行，但同一时刻同一信道，噪声大时可能丢包。
- 采样率、FSK 频率、帧格式等见 `include/common.h`。

---

# Présentation en français

## Logique d’implémentation globale

**IP over Sound** permet de transmettre des paquets IP entre deux ordinateurs via des ondes sonores. Le flux de données est le suivant :

- **Émission (TX)** : le noyau envoie un paquet IP vers l’interface TUN → le programme lit ce paquet depuis TUN → il l’encapsule dans une **trame** (en-tête de synchronisation + longueur + charge utile + CRC) → la trame est convertie en **flux de bits** → modulation **FSK** (0 → 1200 Hz, 1 → 2400 Hz) en échantillons audio → les échantillons sont envoyés à la carte son (haut-parleur).
- **Réception (RX)** : le microphone enregistre des échantillons audio → démodulation FSK (détection par passages par zéro) → flux de bits → recherche de la **synchronisation** (mot de synchro 0x7E) → extraction d’une trame complète → vérification CRC → extraction du paquet IP (charge utile) → écriture du paquet dans TUN → le noyau reçoit le paquet comme s’il venait d’une interface réseau.

Le programme utilise **deux threads** : un thread TX (émission) et un thread RX (réception), qui tournent en parallèle. L’accès à TUN et à l’audio est partagé via des descripteurs/poignées globaux.

**Couches** :
- **Réseau (IP)** : paquets IP échangés avec le noyau via TUN.
- **Liaison (trame)** : format de trame (synchro + longueur + charge + CRC), encapsulation/décapsulation, recherche de synchro dans le flux de bits.
- **Physique** : modulation/démodulation FSK, lecture/écriture audio (PortAudio).

---

## Rôle de chaque fichier

### Répertoire `include/`

| Fichier | Rôle |
|--------|------|
| **common.h** | Constantes globales : fréquence d’échantillonnage (44100 Hz), taille du buffer audio (1024), fréquences FSK (1200 Hz / 2400 Hz), débit (1200 bps), paramètres de trame (SYNC_LEN, SYNC_BYTE, MAX_FRAME_PAYLOAD, CRC_BYTES, etc.). C’est le point central pour adapter le projet. |
| **tun_dev.h** | Interface du module TUN : `tun_open`, `tun_read`, `tun_write`, `tun_close`. Déclare les fonctions d’échange de paquets IP avec le noyau. |
| **protocol.h** | Interface du module trame (couche liaison) : `protocol_encapsulate` (IP → trame), `protocol_decapsulate` (trame → IP, avec vérification CRC), `protocol_find_sync` (recherche du mot de synchro dans le flux de bits). |
| **modem.h** | Interface du modem FSK : création/destruction des poignées TX/RX, `modem_tx_modulate` (bits → échantillons), `modem_rx_demodulate` (échantillons → bits). |
| **audio_dev.h** | Interface audio (PortAudio) : `audio_init`, `audio_write`, `audio_read`, `audio_cleanup`. Lecture micro, écriture haut-parleur. |
| **utils.h** | Fonctions utilitaires : `crc16` (CRC-16 CCITT pour la trame), `debug_hex_dump` (affichage hexadécimal pour débogage). |

### Répertoire `src/`

| Fichier | Rôle |
|--------|------|
| **main.c** | Point d’entrée : ouverture TUN, initialisation audio, création des threads TX et RX, boucle principale jusqu’à Ctrl+C, puis nettoyage. Contient aussi les fonctions d’enchaînement : `frame_to_bits`, `bits_to_bytes`, `bits_append`, `bits_remove`, et les corps des threads `tx_thread_func` et `rx_thread_func`. |
| **tun_dev.c** | Implémentation TUN : `open("/dev/net/tun")`, `ioctl(TUNSETIFF)` pour créer/attacher une interface TUN (ex. tun0), puis `read`/`write` sur le descripteur pour recevoir/envoyer des paquets IP bruts. Nécessite root ou CAP_NET_ADMIN. |
| **protocol.c** | Implémentation du protocole de trame : encapsulation (synchro + longueur big-endian + charge + CRC), décapsulation (lecture longueur, vérification CRC, extraction charge), recherche de synchro dans un buffer de bits (alignement bit à bit). |
| **modem.c** | Implémentation FSK : côté TX, génération de sinusoïdes (1200 Hz / 2400 Hz) avec phase continue ; côté RX, démodulation par comptage des passages par zéro pour distinguer 0 (basse fréquence) et 1 (haute fréquence). |
| **audio_dev.c** | Implémentation PortAudio : ouverture des flux entrée (micro) et sortie (haut-parleur) par défaut, `Pa_ReadStream` / `Pa_WriteStream` avec le format float et la fréquence/taille de buffer définies dans common.h. |
| **utils.c** | Implémentation CRC-16 (CCITT) et affichage hexadécimal pour le débogage. |

### Autres fichiers

| Fichier | Rôle |
|--------|------|
| **Makefile** | Règles de compilation : compilation des .c en .o, liaison avec `-lpthread -lm -lportaudio`, production de l’exécutable `ipo_sound`. |
| **scripts/setup_tun.sh** | Script pour créer/configurer l’interface TUN (ex. tun0) et lui attribuer une adresse IP (ex. 10.0.0.1/24). À lancer en root. |

---

## Déroulement du programme (flux d’exécution)

1. **Démarrage (`main`)**  
   - Gestion du signal SIGINT (Ctrl+C) pour mettre fin proprement à l’exécution.  
   - Ouverture de l’interface TUN (`tun_open`, ex. tun0) ; en cas d’échec, message invitant à lancer en `sudo`.  
   - Initialisation de l’audio (`audio_init`) : PortAudio, flux micro et haut-parleur, démarrage des flux.  
   - Création de deux threads : `tx_thread_func` et `rx_thread_func`, auxquels on passe le descripteur TUN (et l’audio via une variable globale).  
   - La boucle principale se contente d’attendre (p.ex. `sleep(1)`) tant que `g_running` est vrai.

2. **Thread TX (émission)**  
   - Boucle tant que `g_running` et que l’audio est disponible.  
   - **tun_read** : lecture bloquante d’un paquet IP depuis TUN (aucun son n’est émis tant qu’aucun paquet n’est envoyé vers TUN).  
   - **protocol_encapsulate** : construction de la trame (synchro + longueur + paquet IP + CRC).  
   - **frame_to_bits** : conversion de la trame (octets) en flux de bits (convention : 8 bits par octet, bit de poids fort en premier).  
   - **modem_tx_modulate** : modulation FSK des bits en échantillons audio (sinusoïdes 1200 Hz / 2400 Hz).  
   - **audio_write** : envoi des échantillons vers le haut-parleur par blocs (ex. AUDIO_FRAMES_PER_BUFFER).  
   → C’est à ce moment que le son est émis.

3. **Thread RX (réception)**  
   - Boucle tant que `g_running` et que l’audio est disponible.  
   - **audio_read** : lecture d’un bloc d’échantillons depuis le microphone.  
   - **modem_rx_demodulate** : démodulation FSK → flux de bits.  
   - Les bits sont **ajoutés** à un buffer cumulatif (`bits_append`) ; si le buffer est plein, une partie est supprimée pour faire de la place (`bits_remove`).  
   - **protocol_find_sync** : recherche de la position de début de trame (mot de synchro dans le flux de bits).  
   - Vérification que la longueur de trame (lue dans l’en-tête) est valide et que suffisamment de bits sont disponibles.  
   - **bits_to_bytes** : conversion des bits de la trame en octets.  
   - **protocol_decapsulate** : vérification CRC et extraction du paquet IP.  
   - **tun_write** : injection du paquet IP dans TUN ; le noyau le traite comme un paquet reçu sur l’interface.

4. **Arrêt**  
   - L’utilisateur envoie Ctrl+C → `signal_handler` met `g_running` à 0.  
   - La boucle de `main` se termine, `main` met aussi `g_running` à 0 (redondant), puis appelle `pthread_join` sur les deux threads pour les laisser quitter proprement.  
   - **audio_cleanup** : arrêt et fermeture des flux audio, `Pa_Terminate`.  
   - **tun_close** : fermeture du descripteur TUN.  
   - Fin du programme.

En résumé : le **son n’est émis que lorsqu’il y a du trafic IP à envoyer** (paquet lu depuis TUN). La réception est continue : le micro enregistre en permanence, et dès qu’une trame valide est détectée et vérifiée, le paquet IP est renvoyé au noyau via TUN.

---

# 中文详细说明

## 整体实现逻辑

**IP over Sound** 在两台电脑之间通过声波传输 IP 数据包，数据流大致如下：

- **发送（TX）**：内核把 IP 包发往 TUN 接口 → 程序从 TUN 读出该包 → 封装成**帧**（同步字 + 长度 + 载荷 + CRC）→ 帧转为**比特流** → **FSK 调制**（0→1200 Hz，1→2400 Hz）成音频采样 → 采样送入声卡（扬声器）播放。
- **接收（RX）**：麦克风采集音频采样 → FSK 解调（过零检测）→ 比特流 → 在比特流中**找同步**（同步字 0x7E）→ 取出一帧完整数据 → 校验 CRC → 取出 IP 包（载荷）→ 写入 TUN → 内核把该包当作从“网卡”收到的 IP 包处理。

程序使用**两个线程**：TX 线程（发送）和 RX 线程（接收），并行运行；TUN 与音频通过全局的 fd/句柄共享。

**分层**：
- **网络层（IP）**：通过 TUN 与内核交换 IP 包。
- **链路层（帧）**：帧格式（同步 + 长度 + 载荷 + CRC）、封装/解封装、在比特流中找同步。
- **物理层**：FSK 调制/解调、音频读写（PortAudio）。

---

## 各文件的功能

### 目录 `include/`

| 文件 | 作用 |
|------|------|
| **common.h** | 全局常量：采样率（44100 Hz）、音频缓冲区大小（1024）、FSK 频率（1200 Hz / 2400 Hz）、波特率（1200 bps）、帧参数（SYNC_LEN、SYNC_BYTE、MAX_FRAME_PAYLOAD、CRC_BYTES 等）。修改项目参数时主要改此文件。 |
| **tun_dev.h** | TUN 模块接口：`tun_open`、`tun_read`、`tun_write`、`tun_close`。声明与内核交换 IP 包的函数。 |
| **protocol.h** | 帧/链路层模块接口：`protocol_encapsulate`（IP→帧）、`protocol_decapsulate`（帧→IP，含 CRC 校验）、`protocol_find_sync`（在比特流中找同步字）。 |
| **modem.h** | FSK 调制解调接口：创建/销毁 TX/RX 句柄，`modem_tx_modulate`（比特→采样）、`modem_rx_demodulate`（采样→比特）。 |
| **audio_dev.h** | 音频（PortAudio）接口：`audio_init`、`audio_write`、`audio_read`、`audio_cleanup`。读麦克风、写扬声器。 |
| **utils.h** | 工具函数：`crc16`（帧尾 CRC-16 CCITT）、`debug_hex_dump`（调试用十六进制打印）。 |

### 目录 `src/`

| 文件 | 作用 |
|------|------|
| **main.c** | 程序入口：打开 TUN、初始化音频、创建 TX/RX 线程、主循环直到 Ctrl+C、然后清理。还包含串联用的函数：`frame_to_bits`、`bits_to_bytes`、`bits_append`、`bits_remove`，以及线程函数 `tx_thread_func`、`rx_thread_func`。 |
| **tun_dev.c** | TUN 实现：`open("/dev/net/tun")`、`ioctl(TUNSETIFF)` 创建/绑定 TUN 接口（如 tun0），再对该 fd 做 `read`/`write` 收发原始 IP 包。需要 root 或 CAP_NET_ADMIN。 |
| **protocol.c** | 帧协议实现：封装（同步 + 长度大端 + 载荷 + CRC）、解封装（读长度、校验 CRC、取载荷）、在比特流中找同步（按比特对齐）。 |
| **modem.c** | FSK 实现：发送端用相位连续的正弦（1200 Hz / 2400 Hz）生成采样；接收端用过零次数区分 0（低频）与 1（高频）。 |
| **audio_dev.c** | PortAudio 实现：打开默认输入（麦克风）和输出（扬声器）流，用 common.h 中的采样率和缓冲区大小做 `Pa_ReadStream` / `Pa_WriteStream`（float 格式）。 |
| **utils.c** | CRC-16（CCITT）实现及调试用十六进制输出。 |

### 其他文件

| 文件 | 作用 |
|------|------|
| **Makefile** | 编译规则：.c 编成 .o，链接时加 `-lpthread -lm -lportaudio`，生成可执行文件 `ipo_sound`。 |
| **scripts/setup_tun.sh** | 创建并配置 TUN 接口（如 tun0）、配置 IP 地址（如 10.0.0.1/24）的脚本，需 root 运行。 |

---

## 运行流程

1. **启动（main）**  
   - 注册 SIGINT（Ctrl+C）处理，用于干净退出。  
   - 打开 TUN 接口（`tun_open`，如 tun0）；失败则提示用 sudo 运行。  
   - 初始化音频（`audio_init`）：PortAudio、打开麦克风与扬声器流并启动。  
   - 创建两个线程：`tx_thread_func` 和 `rx_thread_func`，传入 TUN 的 fd（音频通过全局变量传入）。  
   - main 只做 `while (g_running) sleep(1)`，等待退出。

2. **TX 线程（发送）**  
   - 在 `g_running` 且音频可用时循环。  
   - **tun_read**：从 TUN 阻塞读一个 IP 包（没有包时不会出声）。  
   - **protocol_encapsulate**：封装成帧（同步 + 长度 + IP 包 + CRC）。  
   - **frame_to_bits**：把帧（字节）转成比特流（每字节 8 比特，高位在前）。  
   - **modem_tx_modulate**：把比特调制成音频采样（1200 Hz / 2400 Hz 正弦）。  
   - **audio_write**：按块（如 AUDIO_FRAMES_PER_BUFFER）把采样写入扬声器。  
   → 此时才有声音发出。

3. **RX 线程（接收）**  
   - 在 `g_running` 且音频可用时循环。  
   - **audio_read**：从麦克风读一块采样。  
   - **modem_rx_demodulate**：解调得到比特流。  
   - 比特**追加**到累积缓冲区（`bits_append`）；缓冲区满时丢弃前半部分腾出空间（`bits_remove`）。  
   - **protocol_find_sync**：在比特流中找帧起始位置（同步字）。  
   - 检查帧长度（从帧头读出）合法且已有足够比特。  
   - **bits_to_bytes**：把该帧的比特转成字节。  
   - **protocol_decapsulate**：校验 CRC 并取出 IP 包。  
   - **tun_write**：把 IP 包写回 TUN，内核当作从该接口收到的包处理。

4. **退出**  
   - 用户按 Ctrl+C → `signal_handler` 将 `g_running` 置 0。  
   - main 的循环结束，再置一次 `g_running = 0`，然后 `pthread_join` 等待两个线程结束。  
   - **audio_cleanup**：停止并关闭音频流，`Pa_Terminate`。  
   - **tun_close**：关闭 TUN 的 fd。  
   - 程序结束。

小结：**只有在有待发送的 IP 流量（从 TUN 读到包）时才会发出声音**。接收是持续的：麦克风一直在录，一旦在比特流中检测到完整且校验通过的帧，就把其中的 IP 包写回 TUN。
