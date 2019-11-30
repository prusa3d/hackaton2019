// trinamic.cpp
#include "dbg.h"
#include "config.h"
#include "hwio_a3ides.h"
#include "TMCStepper.h"

#define DBG _dbg3 //debug level 3
//#define DBG(...)  //disable debug

#if ((BOARD == A3IDES2209_REV01) || (BOARD == A3IDES2209_REV02))

extern TMC2209Stepper stepperX;
extern TMC2209Stepper stepperY;
extern TMC2209Stepper stepperZ;
extern TMC2209Stepper stepperE0;

extern "C" {

TMC2209Stepper* pStepX = 0;
TMC2209Stepper* pStepY = 0;
TMC2209Stepper* pStepZ = 0;
TMC2209Stepper* pStepE = 0;

void init_tmc(void)
{
    pStepX = &stepperX;
    pStepY = &stepperY;
    pStepZ = &stepperZ;
    pStepE = &stepperE0;
    //int i = 0;
    pStepX->TCOOLTHRS(400);
    pStepY->TCOOLTHRS(400);
    pStepZ->TCOOLTHRS(400);
    pStepE->TCOOLTHRS(400);
    pStepX->SGTHRS(100);
    pStepY->SGTHRS(100);
    pStepZ->SGTHRS(100);
    pStepE->SGTHRS(100);
}

unsigned int get_tmc_stall_extruder(void)
{
    unsigned int sgx = pStepE->SG_RESULT();

    return sgx;
}

void tmc_sample(void)
{
    unsigned int sgx = pStepX->SG_RESULT();
    //unsigned int sgy = pStepY->SG_RESULT();
    unsigned int diag = 0;
    //	diag |= hwio_di_get_val(_DI_Z_DIAG);
    //diag |= hwio_di_get_val(_DI_Y_DIAG) << 1;
    //diag |= hwio_di_get_val(_DI_Z_DIAG) << 2;
    //diag |= hwio_di_get_val(_DI_E0_DIAG) << 3;
    unsigned int tstepx = pStepX->TSTEP();
    //unsigned int tstepy = pStepX->TSTEP();
    //	int sgz = pStepZ->SG_RESULT();
    //	int sge = pStepE->SG_RESULT();
    //	_dbg("sg %d %d %d %d", sgx, sgy, sgz, sge);
    sgx = sgx; //prevent warning
    tstepx = tstepx; //prevent warning
    diag = diag; //prevent warning
    DBG("sg %u %u %u", sgx, diag, tstepx);
}

} //extern "C"

#elif (BOARD == A3IDES2130_REV01)

void init_tmc(void)
{
    //no init required
}

#else

#error "BOARD not defined"

#endif
