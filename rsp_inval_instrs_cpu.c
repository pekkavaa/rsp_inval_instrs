#include <libdragon.h>

DEFINE_RSP_UCODE(rsp_inval_instrs);
DEFINE_RSP_UCODE(rsp_state_loader);

#define DUMP_BYTES (3*4)
static uint64_t dmem_scratch_space[0x100];

#define COP0_DMA_SPADDR         0
#define COP0_DMA_RAMADDR        1
#define COP0_DMA_READ           2
#define COP0_DMA_WRITE          3
#define COP0_SP_STATUS          4
#define COP0_DMA_FULL           5
#define COP0_DMA_BUSY           6
#define COP0_SEMAPHORE          7
#define COP0_DP_START           8
#define COP0_DP_END             9
#define COP0_DP_CURRENT         10
#define COP0_DP_STATUS          11
#define COP0_DP_CLOCK           12
#define COP0_DP_BUSY            13
#define COP0_DP_PIPE_BUSY       14
#define COP0_DP_TMEM_BUSY       15

#define COP2_CTRL_VCO           0
#define COP2_CTRL_VCC           1
#define COP2_CTRL_VCE           2

#define SENTINEL_INSTRUCTION (0x24001234)  // li $0, 0x1234
static int sentinel_offset = -1;

void set_test_instruction(uint32_t instr) {
    *((uint32_t*)&rsp_inval_instrs.code[sentinel_offset]) = instr;
    int codebytes = (rsp_inval_instrs.code_end - (void*)rsp_inval_instrs.code);
    data_cache_hit_writeback_invalidate(rsp_inval_instrs.code, (codebytes+15)&~0xf);
    rsp_load_code(rsp_inval_instrs.code, codebytes, 0);
}

static uint32_t rand_state = 1;
static uint32_t myrand(void) {
	uint32_t x = rand_state;
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 5;
	return rand_state = x;
}

// RANDN(n): generate a random number from 0 to n-1
#define RANDN(n) ({ \
	__builtin_constant_p((n)) ? \
		(myrand()%(n)) : \
		(uint32_t)(((uint64_t)myrand() * (n)) >> 32); \
})

static uint8_t* garbage = NULL;

void randomize_garbage(uint32_t seed) {
    if (!garbage) {
        garbage = malloc_uncached_aligned(16,4096);
    }
    rand_state = seed;
    uint32_t* dest = (uint32_t*)garbage;
    for (int i=0;i<1024;i++) {
        *dest = myrand();
        dest++;
    }
}

void set_rsp_regs_to_garbage() {
    rsp_load(&rsp_state_loader);
    rsp_load_data(garbage, 4096, 0);
    rsp_run_async();
    rsp_wait();
}

void find_sentinel_offset() {
    rsp_load(&rsp_inval_instrs);
    int codebytes = (rsp_inval_instrs.code_end - (void*)rsp_inval_instrs.code);
    // debugf("RSP IMEM:\n");
    // debug_hexdump(rsp_inval_instrs.code, codebytes);

    uint32_t* code=(uint32_t*)rsp_inval_instrs.code;
    int offset=0;
    sentinel_offset = -1;
    while (code < (uint32_t*)rsp_inval_instrs.code_end) {
        if (*code == SENTINEL_INSTRUCTION) {
            // sentinel_offset = rsp_inval_instrs.data - (uint8_t*)code;
            sentinel_offset = offset;
            break;
        }
        offset+=4;
        code++;
    }
    assert(sentinel_offset > -1);
    //debugf("Found sentinel at offset 0x%04x\n", sentinel_offset);
}

uint32_t make_cop0_instr(uint32_t function, uint32_t arg) {
    uint32_t instr = 0x42000000; // "COP0", "CO=1"
    instr |= (function & 0x3f);
    instr |= (arg & 0x7ffff) << 6;
    return instr;
}

int main(void)
{
    debug_init_isviewer();
    debug_init_usblog();
    rsp_init();
    debugf("Build: " __TIMESTAMP__ "\n");

    randomize_garbage(123);
    find_sentinel_offset();

    const uint32_t MASK_NONE = 0;
    const uint32_t MASK_COP0_ARG = 0x1FFFFC0;
    const uint32_t MASK_MOV_REG = 0x00FF0000;
    const uint32_t MASK_TNE_ARG = 0x0000001F;


    struct TestCase {
        uint32_t instr;
        uint32_t mask;
        const char* name;
    } cases[] = {
        {0x34008888, MASK_NONE, "li $0, 0x8888"},
        {0x34018888, MASK_NONE, "li $1, 0x8888"},
        {0x3400FFFF, MASK_MOV_REG, "li $X, 0xFFFF"},
        {0x00e77cb6, MASK_TNE_ARG, "tne a3,a3,X"},
        {make_cop0_instr(0xffffffff, 0xeeeeeeee), MASK_NONE, "ERET"},
        {make_cop0_instr(/*func=*/ 0x00, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x00"},
        {make_cop0_instr(/*func=*/ 0x01, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x01"},
        {make_cop0_instr(/*func=*/ 0x02, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x02"},
        {make_cop0_instr(/*func=*/ 0x03, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x03"},
        {make_cop0_instr(/*func=*/ 0x04, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x04"},
        {make_cop0_instr(/*func=*/ 0x05, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x05"},
        {make_cop0_instr(/*func=*/ 0x06, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x06"},
        {make_cop0_instr(/*func=*/ 0x07, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x07"},
        {make_cop0_instr(/*func=*/ 0x08, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x08"},
        {make_cop0_instr(/*func=*/ 0x09, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x09"},
        {make_cop0_instr(/*func=*/ 0x0a, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x0a"},
        {make_cop0_instr(/*func=*/ 0x0b, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x0b"},
        {make_cop0_instr(/*func=*/ 0x0c, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x0c"},
        {make_cop0_instr(/*func=*/ 0x0d, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x0d"},
        {make_cop0_instr(/*func=*/ 0x0e, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x0e"},
        {make_cop0_instr(/*func=*/ 0x0f, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x0f"},
        {make_cop0_instr(/*func=*/ 0x10, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x10"},
        {make_cop0_instr(/*func=*/ 0x11, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x11"},
        {make_cop0_instr(/*func=*/ 0x12, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x12"},
        {make_cop0_instr(/*func=*/ 0x13, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x13"},
        {make_cop0_instr(/*func=*/ 0x14, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x14"},
        {make_cop0_instr(/*func=*/ 0x15, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x15"},
        {make_cop0_instr(/*func=*/ 0x16, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x16"},
        {make_cop0_instr(/*func=*/ 0x17, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x17"},
        {make_cop0_instr(/*func=*/ 0x18, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x18"},
        {make_cop0_instr(/*func=*/ 0x19, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x19"},
        {make_cop0_instr(/*func=*/ 0x1a, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x1a"},
        {make_cop0_instr(/*func=*/ 0x1b, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x1b"},
        {make_cop0_instr(/*func=*/ 0x1c, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x1c"},
        {make_cop0_instr(/*func=*/ 0x1d, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x1d"},
        {make_cop0_instr(/*func=*/ 0x1e, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x1e"},
        {make_cop0_instr(/*func=*/ 0x1f, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x1f"},
        {make_cop0_instr(/*func=*/ 0x20, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x20"},
        {make_cop0_instr(/*func=*/ 0x21, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x21"},
        {make_cop0_instr(/*func=*/ 0x22, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x22"},
        {make_cop0_instr(/*func=*/ 0x23, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x23"},
        {make_cop0_instr(/*func=*/ 0x24, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x24"},
        {make_cop0_instr(/*func=*/ 0x25, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x25"},
        {make_cop0_instr(/*func=*/ 0x26, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x26"},
        {make_cop0_instr(/*func=*/ 0x27, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x27"},
        {make_cop0_instr(/*func=*/ 0x28, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x28"},
        {make_cop0_instr(/*func=*/ 0x29, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x29"},
        {make_cop0_instr(/*func=*/ 0x2a, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x2a"},
        {make_cop0_instr(/*func=*/ 0x2b, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x2b"},
        {make_cop0_instr(/*func=*/ 0x2c, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x2c"},
        {make_cop0_instr(/*func=*/ 0x2d, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x2d"},
        {make_cop0_instr(/*func=*/ 0x2e, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x2e"},
        {make_cop0_instr(/*func=*/ 0x2f, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x2f"},
        {make_cop0_instr(/*func=*/ 0x30, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x30"},
        {make_cop0_instr(/*func=*/ 0x31, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x31"},
        {make_cop0_instr(/*func=*/ 0x32, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x32"},
        {make_cop0_instr(/*func=*/ 0x33, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x33"},
        {make_cop0_instr(/*func=*/ 0x34, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x34"},
        {make_cop0_instr(/*func=*/ 0x35, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x35"},
        {make_cop0_instr(/*func=*/ 0x36, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x36"},
        {make_cop0_instr(/*func=*/ 0x37, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x37"},
        {make_cop0_instr(/*func=*/ 0x38, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x38"},
        {make_cop0_instr(/*func=*/ 0x39, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x39"},
        {make_cop0_instr(/*func=*/ 0x3a, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x3a"},
        {make_cop0_instr(/*func=*/ 0x3b, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x3b"},
        {make_cop0_instr(/*func=*/ 0x3c, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x3c"},
        {make_cop0_instr(/*func=*/ 0x3d, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x3d"},
        {make_cop0_instr(/*func=*/ 0x3e, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x3e"},
        {make_cop0_instr(/*func=*/ 0x3f, /*arg=*/ 0), MASK_COP0_ARG, "COP0 0x3f"},
    };

    const int numCases = sizeof(cases)/sizeof(cases[0]);
    struct Result {
        int numDiffs;
        int itersFailed;
        int itersRun;
    } results[numCases] = {};

    const int MAX_ITER = 100;

    for (int caseIdx=0;caseIdx<numCases;caseIdx++) {
        debugf("[% 2d] \"%s\", instr: 0x%08lx\n", caseIdx, cases[caseIdx].name, cases[caseIdx].instr);
        for (int iter=0;iter<MAX_ITER;iter++) {
            set_rsp_regs_to_garbage();
            rsp_load(&rsp_inval_instrs);
            uint32_t test_instr = cases[caseIdx].instr;
            uint32_t mask = cases[caseIdx].mask;
            if (mask != MASK_NONE) {
                test_instr = (test_instr & ~mask) | (myrand() & mask);
            }
            // debugf("% 4d \%s: %08lx\n", iter, cases[caseIdx].name, test_instr);
            set_test_instruction(test_instr);

            rsp_run_async();
            uint32_t status = 0;
            int ms = 0;
            for(; ms<10000; ++ms) {
                wait_ms(1);
                status = *SP_STATUS;
                if (status & (SP_STATUS_HALTED | SP_STATUS_BROKE | SP_STATUS_SIG2 | SP_STATUS_SIG3)) break;
            }

            rsp_snapshot_t before __attribute__((aligned(8)))={};
            rsp_snapshot_t after __attribute__((aligned(8)))={};
            // align to 8 bytes

            rsp_read_data(&before, 764, 0);
            rsp_read_data(&after, 764, 1024);

            //memset(after.dmem, 0, 4096);
            rsp_read_data(&after.dmem, 4096, 0);

            // typedef struct {
            //     uint32_t gpr[32];           ///< General purpose registers
            //     uint16_t vpr[32][8];        ///< Vector registers
            //     uint16_t vaccum[3][8];      ///< Vector accumulator
            //     uint32_t cop0[16];          ///< COP0 registers (note: reg 4 is SP_STATUS)
            //     uint32_t cop2[3];           ///< COP2 control registers
            //     uint32_t pc;                ///< Program counter
            //     uint8_t dmem[4096] __attribute__((aligned(8)));  ///< Contents of DMEM
            //     uint8_t imem[4096] __attribute__((aligned(8)));  ///< Contents of IMEM
            // } rsp_snapshot_t;

            // debugf("DMEM:\n");
            // debug_hexdump(after.dmem, 100);
            int diffs=0;
            int ignored=0;
            int cop0diffs = 0;
            for (int i=0;i<offsetof(rsp_snapshot_t, pc);i++) {
                uint8_t* a = (uint8_t*)&before;
                uint8_t* b = (uint8_t*)&after;
                if (a[i] != b[i]) {
                    bool ignore=false;
                    // debugf("[0x%04x] before=%02x vs after=%02x. DMEM[word=%d]\n", i,a[i],b[i],i/4);
                    if (i < offsetof(rsp_snapshot_t, vpr)) {
                        int idx = i/4;
                        debugf("gpr %d\n", idx);
                    } else if (i < offsetof(rsp_snapshot_t, vaccum)) {
                        int idx = (i - offsetof(rsp_snapshot_t, vpr))/16;
                        debugf("vpr %d\n", idx);
                    }else if (i < offsetof(rsp_snapshot_t, cop0)) {
                        int idx = (i - offsetof(rsp_snapshot_t, vaccum))/16;
                        debugf("vaccum %d\n", idx);
                    } else if ( i < offsetof(rsp_snapshot_t, cop2)) {
                        int idx = (i - offsetof(rsp_snapshot_t, cop0))/4;
                        if (idx == COP0_SEMAPHORE || idx == COP0_DP_CLOCK || idx == COP0_DP_PIPE_BUSY) {
                            ignore=true;
                            ignored++;
                        } else {
                            debugf("cop0 %d\n", idx);
                            cop0diffs++;
                        }
                        
                    } else if ( i < offsetof(rsp_snapshot_t, pc)) {
                        int idx = (i - offsetof(rsp_snapshot_t, cop2))/4;
                        debugf("cop2 %d\n", idx);
                    }

                    if (!ignore) {
                        diffs++;
                    }
                }
            }

            if (cop0diffs > 0) { 
                debugf("  before vs after\n");
                const char* cop0names[] = {
                    "COP0_DMA_SPADDR",
                    "COP0_DMA_RAMADDR",
                    "COP0_DMA_READ",
                    "COP0_DMA_WRITE",
                    "COP0_SP_STATUS",
                    "COP0_DMA_FULL",
                    "COP0_DMA_BUSY",
                    "COP0_SEMAPHORE",
                    "COP0_DP_START",
                    "COP0_DP_END",
                    "COP0_DP_CURRENT",
                    "COP0_DP_STATUS",
                    "COP0_DP_CLOCK",
                    "COP0_DP_BUSY",
                    "COP0_DP_PIPE_BUSY",
                    "COP0_DP_TMEM_BUSY"
                };

                for (int j=0;j<16;j++) {
                    debugf("[%18s (%02d)] 0x%08lx 0x%08lx%s\n", cop0names[j], j, before.cop0[j], after.cop0[j], (before.cop0[j] != after.cop0[j]) ? " <--" : "");
                }
            }

            //results[caseIdx] = (struct Result){.numDiffs = diffs};
            results[caseIdx].numDiffs += diffs; // = (struct Result){.numDiffs = diffs};
            results[caseIdx].itersFailed += (diffs > 0);
            results[caseIdx].itersRun++;
            if (diffs > 0) {
                debugf("Found %d diffs (not including %d ignored)\n", diffs,ignored);
                debugf("test_instr=0x%08lx", test_instr);
            }

            // debugf("Before:\n");
            // debug_hexdump(&before, 200);
            // debugf("After:\n");
            // debug_hexdump(&after, 200);

            if (false) {
                if((status & SP_STATUS_SIG2)){
                    debugf("Test passed");
                }else if((status & SP_STATUS_SIG3)){
                    debugf("Test failed");
                }else{
                    debugf("Test timed out");
                }
                debugf(" after %d ms\n", ms);
            }

            if (mask == MASK_NONE) {
                break; // don't run multiple iterations if we don't randomize
            }
        }
    }

    debugf("\n");

    debugf("[%3s] %15s %10s %10s %2s\n", "id", "name", "instr", "fail/total", "diffs");
    for (int caseIdx=0;caseIdx<numCases;caseIdx++) {
        debugf("[% 3d] %15s 0x%08lx % 4d/% 4d % 2d\n", caseIdx, cases[caseIdx].name, cases[caseIdx].instr,
            results[caseIdx].itersFailed,
            results[caseIdx].itersRun,
            results[caseIdx].numDiffs);
    }

    debugf("\nTests done. Triggering a crash...\n");
    rsp_crash(); // just show something on screen
}
