
src/asmhead.o:     file format elf32-i386


Disassembly of section .text:

00000000 <old_set_video_mode-0x2a>:
   0:	66 e8 fc ff          	callw  0 <old_set_video_mode-0x2a>
   4:	ff                   	(bad)  
   5:	ff 66 e8             	jmp    *-0x18(%esi)
   8:	fc                   	cld    
   9:	ff                   	(bad)  
   a:	ff                   	(bad)  
   b:	ff 66 e8             	jmp    *-0x18(%esi)
   e:	fc                   	cld    
   f:	ff                   	(bad)  
  10:	ff                   	(bad)  
  11:	ff 66 e8             	jmp    *-0x18(%esi)
  14:	fc                   	cld    
  15:	ff                   	(bad)  
  16:	ff                   	(bad)  
  17:	ff 66 e8             	jmp    *-0x18(%esi)
  1a:	be 00 00 00 66       	mov    $0x66000000,%esi
  1f:	e8 fc ff ff ff       	call   20 <old_set_video_mode-0xa>
  24:	66 e8 fc ff          	callw  24 <old_set_video_mode-0x6>
  28:	ff                   	(bad)  
  29:	ff                   	(bad)  

0000002a <old_set_video_mode>:
  2a:	b8 00 90 8e c0       	mov    $0xc08e9000,%eax
  2f:	bf 00 00 b8 00       	mov    $0xb80000,%edi
  34:	4f                   	dec    %edi
  35:	cd 10                	int    $0x10
  37:	83 f8 4f             	cmp    $0x4f,%eax
  3a:	75 3f                	jne    7b <error_vbe>
  3c:	26 8b 45 04          	mov    %es:0x4(%ebp),%eax
  40:	3d 00 02 72 36       	cmp    $0x36720200,%eax
  45:	b9 11 01 b8 01       	mov    $0x1b80111,%ecx
  4a:	4f                   	dec    %edi
  4b:	cd 10                	int    $0x10
  4d:	83 f8 4f             	cmp    $0x4f,%eax
  50:	75 29                	jne    7b <error_vbe>
  52:	26 80 7d 19 10       	cmpb   $0x10,%es:0x19(%ebp)
  57:	75 22                	jne    7b <error_vbe>
  59:	26 80 7d 1b 06       	cmpb   $0x6,%es:0x1b(%ebp)
  5e:	75 1b                	jne    7b <error_vbe>
  60:	26 8b 05 25 80 00 74 	mov    %es:0x74008025,%eax
  67:	13 bb 11 41 b8 02    	adc    0x2b84111(%ebx),%edi
  6d:	4f                   	dec    %edi
  6e:	cd 10                	int    $0x10
  70:	26 66 8b 45 28       	mov    %es:0x28(%ebp),%ax
  75:	66 a3 f0 0f 66 c3    	mov    %ax,0xc3660ff0

0000007b <error_vbe>:
  7b:	be 87 00 66 e8       	mov    $0xe8660087,%esi
  80:	1e                   	push   %ds
  81:	00 00                	add    %al,(%eax)
	...

00000084 <error_fin>:
  84:	f4                   	hlt    
  85:	eb fd                	jmp    84 <error_fin>

00000087 <vbe_err_msg>:
  87:	0a 73 65             	or     0x65(%ebx),%dh
  8a:	74 74                	je     100 <flush_pipeline+0xc>
  8c:	69 6e 67 20 76 69 64 	imul   $0x64697620,0x67(%esi),%ebp
  93:	65 6f                	outsl  %gs:(%esi),(%dx)
  95:	20 6d 6f             	and    %ch,0x6f(%ebp)
  98:	64 65 20 65 72       	fs and %ah,%fs:%gs:0x72(%ebp)
  9d:	72 6f                	jb     10e <old_move_disk_data+0xb>
  9f:	72 0a                	jb     ab <print+0x9>
	...

000000a2 <print>:
  a2:	8a 04 83             	mov    (%ebx,%eax,4),%al
  a5:	c6 01 3c             	movb   $0x3c,(%ecx)
  a8:	00 74 09 b4          	add    %dh,-0x4c(%ecx,%ecx,1)
  ac:	0e                   	push   %cs
  ad:	bb 0f 00 cd 10       	mov    $0x10cd000f,%ebx
  b2:	eb ee                	jmp    a2 <print>

000000b4 <print_fin>:
  b4:	66 c3                	retw   

000000b6 <old_disable_interrupts>:
  b6:	b0 ff                	mov    $0xff,%al
  b8:	e6 21                	out    %al,$0x21
  ba:	90                   	nop
  bb:	e6 a1                	out    %al,$0xa1
  bd:	fa                   	cli    
  be:	66 c3                	retw   

000000c0 <old_enable_a20>:
  c0:	66 e8 ce 00          	callw  192 <skip+0xd>
  c4:	00 00                	add    %al,(%eax)
  c6:	b0 d1                	mov    $0xd1,%al
  c8:	e6 64                	out    %al,$0x64
  ca:	66 e8 c4 00          	callw  192 <skip+0xd>
  ce:	00 00                	add    %al,(%eax)
  d0:	b0 df                	mov    $0xdf,%al
  d2:	e6 60                	out    %al,$0x60
  d4:	66 e8 ba 00          	callw  192 <skip+0xd>
  d8:	00 00                	add    %al,(%eax)
  da:	66 c3                	retw   

000000dc <to_protect_mode>:
  dc:	66 0f 01 16          	lgdtw  (%esi)
  e0:	d2 01                	rolb   %cl,(%ecx)
  e2:	0f 20 c0             	mov    %cr0,%eax
  e5:	66 25 ff ff          	and    $0xffff,%ax
  e9:	ff                   	(bad)  
  ea:	7f 66                	jg     152 <old_move_and_run_onsensys+0xd>
  ec:	83 c8 01             	or     $0x1,%eax
  ef:	0f 22 c0             	mov    %eax,%cr0
  f2:	eb 00                	jmp    f4 <flush_pipeline>

000000f4 <flush_pipeline>:
  f4:	b8 08 00 8e d8       	mov    $0xd88e0008,%eax
  f9:	8e c0                	mov    %eax,%es
  fb:	8e e0                	mov    %eax,%fs
  fd:	8e e8                	mov    %eax,%gs
  ff:	8e d0                	mov    %eax,%ss
 101:	66 c3                	retw   

00000103 <old_move_disk_data>:
 103:	66 be 00 7c          	mov    $0x7c00,%si
 107:	00 00                	add    %al,(%eax)
 109:	66 bf 00 00          	mov    $0x0,%di
 10d:	10 00                	adc    %al,(%eax)
 10f:	66 b9 80 00          	mov    $0x80,%cx
 113:	00 00                	add    %al,(%eax)
 115:	66 e8 81 00          	callw  19a <wait_kb_out+0x6>
 119:	00 00                	add    %al,(%eax)
 11b:	66 be 00 82          	mov    $0x8200,%si
 11f:	00 00                	add    %al,(%eax)
 121:	66 bf 00 02          	mov    $0x200,%di
 125:	10 00                	adc    %al,(%eax)
 127:	66 b9 00 00          	mov    $0x0,%cx
 12b:	00 00                	add    %al,(%eax)
 12d:	b1 20                	mov    $0x20,%cl
 12f:	66 69 c9 00 12       	imul   $0x1200,%cx,%cx
 134:	00 00                	add    %al,(%eax)
 136:	66 81 e9 80 00       	sub    $0x80,%cx
 13b:	00 00                	add    %al,(%eax)
 13d:	66 e8 59 00          	callw  19a <wait_kb_out+0x6>
 141:	00 00                	add    %al,(%eax)
 143:	66 c3                	retw   

00000145 <old_move_and_run_onsensys>:
 145:	66 be 00 00          	mov    $0x0,%si
 149:	00 00                	add    %al,(%eax)
 14b:	66 bf 00 00          	mov    $0x0,%di
 14f:	28 00                	sub    %al,(%eax)
 151:	66 b9 00 00          	mov    $0x0,%cx
 155:	02 00                	add    (%eax),%al
 157:	66 e8 3f 00          	callw  19a <wait_kb_out+0x6>
 15b:	00 00                	add    %al,(%eax)
 15d:	66 bb 00 00          	mov    $0x0,%bx
 161:	28 00                	sub    %al,(%eax)
 163:	67 66 8b 4b 10       	mov    0x10(%bp,%di),%cx
 168:	66 83 c1 03          	add    $0x3,%cx
 16c:	66 c1 e9 02          	shr    $0x2,%cx
 170:	74 13                	je     185 <skip>
 172:	67 66 8b 73 14       	mov    0x14(%bp,%di),%si
 177:	66 01 de             	add    %bx,%si
 17a:	67 66 8b 7b 0c       	mov    0xc(%bp,%di),%di
 17f:	66 e8 17 00          	callw  19a <wait_kb_out+0x6>
	...

00000185 <skip>:
 185:	67 66 8b 63 0c       	mov    0xc(%bp,%di),%sp
 18a:	66 ea 1b 00 28 00    	ljmpw  $0x28,$0x1b
 190:	10 00                	adc    %al,(%eax)
 192:	66 c3                	retw   

00000194 <wait_kb_out>:
 194:	e4 64                	in     $0x64,%al
 196:	24 02                	and    $0x2,%al
 198:	75 fa                	jne    194 <wait_kb_out>
 19a:	66 c3                	retw   

0000019c <copy_memory>:
 19c:	67 66 8b 06 66 83    	mov    -0x7c9a,%ax
 1a2:	c6 04 67 66          	movb   $0x66,(%edi,%eiz,2)
 1a6:	89 07                	mov    %eax,(%edi)
 1a8:	66 83 c7 04          	add    $0x4,%di
 1ac:	66 83 e9 01          	sub    $0x1,%cx
 1b0:	75 ea                	jne    19c <copy_memory>
 1b2:	66 c3                	retw   
 1b4:	8d b4 00 00 00 00 00 	lea    0x0(%eax,%eax,1),%esi

000001b8 <GDT0>:
	...
 1c0:	ff                   	(bad)  
 1c1:	ff 00                	incl   (%eax)
 1c3:	00 00                	add    %al,(%eax)
 1c5:	92                   	xchg   %eax,%edx
 1c6:	cf                   	iret   
 1c7:	00 ff                	add    %bh,%bh
 1c9:	ff 00                	incl   (%eax)
 1cb:	00 00                	add    %al,(%eax)
 1cd:	9a cf 00 00 00 17 00 	lcall  $0x17,$0xcf

000001d2 <GDTR0>:
 1d2:	17                   	pop    %ss
 1d3:	00                   	.byte 0x0
 1d4:	b8                   	.byte 0xb8
 1d5:	01 00                	add    %eax,(%eax)
	...

src/gdt.o:     file format elf32-i386


Disassembly of section .text:

00000000 <gdt_init>:
   0:	55                   	push   %ebp
   1:	89 e5                	mov    %esp,%ebp
   3:	53                   	push   %ebx
   4:	83 ec 14             	sub    $0x14,%esp
   7:	31 db                	xor    %ebx,%ebx
   9:	50                   	push   %eax
   a:	50                   	push   %eax
   b:	6a 00                	push   $0x0
   d:	6a 00                	push   $0x0
   f:	6a 00                	push   $0x0
  11:	6a 00                	push   $0x0
  13:	6a 00                	push   $0x0
  15:	53                   	push   %ebx
  16:	43                   	inc    %ebx
  17:	e8 fc ff ff ff       	call   18 <gdt_init+0x18>
  1c:	83 c4 20             	add    $0x20,%esp
  1f:	81 fb 00 20 00 00    	cmp    $0x2000,%ebx
  25:	75 e2                	jne    9 <gdt_init+0x9>
  27:	6a 00                	push   $0x0
  29:	6a ff                	push   $0xffffffff
  2b:	6a 00                	push   $0x0
  2d:	6a 01                	push   $0x1
  2f:	e8 fc ff ff ff       	call   30 <gdt_init+0x30>
  34:	6a 00                	push   $0x0
  36:	6a ff                	push   $0xffffffff
  38:	6a 00                	push   $0x0
  3a:	6a 02                	push   $0x2
  3c:	e8 fc ff ff ff       	call   3d <gdt_init+0x3d>
  41:	83 c4 20             	add    $0x20,%esp
  44:	6a 03                	push   $0x3
  46:	6a ff                	push   $0xffffffff
  48:	6a 00                	push   $0x0
  4a:	6a 03                	push   $0x3
  4c:	e8 fc ff ff ff       	call   4d <gdt_init+0x4d>
  51:	6a 03                	push   $0x3
  53:	6a ff                	push   $0xffffffff
  55:	6a 00                	push   $0x0
  57:	6a 04                	push   $0x4
  59:	e8 fc ff ff ff       	call   5a <gdt_init+0x5a>
  5e:	66 c7 45 ec ff ff    	movw   $0xffff,-0x14(%ebp)
  64:	c7 45 ee 00 00 27 00 	movl   $0x270000,-0x12(%ebp)
  6b:	8d 45 ec             	lea    -0x14(%ebp),%eax
  6e:	0f 01 10             	lgdtl  (%eax)
  71:	66 c7 45 f6 10 00    	movw   $0x10,-0xa(%ebp)
  77:	c7 45 f2 27 00 00 00 	movl   $0x27,-0xe(%ebp)
  7e:	8d 45 f2             	lea    -0xe(%ebp),%eax
  81:	ff 28                	ljmp   *(%eax)
  83:	b8 08 00 00 00       	mov    $0x8,%eax
  88:	83 c4 20             	add    $0x20,%esp
  8b:	8e d8                	mov    %eax,%ds
  8d:	8e c0                	mov    %eax,%es
  8f:	8e e0                	mov    %eax,%fs
  91:	8e e8                	mov    %eax,%gs
  93:	8e d0                	mov    %eax,%ss
  95:	8b 5d fc             	mov    -0x4(%ebp),%ebx
  98:	c9                   	leave  
  99:	c3                   	ret    

0000009a <set_code_seg>:
  9a:	55                   	push   %ebp
  9b:	89 e5                	mov    %esp,%ebp
  9d:	83 ec 10             	sub    $0x10,%esp
  a0:	ff 75 14             	pushl  0x14(%ebp)
  a3:	6a 40                	push   $0x40
  a5:	68 9a 00 00 00       	push   $0x9a
  aa:	ff 75 10             	pushl  0x10(%ebp)
  ad:	ff 75 0c             	pushl  0xc(%ebp)
  b0:	ff 75 08             	pushl  0x8(%ebp)
  b3:	e8 fc ff ff ff       	call   b4 <set_code_seg+0x1a>
  b8:	83 c4 20             	add    $0x20,%esp
  bb:	c9                   	leave  
  bc:	c3                   	ret    

000000bd <set_data_seg>:
  bd:	55                   	push   %ebp
  be:	89 e5                	mov    %esp,%ebp
  c0:	83 ec 10             	sub    $0x10,%esp
  c3:	ff 75 14             	pushl  0x14(%ebp)
  c6:	6a 40                	push   $0x40
  c8:	68 92 00 00 00       	push   $0x92
  cd:	ff 75 10             	pushl  0x10(%ebp)
  d0:	ff 75 0c             	pushl  0xc(%ebp)
  d3:	ff 75 08             	pushl  0x8(%ebp)
  d6:	e8 fc ff ff ff       	call   d7 <set_data_seg+0x1a>
  db:	83 c4 20             	add    $0x20,%esp
  de:	c9                   	leave  
  df:	c3                   	ret    

000000e0 <set_seg_desc>:
  e0:	55                   	push   %ebp
  e1:	a1 00 00 00 00       	mov    0x0,%eax
  e6:	89 e5                	mov    %esp,%ebp
  e8:	57                   	push   %edi
  e9:	56                   	push   %esi
  ea:	53                   	push   %ebx
  eb:	8b 75 08             	mov    0x8(%ebp),%esi
  ee:	8b 55 10             	mov    0x10(%ebp),%edx
  f1:	8b 5d 0c             	mov    0xc(%ebp),%ebx
  f4:	8b 7d 18             	mov    0x18(%ebp),%edi
  f7:	8d 04 f0             	lea    (%eax,%esi,8),%eax
  fa:	81 fa ff ff 0f 00    	cmp    $0xfffff,%edx
 100:	76 09                	jbe    10b <set_seg_desc+0x2b>
 102:	c1 ea 0c             	shr    $0xc,%edx
 105:	81 cf 80 00 00 00    	or     $0x80,%edi
 10b:	66 89 10             	mov    %dx,(%eax)
 10e:	89 d9                	mov    %ebx,%ecx
 110:	c1 ea 10             	shr    $0x10,%edx
 113:	66 89 58 02          	mov    %bx,0x2(%eax)
 117:	c1 e9 10             	shr    $0x10,%ecx
 11a:	c1 eb 18             	shr    $0x18,%ebx
 11d:	88 48 04             	mov    %cl,0x4(%eax)
 120:	8d 0c 3a             	lea    (%edx,%edi,1),%ecx
 123:	8b 55 1c             	mov    0x1c(%ebp),%edx
 126:	c1 e2 05             	shl    $0x5,%edx
 129:	88 58 07             	mov    %bl,0x7(%eax)
 12c:	0b 55 14             	or     0x14(%ebp),%edx
 12f:	88 48 06             	mov    %cl,0x6(%eax)
 132:	88 50 05             	mov    %dl,0x5(%eax)
 135:	5b                   	pop    %ebx
 136:	5e                   	pop    %esi
 137:	5f                   	pop    %edi
 138:	5d                   	pop    %ebp
 139:	c3                   	ret    

src/head.o:     file format elf32-i386


Disassembly of section .text:

00000000 <testa>:
   0:	55                   	push   %ebp
   1:	89 e5                	mov    %esp,%ebp
   3:	90                   	nop
   4:	5d                   	pop    %ebp
   5:	c3                   	ret    

00000006 <disable_interrupts>:
   6:	66 55                	push   %bp
   8:	b0 ff                	mov    $0xff,%al
   a:	66 89 e5             	mov    %sp,%bp
   d:	e6 21                	out    %al,$0x21
   f:	90                   	nop
  10:	e6 a1                	out    %al,$0xa1
  12:	fa                   	cli    
  13:	66 5d                	pop    %bp
  15:	66 c3                	retw   

00000017 <enable_a20>:
  17:	66 55                	push   %bp
  19:	66 89 e5             	mov    %sp,%bp
  1c:	66 83 ec 08          	sub    $0x8,%sp
  20:	66 e8 fc ff          	callw  20 <enable_a20+0x9>
  24:	ff                   	(bad)  
  25:	ff b0 d1 e6 64 66    	pushl  0x6664e6d1(%eax)
  2b:	e8 fc ff ff ff       	call   2c <enable_a20+0x15>
  30:	b0 df                	mov    $0xdf,%al
  32:	e6 60                	out    %al,$0x60
  34:	66 c9                	leavew 
  36:	e9 fe ff 66 55       	jmp    55670039 <memcopy+0x5566ff6d>

00000039 <wait_kbc_sendready>:
  39:	66 55                	push   %bp
  3b:	66 89 e5             	mov    %sp,%bp
  3e:	e4 64                	in     $0x64,%al
  40:	a8 02                	test   $0x2,%al
  42:	75 fa                	jne    3e <wait_kbc_sendready+0x5>
  44:	66 5d                	pop    %bp
  46:	66 c3                	retw   

00000048 <move_disk_data>:
  48:	66 55                	push   %bp
  4a:	66 b9 00 02          	mov    $0x200,%cx
  4e:	00 00                	add    %al,(%eax)
  50:	66 89 e5             	mov    %sp,%bp
  53:	66 ba 00 7c          	mov    $0x7c00,%dx
  57:	00 00                	add    %al,(%eax)
  59:	66 83 ec 08          	sub    $0x8,%sp
  5d:	66 b8 00 00          	mov    $0x0,%ax
  61:	10 00                	adc    %al,(%eax)
  63:	66 e8 63 00          	callw  ca <move_and_run_onsensys+0x4b>
  67:	00 00                	add    %al,(%eax)
  69:	66 b9 00 f8          	mov    $0xf800,%cx
  6d:	0a 00                	or     (%eax),%al
  6f:	66 ba 00 82          	mov    $0x8200,%dx
  73:	00 00                	add    %al,(%eax)
  75:	66 b8 00 02          	mov    $0x200,%ax
  79:	10 00                	adc    %al,(%eax)
  7b:	66 c9                	leavew 
  7d:	eb 4d                	jmp    cc <memcopy>

0000007f <move_and_run_onsensys>:
  7f:	66 55                	push   %bp
  81:	66 b9 00 00          	mov    $0x0,%cx
  85:	08 00                	or     %al,(%eax)
  87:	66 89 e5             	mov    %sp,%bp
  8a:	66 ba 00 00          	mov    $0x0,%dx
  8e:	00 00                	add    %al,(%eax)
  90:	66 83 ec 08          	sub    $0x8,%sp
  94:	66 b8 00 00          	mov    $0x0,%ax
  98:	28 00                	sub    %al,(%eax)
  9a:	66 e8 2c 00          	callw  ca <move_and_run_onsensys+0x4b>
  9e:	00 00                	add    %al,(%eax)
  a0:	66 8b 0e             	mov    (%esi),%cx
  a3:	10 00                	adc    %al,(%eax)
  a5:	66 85 c9             	test   %cx,%cx
  a8:	74 0f                	je     b9 <move_and_run_onsensys+0x3a>
  aa:	66 8b 16             	mov    (%esi),%dx
  ad:	14 00                	adc    $0x0,%al
  af:	66 a1 0c 00 66 e8    	mov    0xe866000c,%ax
  b5:	13 00                	adc    (%eax),%eax
  b7:	00 00                	add    %al,(%eax)
  b9:	66 a1 0c 00 66 89    	mov    0x8966000c,%ax
  bf:	c4 66 ea             	les    -0x16(%esi),%esp
  c2:	1b 00                	sbb    (%eax),%eax
  c4:	28 00                	sub    %al,(%eax)
  c6:	10 00                	adc    %al,(%eax)
  c8:	66 c9                	leavew 
  ca:	66 c3                	retw   

000000cc <memcopy>:
  cc:	66 55                	push   %bp
  ce:	66 89 e5             	mov    %sp,%bp
  d1:	66 56                	push   %si
  d3:	66 89 ce             	mov    %cx,%si
  d6:	66 53                	push   %bx
  d8:	66 31 db             	xor    %bx,%bx
  db:	66 39 f3             	cmp    %si,%bx
  de:	7d 0c                	jge    ec <memcopy+0x20>
  e0:	67 8a 0c             	mov    (%si),%cl
  e3:	1a 67 88             	sbb    -0x78(%edi),%ah
  e6:	0c 18                	or     $0x18,%al
  e8:	66 43                	inc    %bx
  ea:	eb ef                	jmp    db <memcopy+0xf>
  ec:	66 5b                	pop    %bx
  ee:	66 5e                	pop    %si
  f0:	66 5d                	pop    %bp
  f2:	66 c3                	retw   

src/video.o:     file format elf32-i386


Disassembly of section .text:

00000000 <set_video_mode>:
   0:	66 55                	push   %bp
   2:	66 b8 00 4f          	mov    $0x4f00,%ax
   6:	00 00                	add    %al,(%eax)
   8:	66 89 e5             	mov    %sp,%bp
   b:	66 57                	push   %di
   d:	66 53                	push   %bx
   f:	67 66 8d bd f8 fd    	lea    -0x208(%di),%di
  15:	ff                   	(bad)  
  16:	ff 66 81             	jmp    *-0x7f(%esi)
  19:	ec                   	in     (%dx),%al
  1a:	00 02                	add    %al,(%edx)
  1c:	00 00                	add    %al,(%eax)
  1e:	66 89 fa             	mov    %di,%dx
  21:	66 83 e7 0f          	and    $0xf,%di
  25:	66 c1 ea 04          	shr    $0x4,%dx
  29:	89 d3                	mov    %edx,%ebx
  2b:	8e c3                	mov    %ebx,%es
  2d:	cd 10                	int    $0x10
  2f:	89 c7                	mov    %eax,%edi
  31:	f7 c7 4f 00 75 08    	test   $0x875004f,%edi
  37:	66 e8 7f 00          	callw  ba <set_video_mode+0xba>
  3b:	00 00                	add    %al,(%eax)
  3d:	eb 6e                	jmp    ad <set_video_mode+0xad>
  3f:	67 81 bd fc fd ff ff 	cmpl   $0x1ffffff,-0x204(%di)
  46:	ff 01 
  48:	76 ed                	jbe    37 <set_video_mode+0x37>
  4a:	67 66 8d bd f8 fe    	lea    -0x108(%di),%di
  50:	ff                   	(bad)  
  51:	ff 66 b9             	jmp    *-0x47(%esi)
  54:	11 01                	adc    %eax,(%ecx)
  56:	00 00                	add    %al,(%eax)
  58:	66 89 fa             	mov    %di,%dx
  5b:	66 b8 01 4f          	mov    $0x4f01,%ax
  5f:	00 00                	add    %al,(%eax)
  61:	66 c1 ea 04          	shr    $0x4,%dx
  65:	66 83 e7 0f          	and    $0xf,%di
  69:	89 d3                	mov    %edx,%ebx
  6b:	8e c3                	mov    %ebx,%es
  6d:	cd 10                	int    $0x10
  6f:	89 c7                	mov    %eax,%edi
  71:	f7 c7 4f 00 74 c0    	test   $0xc074004f,%edi
  77:	67 80 bd 11 ff ff    	cmpb   $0xff,-0xef(%di)
  7d:	ff 10                	call   *(%eax)
  7f:	75 b6                	jne    37 <set_video_mode+0x37>
  81:	67 80 bd 13 ff ff    	cmpb   $0xff,-0xed(%di)
  87:	ff 06                	incl   (%esi)
  89:	75 ac                	jne    37 <set_video_mode+0x37>
  8b:	67 f6 85 f8 fe ff    	testb  $0xff,-0x108(%di)
  91:	ff 80 74 a2 66 bb    	incl   -0x44995d8c(%eax)
  97:	11 41 00             	adc    %eax,0x0(%ecx)
  9a:	00 b0 02 b4 4f cd    	add    %dh,-0x32b04bfe(%eax)
  a0:	10 67 66             	adc    %ah,0x66(%edi)
  a3:	8b 85 20 ff ff ff    	mov    -0xe0(%ebp),%eax
  a9:	66 a3 f0 0f 66 81    	mov    %ax,0x81660ff0
  af:	c4 00                	les    (%eax),%eax
  b1:	02 00                	add    (%eax),%al
  b3:	00 66 5b             	add    %ah,0x5b(%esi)
  b6:	66 5f                	pop    %di
  b8:	66 5d                	pop    %bp
  ba:	66 c3                	retw   

000000bc <screen320x200>:
  bc:	66 55                	push   %bp
  be:	66 89 e5             	mov    %sp,%bp
  c1:	f4                   	hlt    
  c2:	66 5d                	pop    %bp
  c4:	66 c3                	retw   
