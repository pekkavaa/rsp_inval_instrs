#include <libdragon.h>

DEFINE_RSP_UCODE(rsp_inval_instrs);

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

    rsp_load(&rsp_inval_instrs);
    SP_DMEM[0] = (uint32_t)(&dmem_scratch_space[0]);

    debugf("Build: " __TIMESTAMP__ "\n");
    int codebytes = (rsp_inval_instrs.code_end - (void*)rsp_inval_instrs.code);
    debugf("RSP IMEM:\n");
    debug_hexdump(rsp_inval_instrs.code, codebytes);

    uint32_t* code=(uint32_t*)rsp_inval_instrs.code;
    int offset=0;
    while (code < (uint32_t*)rsp_inval_instrs.code_end) {
        if (*code == SENTINEL_INSTRUCTION) {
            // sentinel_offset = rsp_inval_instrs.data - (uint8_t*)code;
            sentinel_offset = offset;
        }
        offset+=4;
        code++;
    }
    assert(sentinel_offset > -1);
    debugf("Found sentinel at offset 0x%04x\n", sentinel_offset);

    struct TestCase {
        uint32_t instr;
        const char* name;
    } cases[] = {
        {0x34018888, "li $1, 0x8888"},
        {0x00e77cb6, "tne a3,a3,0x1f2"},
        {0x34008888, "li $0, 0x8888"},
        {make_cop0_instr(3, 123123), "ERET"},
    };

    const int numCases = sizeof(cases)/sizeof(cases[0]);
    struct Result {
        int numDiffs;
    } results[numCases];

    for (int caseIdx=0;caseIdx<numCases;caseIdx++) {
        debugf("[% 2d] \"%s\", instr: 0x%08lx\n", caseIdx, cases[caseIdx].name, cases[caseIdx].instr);
        set_test_instruction(cases[caseIdx].instr);   // li $0, 0x8888
        // set_test_instruction(0x34018888);   // li $1, 0x8888
        // set_test_instruction(0x00e77cb6);  //tne a3,a3,0x1f2

        // debugf("RSP IMEM after:\n");
        // debug_hexdump(rsp_inval_instrs.code, codebytes);

        debugf("Testing...\n");
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

        results[caseIdx] = (struct Result){.numDiffs = diffs};
        debugf("Found %d diffs (not including %d ignored)\n",diffs,ignored);

        // debugf("Before:\n");
        // debug_hexdump(&before, 200);
        // debugf("After:\n");
        // debug_hexdump(&after, 200);

        if((status & SP_STATUS_SIG2)){
            debugf("Test passed");
        }else if((status & SP_STATUS_SIG3)){
            debugf("Test failed");
        }else{
            debugf("Test timed out");
        }
        debugf(" after %d ms\n", ms);
    }

    debugf("\n");

    debugf("[%2s] %15s %10s %2s\n", "id", "name", "instr", "diffs");
    for (int caseIdx=0;caseIdx<numCases;caseIdx++) {
        debugf("[% 2d] %15s 0x%08lx % 2d\n", caseIdx, cases[caseIdx].name, cases[caseIdx].instr, results[caseIdx].numDiffs);
    }

    //rsp_crash();
}
