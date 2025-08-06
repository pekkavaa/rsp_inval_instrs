#include <rsp.inc>

.set noreorder
.set noat

.text
	lw $0,  0*4(zero)
	lw $1,  1*4(zero)
	lw $2,  2*4(zero)
	lw $3,  3*4(zero)
	lw $4,  4*4(zero)
	lw $5,  5*4(zero)
	lw $6,  6*4(zero)
	lw $7,  7*4(zero)
	lw $8,  8*4(zero)
	lw $9,  9*4(zero)
	lw $10, 10*4(zero)
	lw $11, 11*4(zero)
	lw $12, 12*4(zero)
	lw $13, 13*4(zero)
	lw $14, 14*4(zero)
	lw $15, 15*4(zero)
	lw $16, 16*4(zero) // s0
	lw $17, 17*4(zero)
	lw $18, 18*4(zero)
	lw $19, 19*4(zero)
	lw $20, 20*4(zero)
	lw $21, 21*4(zero)
	lw $22, 22*4(zero)
	lw $23, 23*4(zero)
	lw $24, 24*4(zero)
	lw $25, 25*4(zero)
	lw $26, 26*4(zero)
	lw $27, 27*4(zero)
	lw $28, 28*4(zero)
	lw $29, 29*4(zero)
	lw $30, 30*4(zero)
	lw $31, 31*4(zero)

	li s0, 32*4
	lqv $v00,  0*16,s0
	lqv $v01,  1*16,s0
	lqv $v02,  2*16,s0
	lqv $v03,  3*16,s0
	lqv $v04,  4*16,s0
	lqv $v05,  5*16,s0
	lqv $v06,  6*16,s0
	lqv $v07,  7*16,s0
	lqv $v08,  8*16,s0
	lqv $v09,  9*16,s0
	lqv $v10, 10*16,s0
	lqv $v11, 11*16,s0
	lqv $v12, 12*16,s0
	lqv $v13, 13*16,s0
	lqv $v14, 14*16,s0
	lqv $v15, 15*16,s0
	lqv $v16, 16*16,s0
	lqv $v17, 17*16,s0
	lqv $v18, 18*16,s0
	lqv $v19, 19*16,s0
	lqv $v20, 20*16,s0
	lqv $v21, 21*16,s0
	lqv $v22, 22*16,s0
	lqv $v23, 23*16,s0
	lqv $v24, 24*16,s0
	lqv $v25, 25*16,s0
	lqv $v26, 26*16,s0
	lqv $v27, 27*16,s0
	lqv $v28, 28*16,s0
	lqv $v29, 29*16,s0
	lqv $v30, 30*16,s0
	lqv $v31, 31*16,s0

	// FIXME: COP2 not loaded
    // vsar $v00, COP2_ACC_HI
    // vsar $v01, COP2_ACC_MD
    // vsar $v02, COP2_ACC_LO

	// sqv $v00,  32*16,s0
	// sqv $v01,  33*16,s0
	// sqv $v02,  34*16,s0

    // restore v00, v01, v02
    // lqv $v00, 0*16,s0
    // lqv $v01, 1*16,s0
    // lqv $v02, 2*16,s0

	add s0, 35*16

	// mfc0 t0, $0
	// mfc0 t1, $1
	// mfc0 t2, $2
	// mfc0 t3, $3
	// mfc0 t4, $4
	// mfc0 t5, $5
	// mfc0 t6, $6
	// mfc0 t7, $7

	// sw t0, 0*4(s0)
	// sw t1, 1*4(s0)
	// sw t2, 2*4(s0)
	// sw t3, 3*4(s0)
	// sw t4, 4*4(s0)
	// sw t5, 5*4(s0)
	// sw t6, 6*4(s0)
	// sw t7, 7*4(s0)

	// mfc0 t0, $8
	// mfc0 t1, $9
	// mfc0 t2, $10
	// mfc0 t3, $11
	// mfc0 t4, $12
	// mfc0 t5, $13
	// mfc0 t6, $14
	// mfc0 t7, $15

	// sw t0, 8*4(s0)
	// sw t1, 9*4(s0)
	// sw t2, 10*4(s0)
	// sw t3, 11*4(s0)
	// sw t4, 12*4(s0)
	// sw t5, 13*4(s0)
	// sw t6, 14*4(s0)
	// sw t7, 15*4(s0)

	// add s0, 16*4

	// cfc2 t0, $0
	// cfc2 t1, $1
	// cfc2 t2, $2

	// sw t0, 0*4(s0)
	// sw t1, 1*4(s0)
	// sw t2, 2*4(s0)

	//lw $0,  0*4(zero)
	//lw $1,  1*4(zero)
	//lw $2,  2*4(zero)
	//lw $3,  3*4(zero)
	//lw $4,  4*4(zero)
	//lw $5,  5*4(zero)
	//lw $6,  6*4(zero)
	//lw $7,  7*4(zero)

    // restore t0-t8 = $8..15
	// lw $8,  8*4(zero)
	// lw $9,  9*4(zero)
	// lw $10, 10*4(zero)
	// lw $11, 11*4(zero)
	// lw $12, 12*4(zero)
	// lw $13, 13*4(zero)
	// lw $14, 14*4(zero)
	// lw $15, 15*4(zero)
 
    // restore s0=$16
	//sw $16, 16*4(zero)

	lw $16, 16*4(zero) // s0
	
    break
halt:
    j       halt
    nop

.data
dummy:
    .word 0
    .word 0
    .word 0
    .word 0

.text
