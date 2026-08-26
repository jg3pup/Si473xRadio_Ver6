# Si473X Radio Ver6

Si473X シリーズ IC を使用したマルチバンドラジオキットです。  
AM / FM / SW / AIR バンドに対応し、Web ブラウザからリモート操作が可能です。

---

## 特徴

- **マルチバンド対応** : MW / FM / SW / AIR（航空無線）
- **Web リモコン** : スマートフォン・PC ブラウザから操作可能
- **PC アプリ** : Windows 対応コントロールアプリ付属
- **Android アプリ** : Android 対応コントロールアプリ付属
- **OTA アップデート** : ESP32-C3 経由でファームウェアを無線更新可能

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
  Si473x_Radio_Manual.pdf    操作説明書
  Manual                     組み立てマニュアル (PDF)
  Schematic                  回路図 (PDF)
```

---

## 導入手順

詳細は `docs/` フォルダ内の組み立てマニュアルをご参照ください。

### RP2040 ファームウェア書き換え

Arduino IDE は不要です。UF2 ファイルをコピーするだけで書き換えできます。

**必要なもの**
- `.uf2` ファイル（本リリースページからダウンロード）
- データ転送対応 USB ケーブル（充電専用ケーブルは不可）

**手順**
1. データ転送対応 USB ケーブルでラジオ本体と PC を接続
2. **BAND+ と BAND− を両方押したまま POWER ボタンで電源 ON**  
   → LED が青く 0.5 秒点灯したらボタンを離す  
   → PC に「RPI-RP2」ドライブとして認識される
3. 「RPI-RP2」ドライブに `.uf2` ファイルをドラッグ＆ドロップ
4. 自動で書き込みが完了しラジオが再起動する

> ⚠️ RPI-RP2 が認識されない場合は USB ケーブルを確認してください（充電専用ケーブルは不可）。

---

### ESP32-C3 ファームウェア書き換え

Arduino IDE は不要です。Chrome ブラウザと [espboards.dev](https://www.espboards.dev/tools/program/) で書き込めます。

**必要なもの**
- マージ済み `.bin` ファイル（本リリースページからダウンロード）
- データ転送対応 USB ケーブル
- Google Chrome または Microsoft Edge（最新版）

> ⚠️ **書き換え前に必ず WiFi スライドスイッチ（SW7）を OFF にしてください。**  
> ON のままだとラジオ本体の電源と競合し、誤動作や故障の原因になります。

**手順**
1. Chrome / Edge で https://www.espboards.dev/tools/program/ を開く
2. WiFi スライドスイッチ（SW7）を **OFF** にしてから USB ケーブルで XIAO ESP32-C3 の USB ポートと PC を接続
3. 「Connect」ボタンをクリック → COM ポートを選択して「接続」
4. `.bin` ファイルを指定、アドレスは **`0x0`** を指定
5. 「Flash（Program）」をクリックして書き込み開始
6. 書き込み完了後、USB ケーブルを抜いてから WiFi スイッチを ON に戻す

---

### Web リモコン・局データのアップロード

ESP32-C3 FW 書き換え後は LittleFS が消去されるため、以下のファイルを再アップロードしてください。

1. ラジオを STA モードで起動し、Chrome で `http://[IPアドレス]/upload` にアクセス
2. 以下のファイルを順にアップロード

| ファイル | アップロード欄 |
|----------|--------------|
| `remote.html.gz` | ① Web リモコン HTML |
| `japan_stations.json` | ② 国内放送局データ |
| `air_stations.json` | ④ AIR 局データ |

3. アップロード完了後、WEB リモコンの「国内局」→「ESP から読み込む」で局データを反映

### Android アプリのインストール

1. Android 端末の設定で「提供元不明のアプリ」のインストールを許可
   - 設定 → セキュリティ → 不明なアプリのインストール
2. `.apk` ファイルをダウンロードしてインストール

---
## Third-Party Notices

The FT8/FT4 auto-decode feature embeds a WebAssembly module built from
[`kgoba/ft8_lib`](https://github.com/kgoba/ft8_lib) (**MIT License**,
Copyright (c) 2018 Kārlis Goba), via a small original C wrapper
(`decode_wrap.c`). See [`THIRD_PARTY_NOTICES.md`](./THIRD_PARTY_NOTICES.md)
for the full notice and license text. This does not change the license of
this repository itself — see [`LICENSE`](./LICENSE).

<!-- 日本語版 -->

## サードパーティ通知

FT8/FT4自動判読機能は、[`kgoba/ft8_lib`](https://github.com/kgoba/ft8_lib)
（**MITライセンス**、Copyright (c) 2018 Kārlis Goba）をもとに自作のCラッパー
（`decode_wrap.c`）でビルドしたWebAssemblyモジュールを組み込んでいます。詳細な
通知文とライセンス全文は [`THIRD_PARTY_NOTICES.md`](./THIRD_PARTY_NOTICES.md)
をご覧ください。この機能によって本リポジトリ自体のライセンス（
[`LICENSE`](./LICENSE)）が変わることはありません。

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

---

## お問い合わせ

不具合・ご質問は [Issues](../../issues) よりお知らせください。
