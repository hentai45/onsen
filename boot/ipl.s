# **** 機能
# FDからIPL以降の内容をメモリに読み込み、実行する


# **** ディスク読み込み時の注意
# (このプログラムの場合は1セクタずつ読み込んでいるので気にしなくてよい)
#
# ・複数のトラックに、またがって読み込めない
#
# ・64KB境界をまたごうとするとエラーになる(エラーコード %AH=0x09)
# つまり0x10000, 0x20000, ... をまたいで読み込めない


.code16gcc

#----------------------------------------------------------------------
# 定数
#----------------------------------------------------------------------
.equ READ_CYLS, 32      # 読み込むシリンダ数(1シリンダ = 18KB) max = 32


#----------------------------------------------------------------------
# コード
#----------------------------------------------------------------------

.text

    jmp   entry

    #----------------------------------------------------------------------
    # FAT12
    #----------------------------------------------------------------------

    .byte   0x90
    .ascii  "ONSEN   "        # ブートセクタの名前(8バイト)
    .word   512               # 1セクタの大きさ
    .byte   1                 # クラスタの大きさ
    .word   1                 # FATがどこから始まるか
    .byte   2                 # FATの個数
    .word   224               # ルートディレクトリのサイズ
    .word   2880              # このドライブの大きさ
    .byte   0xF0              # メディアのタイプ
    .word   9                 # FAT領域の長さ
    .word   18                # 1トラックにいくつのセクタがあるか
    .word   2                 # ヘッドの数
    .int    0                 # 必ず0
    .int    2880              # ドライブのサイズ
    .byte   0, 0, 0x29
    .int    0xFFFFFFFF        # ボリュームシリアル番号
    .ascii  "ONSEN      "     # ディスクの名前(11バイト)
    .ascii  "FAT12   "        # フォーマットの名前
    .skip   18, 0x00          # とりあえず18バイト空けておく

entry:

    #----------------------------------------------------------------------
    # レジスタ初期化
    #----------------------------------------------------------------------

    movw  $0, %ax
    movw  %ax, %ss
    movw  $0x7C00, %sp
    movw  %ax, %ds


    #----------------------------------------------------------------------
    # FDの内容をメモリへ読み込む
    #----------------------------------------------------------------------

    # 読み込み元(FD)の位置
    movb  $0, %dl  # Aドライブ
    movb  $0, %ch  # シリンダ0
    movb  $0, %dh  # ヘッド0
    movb  $2, %cl  # セクタ2

    # 書き込み先（メモリ）の位置。ES:BX
    movw  $0x0820, %di  # diには書き込み先のセグメント位置を入れておく
    movw  $0, %bx

readloop:
    movb  $0x02, %ah  # ディスク読み込み
    movb  $1, %al     # 読み込むセクタ数
    movw  %di, %es    # 書き込み先の位置を設定
    int   $0x13       # エラーならcfがオン。ahも変更される

    jc    error

    # 次の読み込みの準備

    addw  $512 / 16, %di  # 次の書き込み位置を計算
    addb  $1, %cl         # 次のセクタへ

    # cl(セクタ) <= 18 ならreadloopへ
    cmpb  $18, %cl
    jbe   readloop

    # 読み込み元(FD)の位置
    movb  $1, %cl  # セクタ1
    addb  $1, %dh  # ヘッドを変更

    # 書き込み先（メモリ）の位置。ES:BX
    movw  %di, %es

    # dh(ヘッド) < 2 （ヘッドが裏になった）ならreadloopへ
    cmpb  $2, %dh
    jb    readloop

    # 読み込み元(FD)の位置
    movb  $0, %dh  # ヘッド0
    addb  $1, %ch  # 次のシリンダ

    # ch < READ_CYLS （指定シリンダ数を読み込んでない）ならreadloopへ
    cmpb  $READ_CYLS, %ch
    jb    readloop


    #----------------------------------------------------------------------
    # メモリへ読み込んだバイナリを実行
    #----------------------------------------------------------------------

    # セグメントベースをオフセットに変換してジャンプ
    # 0x8000 + 0x4200 (FD読み込み先メモリ + FD(FAT12)のファイル格納位置)
    jmp   0xC200


    #----------------------------------------------------------------------
    # エラー処理
    #----------------------------------------------------------------------

error:
    movw  $load_err_msg, %si
    call  print
error_fin:
    hlt
    jmp   error_fin


    #----------------------------------------------------------------------
    # 文字列出力
    #----------------------------------------------------------------------

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


#----------------------------------------------------------------------
# データ
#----------------------------------------------------------------------

.data

load_err_msg:
    .string "\nload error\n"
