# プロテクトモードに移行し、onsen.sysをセグメントに配置し実行する


#---------------------------------------------------------------------
# コード
#---------------------------------------------------------------------

.text

.code16gcc
.arch i486

    call  set_video_mode         # 画面モードの設定
    call  disable_interrupts     # 割り込みを受け付けないようにする
    call  enable_a20             # 1MB以上のメモリにアクセスできるようにする

    call  to_protect_mode        # プロテクトモードに移行
    call  move_disk_data         # ディスクデータをキャッシュへ転送
    call  move_onsensys          # onsen.sysを転送
    call  run_onsensys           # onsen.sysを実行


#---------------------------------------------------------------------
# 関数
#---------------------------------------------------------------------


# プロテクトモードに移行
to_protect_mode:
    lgdtl (GDTR0)            # 仮のGDTを設定
    movl  %cr0, %eax
    andl  $0x7FFFFFFF, %eax  # ページングを禁止(bit31 = 0)
    orl   $0x00000001, %eax  # プロテクトモード移行(bit0 = 1)
    movl  %eax, %cr0
    jmp   flush_pipeline

flush_pipeline:
    movw  $1*8, %ax  # 読み書き可能セグメント32bit
    movw  %ax, %ds
    movw  %ax, %es
    movw  %ax, %fs
    movw  %ax, %gs
    movw  %ax, %ss
    ret

#---------------------------------------------------------------------
# 仮のGDT
#---------------------------------------------------------------------

.align 8
GDT0:
    .skip 8, 0  # ヌルセレクタ

    # 開始番地   ： 0x00000000
    # 大きさ     ： 4GB
    # 管理用属性 ： システム専用の読み書き可能なセグメント。
    #               実行はできない。
    .word 0xFFFF, 0x0000, 0x9200, 0x00CF

    # onsen.sys用セグメント
    # 開始番地   ： 0x00000000
    # 大きさ     ： 4GB
    # 管理用属性 ： システム専用の実行可能なセグメント。
    #               読み込みもOK。書き込みはできない。
    .word 0xFFFF, 0x0000, 0x9A00, 0x00CF

    .word 0

GDTR0:
    .word 8 * 3 - 1  # リミット（GDTの有効バイト数-1）
    .int GDT0  # GDTが置いてある番地

