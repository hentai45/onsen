MBR は fdisk を使ってもつくることができる。

http://wiki.osdev.org/Loopback_Device

（例）シリンダ数を3にする場合
$ dd if=/dev/zero of=onsen.img bs=516096c count=3
$ fdisk -u -C3 -S63 -H16 onsen.img


ldscripts/mbr.lds のパーティションテーブルを書き換えた場合は、
fdiskのvコマンドでチェックすること。
