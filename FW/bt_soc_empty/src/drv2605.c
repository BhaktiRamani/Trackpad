#include "../inc/drv2605.h"


//void init_drv2605(eLibrarySelect lib, const eWaveform *wf, uint8_t wf_size){
//    // we do not have to wait as the bootup for EFR would have taken more than 250uS
//
//    ENABLE_HAPTICS;
//
//    sStatusReg status;
//    i2c_read(DEFAULT_DRV2605_ADDRESS, STATUS_ADDR, &status.bits, 1);
//    sl_iostream_printf(SL_IOSTREAM_STDOUT, "Device id : %d \n", status.fields.deviceId);
//    EFM_ASSERT(status.fields.deviceId == 7);
//    EFM_ASSERT(status.fields.oc_detect == 0);
//    EFM_ASSERT(status.fields.over_temp == 0);
//    EFM_ASSERT(status.fields.diag_results == 0);
//
//
//    sModeReg mode = {
//        .fields = {
//            .mode = mode_INTERNAL_TRIGGER, // trigger using GO bit
//            .standby = 0, // clear to go outof standby
//        }
//    };
//    i2c_write(DEFAULT_DRV2605_ADDRESS, MODE_ADDR, (uint8_t*)&mode, sizeof(mode));
//
//    load_play_drv2605(lib, wf, wf_size, false);
//
//    DISABLE_HAPTICS;
//}
//
//void load_play_drv2605(const eLibrarySelect lib, const eWaveform *wf, uint8_t wf_size, bool play){
//    ENABLE_HAPTICS;
//
//    i2c_write(DEFAULT_DRV2605_ADDRESS, LIB_SELECTION_ADDR, (uint8_t *)&lib, 1);
//
//    if(wf_size > 8) EFM_ASSERT(false);
//
//    for (uint8_t index = 0; index < wf_size; index++){
//        if(wf->waveform > 123)  EFM_ASSERT(false);
//        i2c_write(DEFAULT_DRV2605_ADDRESS, (WAVEFORM_SEQ_QUEUE_1_ADDR + index), (uint8_t*)&wf[index], sizeof(eWaveform));
//    }
//
//    if(play){
//        uint8_t go = 1;
//        i2c_write(DEFAULT_DRV2605_ADDRESS, GO_ADDR, &go, 1);
//    }
//
//    DISABLE_HAPTICS;
//}
