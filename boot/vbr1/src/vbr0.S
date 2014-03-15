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

    pushw  %dx  # ドライブ番号を引数として渡す

    # ディスクからデータを読み込む
    call main

    popw  %dx

    # メモリへ読み込んだバイナリを実行
    jmp   0x7E00
