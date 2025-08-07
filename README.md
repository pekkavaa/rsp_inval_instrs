# rsp_inval_instrs

*A continuation of [sauraen/rsp_inval_instrs](https://github.com/sauraen/rsp_inval_instrs).*

N64 RSP invalid instructions test; checks the behavior of various invalid
instructions on real N64 hardware. libdragon based, does not contain or depend
on any Nintendo code, assets, libraries, etc.

# Summary

Invalid instructions on the RSP were previously thought to be no-ops. This was
because the testing of them had left the bits in these instructions which would
have been unused in the MIPS context as all zeros. This caused the destination
register rd to be $zero, which discards writes, leading to the observed no-op
behavior. In reality, if the rd bits are set to other values, architectural
state change is observed (i.e. the instructions do things).

<!-- Uses self-modifying RSP code, which is not typically done due to the full separation of code and data in the RSP architecture. -->

# Findings

## SPECIAL

**All** invalid instructions in the SPECIAL group (major op 00, all minor ops
which are not valid instructions--both instructions which normally exist in
MIPS and instructions which are normally invalid) behave as `srlv rd, rs, rs`.
Note that rs is used as both inputs (value being shifted and lower-5-bits shift
amount); the value of rt is ignored.

This is tested by:
- Comparing the result of the instruction-under-test to the result of the 
  reference `srlv rd, rs, rs`
- for each of 2048 random 32-bit inputs as each of rs and rt
- for each shift field value (confirming that this is ignored)
- for each unused minor opcode

Changing the register numbers for rs, rt, and rd--to confirm that the rt
register number is ignored and rs and rd are used properly--is not included, but
was done manually for a handful of values in a separate test.

## COP0


So far it looks like invalid COP0 instructions have no effect on RSP's register contents.
The instructions we are interested in look like this:

```
 COP0    <----argument bits----> <func>
010000 1 aaa aaaa aaaa aaaa aaaa bbbbbb
```
The top seven bits are constant, there are 19 argument bits and the "func" is a constant we can choose. Some values of "func" are valid instructions on the VR4300.

For emulator extension purposes, we'd like to find a safe 6-bit code for the lowest bits. A good code is one that when the 19 argument bits are varied, it produces no side effects when executed.

The test in this repo catches a simple register load (case 1), randomized register load (case 2), the earlier `tne` approach with randomized bits (case 3). Case 4 is a special case of the later COP0 tests, it has no effect as expected. The rest of the "COP0, X, 0xNN" entries are commands with COP0=b01000, CO=1 bits, function=NN and argument is randomized "X". 

    [ id]            name      instr fail/total  diffs
    [  0]   li $0, 0x8888 0x34008888    0/   1     0
    [  1]   li $1, 0x8888 0x34018888    1/   1     2
    [  2]   li $X, 0xFFFF 0x3400ffff  966/  1000  3786
    [  3]     tne a3,a3,X 0x00e77cb6  1000/ 1000  4000
    [  4]            ERET 0x43bbbbbf    0/   1     0
    [  5]   COP0, X, 0x00 0x42000000    0/ 1000    0
    [  6]   COP0, X, 0x01 0x42000001    0/ 1000    0
    ...
    [ 68]   COP0, X, 0x3f 0x4200003f    0/ 1000    0


The full execution log: https://github.com/pekkavaa/rsp_inval_instrs/blob/main/logs/2025-08-06-cop0-run.txt

This test didn't observe changes in general purpose or vector registers. 
Caveats:
- I ignored three SP registers: COP0_SEMAPHORE, COP0_DP_CLOCK, and COP0_DP_PIPE_BUSY.
- Control register initial contents were not randomized.
- Vector accumulator contents was not randomized.
- Register contents are randomized just once at the beginning. They are set before testing each instruction though.