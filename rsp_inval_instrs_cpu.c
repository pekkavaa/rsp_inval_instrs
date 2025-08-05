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

    rsp_snapshot_t before={};
    rsp_snapshot_t after={};
    // align to 8 bytes

    rsp_read_data(&before, 764, 0);
    rsp_read_data(&after, 764, 1024);

    //memset(after.dmem, 0, 4096);
    rsp_read_data(&after.dmem, 4096, 0);

    // debugf("DMEM:\n");
    // debug_hexdump(after.dmem, 100);
    int diffs=0;
    for (int i=0;i<764;i++) {
        uint8_t* a = (uint8_t*)&before;
        uint8_t* b = (uint8_t*)&after;
        if (a[i] != b[i]) {
            debugf("[0x%04x] before=%02x vs after=%02x\n", i,a[i],b[i]);
            diffs++;
        }
    }

    if (diffs > 0) {
        debugf("Found %d diffs\n",diffs);
    }

    debugf("Before:\n");
    debug_hexdump(&before, 100);
    debugf("After:\n");
    debug_hexdump(&after, 100);

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
