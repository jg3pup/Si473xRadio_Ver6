# Si473X Radio Ver6

Si473X シリーズ IC を使用したマルチバンドラジオキットです。  
AM / FM / SW / AIR バンドに対応し、Web ブラウザからリモート操作が可能です。

---

## 特徴

- **マルチバンド対応** : MW / FM / SW / AIR（航空無線）
- **Web リモコン** : スマートフォン・PC ブラウザから操作可能
- **PC アプリ** : Windows 対応コントロールアプリ付属
- **Android アプリ** : Android 対応コントロールアプリ付属


---

## 頒布について

- **頒布者** : JG3PUP
- **頒布形態** : 無償頒布（無料）
- **対象** : アマチュア無線・BCL・電子工作を楽しむ方

> 本キットは個人の趣味として制作・頒布するものです。  
> 商業目的での再配布・転売はお断りします。

---

## ファイル構成

```
firmware/
  RP2040/         RP2040 ファームウェア (.uf2)
  ESP32C3/        ESP32-C3 ファームウェア (.bin)
web/
  remote.html     Web リモコン本体
  remote.html.gz  Web リモコン（gzip圧縮版・ESP32-C3 書き込み用）
app/
  windows/        Windows 用コントロールアプリ (.exe)
  android/        Android 用コントロールアプリ (.apk)
docs/
  Manual          組み立てマニュアル (PDF)
  Schematic       回路図 (PDF)
```

---

## 導入手順

詳細は `docs/` フォルダ内の組み立てマニュアルをご参照ください。

### ファームウェア書き込み

**RP2040 (Raspberry Pi Pico Zero)**
1. BOOTSEL ボタンを押しながら USB 接続
2. 表示された RPI-RP2 ドライブに `.uf2` ファイルをコピー

**ESP32-C3 (XIAO ESP32-C3)**
1. [esptool / espboards.dev](https://espboards.dev) を使用
2. `.merged.bin` を アドレス `0x0` に書き込み

### Web リモコン（remote.html）の更新

1. ESP32-C3 の Web UI（`http://[IPアドレス]/`）にアクセス
2. `/upload` エンドポイントから `remote.html.gz` をアップロード

### Android アプリのインストール

1. Android 端末の設定で「提供元不明のアプリ」のインストールを許可
   - 設定 → セキュリティ → 不明なアプリのインストール
2. `.apk` ファイルをダウンロードしてインストール

---

## リリース履歴

[Releases ページ](../../releases) をご参照ください。

---

## 免責事項

- 本キットは趣味目的の個人製作品です。動作を保証するものではありません。
- 製作・使用による損害について、頒布者は一切の責任を負いません。
- 航空無線（AIR バンド）は**受信専用**です。送信は法律で禁止されています。

---

## お問い合わせ

不具合・ご質問は [Issues](../../issues) よりお知らせください。
