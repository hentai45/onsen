
src/asmhead.o:     file format elf32-i386


Disassembly of section .text:

00000000 <old_set_video_mode-0x24>:
   0:	66 e8 fc ff          	callw  0 <old_set_video_mode-0x24>
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
  14:	be 00 00 00 66       	mov    $0x66000000,%esi
  19:	e8 fc ff ff ff       	call   1a <old_set_video_mode-0xa>
  1e:	66 e8 fc ff          	callw  1e <old_set_video_mode-0x6>
  22:	ff                   	(bad)  
  23:	ff                   	(bad)  

00000024 <old_set_video_mode>:
  24:	b8 00 90 8e c0       	mov    $0xc08e9000,%eax
  29:	bf 00 00 b8 00       	mov    $0xb80000,%edi
  2e:	4f                   	dec    %edi
  2f:	cd 10                	int    $0x10
  31:	83 f8 4f             	cmp    $0x4f,%eax
  34:	75 3f                	jne    75 <error_vbe>
  36:	26 8b 45 04          	mov    %es:0x4(%ebp),%eax
  3a:	3d 00 02 72 36       	cmp    $0x36720200,%eax
  3f:	b9 11 01 b8 01       	mov    $0x1b80111,%ecx
  44:	4f                   	dec    %edi
  45:	cd 10                	int    $0x10
  47:	83 f8 4f             	cmp    $0x4f,%eax
  4a:	75 29                	jne    75 <error_vbe>
  4c:	26 80 7d 19 10       	cmpb   $0x10,%es:0x19(%ebp)
  51:	75 22                	jne    75 <error_vbe>
  53:	26 80 7d 1b 06       	cmpb   $0x6,%es:0x1b(%ebp)
  58:	75 1b                	jne    75 <error_vbe>
  5a:	26 8b 05 25 80 00 74 	mov    %es:0x74008025,%eax
  61:	13 bb 11 41 b8 02    	adc    0x2b84111(%ebx),%edi
  67:	4f                   	dec    %edi
  68:	cd 10                	int    $0x10
  6a:	26 66 8b 45 28       	mov    %es:0x28(%ebp),%ax
  6f:	66 a3 f0 0f 66 c3    	mov    %ax,0xc3660ff0

00000075 <error_vbe>:
  75:	be 81 00 66 e8       	mov    $0xe8660081,%esi
  7a:	1e                   	push   %ds
  7b:	00 00                	add    %al,(%eax)
	...

0000007e <error_fin>:
  7e:	f4                   	hlt    
  7f:	eb fd                	jmp    7e <error_fin>

00000081 <vbe_err_msg>:
  81:	0a 73 65             	or     0x65(%ebx),%dh
  84:	74 74                	je     fa <flush_pipeline+0xc>
  86:	69 6e 67 20 76 69 64 	imul   $0x64697620,0x67(%esi),%ebp
  8d:	65 6f                	outsl  %gs:(%esi),(%dx)
  8f:	20 6d 6f             	and    %ch,0x6f(%ebp)
  92:	64 65 20 65 72       	fs and %ah,%fs:%gs:0x72(%ebp)
  97:	72 6f                	jb     108 <old_move_disk_data+0xb>
  99:	72 0a                	jb     a5 <print+0x9>
	...

0000009c <print>:
  9c:	8a 04 83             	mov    (%ebx,%eax,4),%al
  9f:	c6 01 3c             	movb   $0x3c,(%ecx)
  a2:	00 74 09 b4          	add    %dh,-0x4c(%ecx,%ecx,1)
  a6:	0e                   	push   %cs
  a7:	bb 0f 00 cd 10       	mov    $0x10cd000f,%ebx
  ac:	eb ee                	jmp    9c <print>

000000ae <print_fin>:
  ae:	66 c3                	retw   

000000b0 <old_disable_interrupts>:
  b0:	b0 ff                	mov    $0xff,%al
  b2:	e6 21                	out    %al,$0x21
  b4:	90                   	nop
  b5:	e6 a1                	out    %al,$0xa1
  b7:	fa                   	cli    
  b8:	66 c3                	retw   

000000ba <old_enable_a20>:
  ba:	66 e8 ce 00          	callw  18c <skip+0xd>
  be:	00 00                	add    %al,(%eax)
  c0:	b0 d1                	mov    $0xd1,%al
  c2:	e6 64                	out    %al,$0x64
  c4:	66 e8 c4 00          	callw  18c <skip+0xd>
  c8:	00 00                	add    %al,(%eax)
  ca:	b0 df                	mov    $0xdf,%al
  cc:	e6 60                	out    %al,$0x60
  ce:	66 e8 ba 00          	callw  18c <skip+0xd>
  d2:	00 00                	add    %al,(%eax)
  d4:	66 c3                	retw   

000000d6 <to_protect_mode>:
  d6:	66 0f 01 16          	lgdtw  (%esi)
  da:	ca 01 0f             	lret   $0xf01
  dd:	20 c0                	and    %al,%al
  df:	66 25 ff ff          	and    $0xffff,%ax
  e3:	ff                   	(bad)  
  e4:	7f 66                	jg     14c <old_move_and_run_onsensys+0xd>
  e6:	83 c8 01             	or     $0x1,%eax
  e9:	0f 22 c0             	mov    %eax,%cr0
  ec:	eb 00                	jmp    ee <flush_pipeline>

000000ee <flush_pipeline>:
  ee:	b8 08 00 8e d8       	mov    $0xd88e0008,%eax
  f3:	8e c0                	mov    %eax,%es
  f5:	8e e0                	mov    %eax,%fs
  f7:	8e e8                	mov    %eax,%gs
  f9:	8e d0                	mov    %eax,%ss
  fb:	66 c3                	retw   

000000fd <old_move_disk_data>:
  fd:	66 be 00 7c          	mov    $0x7c00,%si
 101:	00 00                	add    %al,(%eax)
 103:	66 bf 00 00          	mov    $0x0,%di
 107:	10 00                	adc    %al,(%eax)
 109:	66 b9 80 00          	mov    $0x80,%cx
 10d:	00 00                	add    %al,(%eax)
 10f:	66 e8 81 00          	callw  194 <wait_kb_out+0x6>
 113:	00 00                	add    %al,(%eax)
 115:	66 be 00 82          	mov    $0x8200,%si
 119:	00 00                	add    %al,(%eax)
 11b:	66 bf 00 02          	mov    $0x200,%di
 11f:	10 00                	adc    %al,(%eax)
 121:	66 b9 00 00          	mov    $0x0,%cx
 125:	00 00                	add    %al,(%eax)
 127:	b1 20                	mov    $0x20,%cl
 129:	66 69 c9 00 12       	imul   $0x1200,%cx,%cx
 12e:	00 00                	add    %al,(%eax)
 130:	66 81 e9 80 00       	sub    $0x80,%cx
 135:	00 00                	add    %al,(%eax)
 137:	66 e8 59 00          	callw  194 <wait_kb_out+0x6>
 13b:	00 00                	add    %al,(%eax)
 13d:	66 c3                	retw   

0000013f <old_move_and_run_onsensys>:
 13f:	66 be 00 00          	mov    $0x0,%si
 143:	00 00                	add    %al,(%eax)
 145:	66 bf 00 00          	mov    $0x0,%di
 149:	28 00                	sub    %al,(%eax)
 14b:	66 b9 00 00          	mov    $0x0,%cx
 14f:	02 00                	add    (%eax),%al
 151:	66 e8 3f 00          	callw  194 <wait_kb_out+0x6>
 155:	00 00                	add    %al,(%eax)
 157:	66 bb 00 00          	mov    $0x0,%bx
 15b:	28 00                	sub    %al,(%eax)
 15d:	67 66 8b 4b 10       	mov    0x10(%bp,%di),%cx
 162:	66 83 c1 03          	add    $0x3,%cx
 166:	66 c1 e9 02          	shr    $0x2,%cx
 16a:	74 13                	je     17f <skip>
 16c:	67 66 8b 73 14       	mov    0x14(%bp,%di),%si
 171:	66 01 de             	add    %bx,%si
 174:	67 66 8b 7b 0c       	mov    0xc(%bp,%di),%di
 179:	66 e8 17 00          	callw  194 <wait_kb_out+0x6>
	...

0000017f <skip>:
 17f:	67 66 8b 63 0c       	mov    0xc(%bp,%di),%sp
 184:	66 ea 1b 00 28 00    	ljmpw  $0x28,$0x1b
 18a:	10 00                	adc    %al,(%eax)
 18c:	66 c3                	retw   

0000018e <wait_kb_out>:
 18e:	e4 64                	in     $0x64,%al
 190:	24 02                	and    $0x2,%al
 192:	75 fa                	jne    18e <wait_kb_out>
 194:	66 c3                	retw   

00000196 <copy_memory>:
 196:	67 66 8b 06 66 83    	mov    -0x7c9a,%ax
 19c:	c6 04 67 66          	movb   $0x66,(%edi,%eiz,2)
 1a0:	89 07                	mov    %eax,(%edi)
 1a2:	66 83 c7 04          	add    $0x4,%di
 1a6:	66 83 e9 01          	sub    $0x1,%cx
 1aa:	75 ea                	jne    196 <copy_memory>
 1ac:	66 c3                	retw   
 1ae:	66 90                	xchg   %ax,%ax

000001b0 <GDT0>:
	...
 1b8:	ff                   	(bad)  
 1b9:	ff 00                	incl   (%eax)
 1bb:	00 00                	add    %al,(%eax)
 1bd:	92                   	xchg   %eax,%edx
 1be:	cf                   	iret   
 1bf:	00 ff                	add    %bh,%bh
 1c1:	ff 00                	incl   (%eax)
 1c3:	00 00                	add    %al,(%eax)
 1c5:	9a cf 00 00 00 17 00 	lcall  $0x17,$0xcf

000001ca <GDTR0>:
 1ca:	17                   	pop    %ss
 1cb:	00                   	.byte 0x0
 1cc:	b0 01                	mov    $0x1,%al
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

00000000 <disable_interrupts>:
   0:	66 55                	push   %bp
   2:	b0 ff                	mov    $0xff,%al
   4:	66 89 e5             	mov    %sp,%bp
   7:	e6 21                	out    %al,$0x21
   9:	90                   	nop
   a:	e6 a1                	out    %al,$0xa1
   c:	fa                   	cli    
   d:	66 5d                	pop    %bp
   f:	66 c3                	retw   

00000011 <enable_a20>:
  11:	66 55                	push   %bp
  13:	66 89 e5             	mov    %sp,%bp
  16:	66 83 ec 08          	sub    $0x8,%sp
  1a:	66 e8 fc ff          	callw  1a <enable_a20+0x9>
  1e:	ff                   	(bad)  
  1f:	ff b0 d1 e6 64 66    	pushl  0x6664e6d1(%eax)
  25:	e8 fc ff ff ff       	call   26 <enable_a20+0x15>
  2a:	b0 df                	mov    $0xdf,%al
  2c:	e6 60                	out    %al,$0x60
  2e:	66 c9                	leavew 
  30:	e9 fe ff 66 55       	jmp    55670033 <memcopy+0x5566ff6d>

00000033 <wait_kbc_sendready>:
  33:	66 55                	push   %bp
  35:	66 89 e5             	mov    %sp,%bp
  38:	e4 64                	in     $0x64,%al
  3a:	a8 02                	test   $0x2,%al
  3c:	75 fa                	jne    38 <wait_kbc_sendready+0x5>
  3e:	66 5d                	pop    %bp
  40:	66 c3                	retw   

00000042 <move_disk_data>:
  42:	66 55                	push   %bp
  44:	66 b9 00 02          	mov    $0x200,%cx
  48:	00 00                	add    %al,(%eax)
  4a:	66 89 e5             	mov    %sp,%bp
  4d:	66 ba 00 7c          	mov    $0x7c00,%dx
  51:	00 00                	add    %al,(%eax)
  53:	66 83 ec 08          	sub    $0x8,%sp
  57:	66 b8 00 00          	mov    $0x0,%ax
  5b:	10 00                	adc    %al,(%eax)
  5d:	66 e8 63 00          	callw  c4 <move_and_run_onsensys+0x4b>
  61:	00 00                	add    %al,(%eax)
  63:	66 b9 00 f8          	mov    $0xf800,%cx
  67:	0a 00                	or     (%eax),%al
  69:	66 ba 00 82          	mov    $0x8200,%dx
  6d:	00 00                	add    %al,(%eax)
  6f:	66 b8 00 02          	mov    $0x200,%ax
  73:	10 00                	adc    %al,(%eax)
  75:	66 c9                	leavew 
  77:	eb 4d                	jmp    c6 <memcopy>

00000079 <move_and_run_onsensys>:
  79:	66 55                	push   %bp
  7b:	66 b9 00 00          	mov    $0x0,%cx
  7f:	08 00                	or     %al,(%eax)
  81:	66 89 e5             	mov    %sp,%bp
  84:	66 ba 00 00          	mov    $0x0,%dx
  88:	00 00                	add    %al,(%eax)
  8a:	66 83 ec 08          	sub    $0x8,%sp
  8e:	66 b8 00 00          	mov    $0x0,%ax
  92:	28 00                	sub    %al,(%eax)
  94:	66 e8 2c 00          	callw  c4 <move_and_run_onsensys+0x4b>
  98:	00 00                	add    %al,(%eax)
  9a:	66 8b 0e             	mov    (%esi),%cx
  9d:	10 00                	adc    %al,(%eax)
  9f:	66 85 c9             	test   %cx,%cx
  a2:	74 0f                	je     b3 <move_and_run_onsensys+0x3a>
  a4:	66 8b 16             	mov    (%esi),%dx
  a7:	14 00                	adc    $0x0,%al
  a9:	66 a1 0c 00 66 e8    	mov    0xe866000c,%ax
  af:	13 00                	adc    (%eax),%eax
  b1:	00 00                	add    %al,(%eax)
  b3:	66 a1 0c 00 66 89    	mov    0x8966000c,%ax
  b9:	c4 66 ea             	les    -0x16(%esi),%esp
  bc:	1b 00                	sbb    (%eax),%eax
  be:	28 00                	sub    %al,(%eax)
  c0:	10 00                	adc    %al,(%eax)
  c2:	66 c9                	leavew 
  c4:	66 c3                	retw   

000000c6 <memcopy>:
  c6:	66 55                	push   %bp
  c8:	66 89 e5             	mov    %sp,%bp
  cb:	66 56                	push   %si
  cd:	66 89 ce             	mov    %cx,%si
  d0:	66 53                	push   %bx
  d2:	66 31 db             	xor    %bx,%bx
  d5:	66 39 f3             	cmp    %si,%bx
  d8:	7d 0c                	jge    e6 <memcopy+0x20>
  da:	67 8a 0c             	mov    (%si),%cl
  dd:	1a 67 88             	sbb    -0x78(%edi),%ah
  e0:	0c 18                	or     $0x18,%al
  e2:	66 43                	inc    %bx
  e4:	eb ef                	jmp    d5 <memcopy+0xf>
  e6:	66 5b                	pop    %bx
  e8:	66 5e                	pop    %si
  ea:	66 5d                	pop    %bp
  ec:	66 c3                	retw   

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
