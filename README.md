# ST7789高速描画ライブラリ

## 概要

RP2040(C/C++SDK)用のST7789制御ライブラリ。

DMAを使っているので高速だと思われる。

機能も最低限。

## 使い方

ソースコードを参照のこと

## ベンチマーク実行結果

```
=========================================
ST7789 Graphics Library Test Programme
             (C)2023 I.H(@LunaTsukinashi)
=========================================

LCD Init...OK!

-----------------------------------------
1.Screen Refreshrate Test Begin.
Iteration:0600 frames.

0150 frames:2.237 sec
0300 frames:4.461 sec
0450 frames:6.684 sec
Total Time:8.893 sec.,Framerate: 67.47 fps

-----------------------------------------
2.Sprite Rendering Test Begin.
Iteration:24000 sprites.

03000 sprites:0.826 sec
06000 sprites:1.652 sec
09000 sprites:2.478 sec
12000 sprites:3.303 sec
15000 sprites:4.129 sec
18000 sprites:4.955 sec
21000 sprites:5.781 sec
Total Time:6.606 sec.,3633.04 sprites/sec

All Test Finished.
-----------------------------------------
SUMMARY:
        67.47 frames/sec
        3633.04 sprites/sec
=========================================
```

## ライセンス

```
        DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE 
                    Version 2, December 2004 

 Copyright (C) 2023 I.H(@LunaTsukinashi) <System32@live.jp>

 Everyone is permitted to copy and distribute verbatim or modified 
 copies of this license document, and changing it is allowed as long 
 as the name is changed. 

            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE 
   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION 

  0. You just DO WHAT THE FUCK YOU WANT TO.
```