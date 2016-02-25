  # labwork.S
  # Written 2015 by F Lundevall
  # Skeleton file for IS1200/IS1500 lab 1 2015-2016.
  # The macros PUSH and POP are in the public domain.

.macro	PUSH reg
	addi	$sp,$sp,-4
	sw	\reg,0($sp)
.endm

.macro	POP reg
	lw	\reg,0($sp)
	addi	$sp,$sp,4
.endm

	.data
	.align 2

	.text
	.global hexasc
hexasc:
	li	$t0,9		#sätt t0 till 9
	andi	$a0,$a0,0x000f	#sätt allt utom de 
							#fyra sista bitarna till 0
	ble	$a0,$t0,number	#hoppa om mindre än eller
						#lika med 9, till number
	nop
				#tal mellan 10 och 15
	addi	$v0,$a0,0x37	#lägg till 55
	jr	$ra		#return
	nop
number:				#tal melllan 0 och 9
	addi	$v0,$a0,0x30	#lägg till 48
	jr	$ra		#return
	nop
	
	.global delay
delay:
	## while loop
	ble	$a0,$0,delay_done
	nop
	#######################
	addi	$a0,$a0,-1

	## for loop
	addi	$t0,$0,27000	# räkna upp till
	addi	$t1,$0,0	# i = 0
forloop:
	addi	$t1,$t1,1
	bne	$t1,$t0,forloop
	nop
	#######
	
	bne	$a0,$0,delay	
	nop
	#######################
delay_done:
	jr	$ra
	nop
	.global time2string
time2string:
	#	a0 = addressen till minnet
	#	a1 = nbcd
	PUSH $s0
	PUSH $s1
	PUSH $s2
	PUSH $ra
	
	move	$s0,$a0
	move	$s1,$a1
	
	# börja med att skriva NUL
	addi	$t0,$0,0x00	#
	sb	$t0,5($s0)	# skriv NUL till minnet
	
	#kolla om minuterna är 00
	andi	$t1,$s1,0x00ff
	bne	$t1,$0,not_whole_minute
	nop

	# skriv NUL på en annan plats
	# och sätt
	sb	$t0,6($s0)	# skriv NUL till minnet
	addi	$t0,$0,0x58
	sb	$t0,5($s0)
	
not_whole_minute:
	
	move	$a0,$s1		# första siffran, längst till höger	
	jal	hexasc
	nop
	sb	$v0,4($s0)
	
	srl	$s1,$s1,4	# andra siffran
	move	$a0,$s1
	jal	hexasc
	nop
	sb	$v0,3($s0)
	
	addi	$t0,$0,0x3a	#kolon
	sb	$t0,2($s0)
	
	srl	$s1,$s1,4	# tredje siffran
	move	$a0,$s1
	jal	hexasc
	nop
	sb	$v0,1($s0)
	
	srl	$s1,$s1,4	# fjärde siffran
	move	$a0,$s1
	jal	hexasc
	nop
	sb	$v0,0($s0)
	
	POP $ra
	POP $s2	
	POP $s1
	POP $s0
	jr	$ra
	nop
.global enable_interrupt
enable_interrupt:
	ei
	jr	$ra
	nop
