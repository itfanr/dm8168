/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include "algLink_priv.h"
#include <mcfw/src_bios6/utils/utils_mem.h>



#ifdef ENABLE_FXN_PROFILE
#define ALG_LINK_FXN_PROFILE_STG1_START_COUNT      (1000)
#define ALG_LINK_FXN_PROFILE_STG2_START_COUNT      (1300)
#define ALG_LINK_FXN_PROFILE_PRINT_COUNT           (1600)
#define FXN_PROFILE_CALL_THRESHOLD                  (45)    /* Start profiling functions which are getting invoked more than this number of times */
#include <mcfw/src_bios6/utils/utils_fnprof.h>

static
Void AlgLink_drvFxnProfileControl(AlgLink_Obj * pObj)
{
    static Bool fxnProfStg1Started = FALSE;
    static Bool fxnProfStg2Started = FALSE;
    static Bool fxnProfPrintDone = FALSE;
    static Int32 count = 0;

    if (SYSTEM_LINK_ID_ALG_0 ==  pObj->linkId)
    {

        if ((count >= ALG_LINK_FXN_PROFILE_STG1_START_COUNT)
            &&
            (FALSE == fxnProfStg1Started))
        {
            FNPROF_STG1_enableProfile();
            fxnProfStg1Started = TRUE;
        }
        if ((count >= ALG_LINK_FXN_PROFILE_STG1_START_COUNT)
            &&
            (TRUE == fxnProfStg1Started)
            &&
            (FALSE == fxnProfPrintDone))
        {
            FNPROF_hookOverheadCalibrateFxn();
        }
        if ((count >= ALG_LINK_FXN_PROFILE_STG2_START_COUNT)
            &&
            (FALSE == fxnProfStg2Started))
        {
            FNPROF_STG2_setFxnCallCntThreshold(FXN_PROFILE_CALL_THRESHOLD);
            FNPROF_STG2_enableProfile();
            fxnProfStg2Started = TRUE;
        }
        if ((count >= ALG_LINK_FXN_PROFILE_STG2_START_COUNT)
            &&
            (TRUE == fxnProfStg2Started)
            &&
            (FALSE == fxnProfPrintDone))
        {
            FNPROF_hookOverheadCalibrateFxn();
        }
        if ((count >= ALG_LINK_FXN_PROFILE_PRINT_COUNT)
            &&
            (FALSE == fxnProfPrintDone))
        {
            FNPROF_printProfileInfo();
            fxnProfPrintDone = TRUE;
            FNPROF_disableProfiling();
        }
        count++;
    }
}
#endif


Int32 AlgLink_algCreate(AlgLink_Obj * pObj, AlgLink_CreateParams * pPrm)
{
    Int32 status;
    Int32 queueId;
    
    Vps_printf(" %d: ALG : Create in progress !!!\n", Utils_getCurTimeInMsec());
    Vps_printf("*********************test&&&&&&&&&&&&&&&\r\n");

    UTILS_MEMLOG_USED_START();
    memcpy(&pObj->createArgs, pPrm, sizeof(*pPrm));
    memcpy(&pObj->scdAlg.createArgs, &pPrm->scdCreateParams, sizeof(AlgLink_ScdCreateParams));
    memcpy(&pObj->osdAlg.osdChCreateParams, &pPrm->osdChCreateParams, (sizeof(AlgLink_OsdChCreateParams) * ALG_LINK_OSD_MAX_CH));
    status = System_linkGetInfo(pPrm->inQueParams.prevLinkId, &pObj->inTskInfo);
    Vps_printf("******Id:%d*************\r\n",pPrm->inQueParams.prevLinkId);

    UTILS_assert(status == FVID2_SOK);

    UTILS_assert(pPrm->inQueParams.prevLinkQueId < pObj->inTskInfo.numQue);

    memcpy(&pObj->inQueInfo,
           &pObj->inTskInfo.queInfo[pPrm->inQueParams.prevLinkQueId],
           sizeof(pObj->inQueInfo));

    UTILS_assert(pObj->inQueInfo.numCh <= ALG_LINK_OSD_MAX_CH);

    pObj->scdAlg.inQueInfo = &pObj->inQueInfo;

    pObj->osdAlg.inQueInfo = &pObj->inQueInfo;

    if (pObj->createArgs.enableOSDAlg)
    {
        status = AlgLink_OsdalgCreate(&pObj->osdAlg);
        UTILS_assert(status == FVID2_SOK);
    }

    if (pObj->createArgs.enableSCDAlg)
    {
        for(queueId = 0; queueId < ALG_LINK_MAX_OUT_QUE; queueId++)
        {
           memcpy(&pObj->scdAlg.outQueParams[queueId], &pPrm->outQueParams[queueId], sizeof(System_LinkOutQueParams));
        }
        
        status = Utils_dmaOpen();
        UTILS_assert(status == FVID2_SOK);
        status = AlgLink_ScdalgCreate(&pObj->scdAlg);
        UTILS_assert(status == FVID2_SOK);
    }

    pObj->isCreated = ALG_LINK_STATE_ACTIVE;



    UTILS_MEMLOG_USED_END(pObj->memUsed);
    UTILS_MEMLOG_PRINT("ALGLINK",
                       pObj->memUsed,
                       UTILS_ARRAYSIZE(pObj->memUsed));
    Vps_printf(" %d: ALG : Create Done !!!\n", Utils_getCurTimeInMsec());
    return FVID2_SOK;
}

Int32 AlgLink_algDelete(AlgLink_Obj * pObj)
{
    Int32 status;

    Vps_printf(" %d: ALG : Delete in progress !!!\n", Utils_getCurTimeInMsec());


    if (pObj->createArgs.enableOSDAlg)
    {
        status = AlgLink_OsdalgDelete(&pObj->osdAlg);
        UTILS_assert(status == FVID2_SOK);
    }

    if (pObj->createArgs.enableSCDAlg)
    {
        status = AlgLink_ScdalgDelete(&pObj->scdAlg);
        UTILS_assert(status == FVID2_SOK);
        status = Utils_dmaClose();
        UTILS_assert(status == FVID2_SOK);
    }
    pObj->isCreated = ALG_LINK_STATE_INACTIVE;
    Vps_printf(" %d: ALG : Delete Done !!!\n", Utils_getCurTimeInMsec());

	return FVID2_SOK;
}


Int32 AlgLink_algProcessData(AlgLink_Obj * pObj)
{
    UInt32 frameId, status =  FVID2_SOK;
    System_LinkInQueParams *pInQueParams;
    FVID2_Frame *pFrame;

    FVID2_FrameList frameList;

    pInQueParams = &pObj->createArgs.inQueParams;
    System_getLinksFullFrames(pInQueParams->prevLinkId,
                              pInQueParams->prevLinkQueId, &frameList);

#ifdef ENABLE_FXN_PROFILE
    AlgLink_drvFxnProfileControl(pObj);
#endif /* #ifdef ENABLE_FXN_PROFILE */

    if (frameList.numFrames)
    {
        pObj->inFrameGetCount += frameList.numFrames;
        /* SCD should be done first as it requires to operate on raw YUV */
        if (pObj->createArgs.enableSCDAlg)
        {
            status = AlgLink_ScdAlgSubmitFrames(&pObj->scdAlg, &frameList);
        }

        for(frameId=0; frameId<frameList.numFrames; frameId++)
        {
            pFrame = frameList.frames[frameId];

            if(pFrame->channelNum >= pObj->inQueInfo.numCh)
                continue;

            // do SW OSD
            if (pObj->createArgs.enableOSDAlg)
            {
                AlgLink_OsdalgProcessFrame(&pObj->osdAlg, pFrame);
            }
        }
        System_putLinksEmptyFrames(pInQueParams->prevLinkId,
                                   pInQueParams->prevLinkQueId, &frameList);
    }

    return status;
}
