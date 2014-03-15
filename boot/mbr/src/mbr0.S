    .code16gcc
    .text

    .extern main

    # CSを0にするために次の命令にロングジャンプする
    ljmp  $0, $0x7C05

    # レジスタを初期化する
    movw  $0, %ax
    movw  %ax, %ss
    movw  %ax, %es
    movw  %ax, %fs
    movw  %ax, %gs
    movw  $0x7C00, %sp
    movw  %ax, %ds

    pushw  %dx  # ドライブ番号を保存しておく

    # 自分自身を再配置する
    movw  $0x7C00, %si
    movw  $0x0500, %di
    cld
    movw  $512, %cx
    rep movsb

    # 新しい再配置先から開始する
    ljmp  $0, $rel_start

rel_start:
    # ディスクからデータを読み込む
    call main

    popw  %dx  # ドライブ番号を復元する

    # メモリへ読み込んだバイナリを実行
    jmp   0x7C00
