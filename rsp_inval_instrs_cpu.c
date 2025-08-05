#include <libdragon.h>

DEFINE_RSP_UCODE(rsp_inval_instrs);

#define DUMP_BYTES (3*4)
static uint64_t dmem_scratch_space[0x100];

int main(void)
{
    debug_init_isviewer();
    debug_init_usblog();
    rsp_init();

    rsp_load(&rsp_inval_instrs);
    SP_DMEM[0] = (uint32_t)(&dmem_scratch_space[0]);

    debugf("Build: " __TIMESTAMP__ "\n");
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
    int cop0diffs = 0;
    for (int i=0;i<offsetof(rsp_snapshot_t, pc);i++) {
        uint8_t* a = (uint8_t*)&before;
        uint8_t* b = (uint8_t*)&after;
        if (a[i] != b[i]) {
            debugf("[0x%04x] before=%02x vs after=%02x. DMEM[word=%d]\n", i,a[i],b[i],i/4);
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
                debugf("cop0 %d\n", idx);
                cop0diffs++;

                
                
            } else if ( i < offsetof(rsp_snapshot_t, pc)) {
                int idx = (i - offsetof(rsp_snapshot_t, cop2))/4;
                debugf("cop2 %d\n", idx);
            }
            diffs++;
        }
    }

    if (cop0diffs > 0) { 
                debugf("  before vs after\n");
                // print before vs after cop0 registers in their own two columns
                // before.cop0 and after.cop0
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

    if (diffs > 0) {
        debugf("Found %d diffs\n",diffs);
    }

    debugf("Before:\n");
    debug_hexdump(&before, 200);
    debugf("After:\n");
    debug_hexdump(&after, 200);

    if((status & SP_STATUS_SIG2)){
        debugf("Test passed");
    }else if((status & SP_STATUS_SIG3)){
        debugf("Test failed");
    }else{
        debugf("Test timed out");
    }
    debugf(" after %d ms\n", ms);
    debugf("Done\n");
    //free(dmem);

    //rsp_crash();
}
