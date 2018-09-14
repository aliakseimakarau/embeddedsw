/******************************************************************************
 *
 * Copyright (C) 2018 Xilinx, Inc.	All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * XILINX BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Xilinx shall not be used
 * in advertising or otherwise to promote the sale, use or other dealings in
 * this Software without prior written authorization from Xilinx.
 *
******************************************************************************/
/*****************************************************************************/
/**
 *
 * @file xv_scenechange_intr.c
 * @addtogroup v_scenechange_v1_0
 * @{
 *
 * The functions in this file provides interrupt handler and associated
 * functions.
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   Who    Date     Changes
 * ----- ---- -------- -------------------------------------------------------
 * 1.00  praveenv   13/09/18   Initial Release
 * </pre>
 *
 ******************************************************************************/

/***************************** Include Files *********************************/
#include "xil_printf.h"
#include "xv_scenechange_hw.h"
#include "xv_scenechange.h"

/*****************************************************************************/
/**
 *
 * This function installs an asynchronous callback function:
 *
 * @param    InstancePtr is a pointer to the SceneChange IP instance.
 * @param    CallbackFunc is the address of the callback function.
 * @param    CallbackRef is a user data item that will be passed to the
 *       callback function when it is invoked.
 *
 * @return	None.
 *
 * @note     Invoking this function for a handler that already has been
 *       installed replaces it with the new handler.
 *
 ******************************************************************************/
void XV_scenechange_SetCallback(XV_scenechange *InstancePtr,
				       void *CallbackFunc, void *CallbackRef)
{
	/* Verify arguments. */
	Xil_AssertVoid(InstancePtr != NULL);
	Xil_AssertVoid(CallbackFunc != NULL);
	Xil_AssertVoid(CallbackRef != NULL);

	InstancePtr->FrameDoneCallback = (XVSceneChange_Callback)CallbackFunc;
	InstancePtr->CallbackRef = CallbackRef;
}

/*****************************************************************************/
/**
 *
 * This function is the interrupt handler for the Scenechange core driver.
 *
 * This handler clears the pending interrupt and determined if the source is
 * frame done signal. If yes, starts the next scenechange processing and calls
 * the registered callback function
 *
 * The application is responsible for connecting this function to the interrupt
 * system. Application beyond this driver is also responsible for providing
 * callbacks to handle interrupts during initialization phase.
 *
 * @param    InstancePtr is a pointer to the core instance that just
 *           interrupted.
 *
 * @return   None.
 *
 * @note     None.
 *
 ******************************************************************************/
void XV_scenechange_InterruptHandler(void *InstancePtr)
{
	XV_scenechange *SceneChangePtr = (XV_scenechange *)InstancePtr;
	u32 Status, stream_id, Data, SADTF;

	/* Verify arguments */
	Xil_AssertVoid(SceneChangePtr != NULL);
	Xil_AssertVoid(SceneChangePtr->IsReady == XIL_COMPONENT_IS_READY);

	/* Get the interrupt source */
	Status = XV_scenechange_InterruptGetStatus(SceneChangePtr);

	/* Check for Done Signal */
	if (Status & XV_SCENECHANGE_CTRL_ADDR_ISR_AP_DONE) {
		Data = XV_scenechange_Get_HwReg_stream_enable(SceneChangePtr);
		for (stream_id = 0;
			stream_id < SceneChangePtr->scd_config->num_streams;
								stream_id++) {
			if(Data & (1 << stream_id)) {
				SceneChangePtr->glconfig[stream_id].SAD =
					XV_scenechange_ReadReg(SceneChangePtr->Ctrl_BaseAddress,
					        ((stream_id * XV_SCD_LAYER_OFFSET) +
						XV_SCENECHANGE_CTRL_ADDR_HWREG_SAD0_DATA));

				SADTF = ((SceneChangePtr->glconfig[stream_id].SAD *
					  SceneChangePtr->glconfig[stream_id].subsample) /
					 (SceneChangePtr->glconfig[stream_id].height *
					  SceneChangePtr->glconfig[stream_id].width));

				if(SADTF >= SceneChangePtr->glconfig[stream_id].threshold) {
					SceneChangePtr->scdlayerdetSAD =
						SceneChangePtr->glconfig[stream_id].SAD;
					SceneChangePtr->scddetlayerid = stream_id;
					SceneChangePtr->FrameDoneCallback(SceneChangePtr);
				}
			}
		}
		/* Clear the interrupt */
		XV_scenechange_InterruptClear(SceneChangePtr,
				XV_SCENECHANGE_CTRL_ADDR_ISR_AP_DONE);
	}
}
/** @} */