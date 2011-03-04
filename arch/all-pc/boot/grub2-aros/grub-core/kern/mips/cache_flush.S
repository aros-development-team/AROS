	move $t2, $a0
	addu $t3, $a0, $a1
	srl $t2, $t2, 5
	sll $t2, $t2, 5
	addu $t3, $t3, 0x1f
	srl $t3, $t3, 5
	sll $t3, $t3, 5
	move $t0, $t2
	subu $t1, $t3, $t2
1:
	cache 1, 0($t0)
	addiu $t1, $t1, 0xffff
	bne $t1, $zero, 1b
	 addiu $t0, $t0, 0x1
	sync
	move $t0, $t2
	subu $t1, $t3, $t2
2:
	cache 0, 0($t0)
	addiu $t1, $t1, 0xffff
	bne $t1, $zero, 2b
	 addiu $t0, $t0, 0x1
	sync
