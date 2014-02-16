# プロテクトモードに移行し、onsen.binをセグメントに配置し実行する


#---------------------------------------------------------------------
# 定数
#---------------------------------------------------------------------

.equ IPL, 0x7C00                 # IPLが読み込まれている場所
.equ ONSENSYS, 0x00280000        # onsen.sysのロード先
.equ DISK_CACHE, 0x00100000      # ディスクキャッシュの場所
.equ DISK_CACHE_REAL, 0x00008000 # ディスクキャッシュの場所（リアルモード）

.equ CYLS, 32                    # IPLが読み込んだシリンダ数

.equ VRAM, 0x0FF0                # グラフィックバッファの開始番地

.equ VBEMODE, 0x111              # 640 x  480 x 16bitカラー
.equ DEPTH, 16                   # 色のビット数


#---------------------------------------------------------------------
# コード
#---------------------------------------------------------------------

.text

.code16gcc

    call  set_video_mode         # 画面モードの設定
    call  disable_interrupts     # 割り込みを受け付けないようにする
    call  enable_a20             # 1MB以上のメモリにアクセスできるようにする

.arch i486

    call  to_protect_mode        # プロテクトモードに移行
    call  move_disk_data         # ディスクデータをキャッシュへ転送
    call  move_and_run_onsensys  # onsen.sysを転送して実行


#---------------------------------------------------------------------
# 関数
#---------------------------------------------------------------------

.code16gcc

# 画面モード設定
set_video_mode:
    # VBE存在確認
    movw  $0x9000, %ax
    movw  %ax, %es
    movw  $0, %di
    movw  $0x4F00, %ax
    int   $0x10
    cmpw  $0x004F, %ax
    jne   error_vbe

    # VBEのバージョンチェック
    movw  %es:4(%di), %ax
    cmpw  $0x0200, %ax
    jb    error_vbe

    # 画面モード情報を取得
    movw  $VBEMODE, %cx
    movw  $0x4F01, %ax
    int   $0x10
    cmpw  $0x004F, %ax
    jne   error_vbe

    # 画面モード情報の確認
    cmpb  $DEPTH, %es:0x19(%di)
    jne   error_vbe
    cmpb  $6, %es:0x1B(%di)
    jne   error_vbe
    movw  %es:0x00(%di), %ax
    andw  $0x0080, %ax
    jz    error_vbe  # モード属性のbit7が0だったらあきらめる

    # 画面モード切り替え
    movw  $VBEMODE + 0x4000, %bx
    movw  $0x4F02, %ax
    int   $0x10

    # 画面情報を記録
    movl  %es:0x28(%di), %eax
    movl  %eax, (VRAM)
    ret

# エラー処理
error_vbe:
    movw  $vbe_err_msg, %si
    call  print
error_fin:
    hlt
    jmp   error_fin

vbe_err_msg:
    .string "\nsetting video mode error\n"


# 文字列出力
# si：文字列のアドレス
print:
    movb  (%si), %al
    addw  $1, %si
    cmpb  $0, %al
    je    print_fin
    movb  $0x0E, %ah  # 一文字表示ファンクション
    movw  $15, %bx    # カラーコード
    int   $0x10       # ビデオBIOS呼び出し
    jmp   print
print_fin:
    ret


# 一切の割り込みを受け付けないようにする
# AT互換機の仕様では、
# CLI前にこれをやっておかないと、たまにハングアップする
disable_interrupts:
    movb  $0xFF, %al
    outb  %al, $0x21  # 全マスタの割り込みを禁止
    nop               # OUT命令を連続させるとうまくいかない機種があるらしい
    outb  %al, $0xA1  # 全スレーブの割り込みを禁止
    cli               # CPUレベルでも割り込み禁止
    ret


# CPUから1MB以上のメモリにアクセスできるように、A20GATEを設定
enable_a20:
    call  wait_kb_out
    movb  $0xD1, %al
    outb  %al, $0x64
    call  wait_kb_out
    movb  $0xDF, %al  # enable A20
    outb  %al, $0x60
    call  wait_kb_out
    ret


.arch i486


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


# ディスクデータをキャッシュへ転送
move_disk_data:
    # ブートセクタ
    movl  $IPL, %esi         # 転送元
    movl  $DISK_CACHE, %edi  # 転送先
    movl  $512 / 4, %ecx     # 転送ダブルワード数
    call  copy_memory

    # 残り
    movl  $DISK_CACHE_REAL + 512, %esi  # 転送元
    movl  $DISK_CACHE + 512, %edi       # 転送先
    # 転送ダブルワード数の計算
    movl  $0, %ecx
    movb  $CYLS, %cl                # IPLが読み込んだシリンダ数
    imull $512 * 18 * 2 / 4, %ecx   # シリンダ数からダブルワード数に変換
    subl  $512 / 4, %ecx            # IPLの分だけ差し引く
    call  copy_memory
    ret


# onsen.sysを転送して実行
move_and_run_onsensys:
    # onsen.sysを実行可能セグメントへ転送
    movl  $onsensys, %esi        # 転送元
    movl  $ONSENSYS, %edi        # 転送先
    movl  $512 * 1024 / 4, %ecx  # 転送ダブルワード数
    call  copy_memory

    # .data部分は読み書き可能セグメントへ転送
    movl  $ONSENSYS, %ebx
    movl  16(%ebx), %ecx
    addl  $3, %ecx
    shrl  $2, %ecx
    jz    skip            # 転送すべきものがない
    movl  20(%ebx), %esi  # 転送元
    addl  %ebx, %esi
    movl  12(%ebx), %edi  # 転送先
    call  copy_memory
skip:
    movl  12(%ebx), %esp  # スタック初期値

    # onsen.sys実行
    ljmpl $2*8, $0x28001B  # 2つめの実行可能なセグメントへジャンプ
    ret


# キーボードコントローラに対し入出力ができるまで待機
wait_kb_out:
    inb   $0x64, %al
    andb  $0x02, %al
    jnz   wait_kb_out
    ret


# メモリをコピー
# esi : 転送元
# edi : 転送先
# ecx : 転送ダブルワード数
copy_memory:
    movl  (%esi), %eax
    addl  $4, %esi
    movl  %eax, (%edi)
    addl  $4, %edi
    subl  $1, %ecx
    jnz   copy_memory
    ret
# memcpyはアドレスサイズプリフィクスを入れ忘れなければ、
# ストリング命令でも書ける


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

.align 8


#---------------------------------------------------------------------
# onsen.sys本体
#---------------------------------------------------------------------

onsensys:


