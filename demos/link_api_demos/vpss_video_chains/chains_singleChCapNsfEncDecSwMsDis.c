/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#include <demos/link_api_demos/common/chains.h>
#include <mcfw/interfaces/ti_venc.h>

static SystemVideo_Ivahd2ChMap_Tbl systemVid_encDecIvaChMapTbl =
{
    .isPopulated = 1,
    .ivaMap[0] =
    {
        .EncNumCh  = 10,
        .EncChList = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0 , 0, 0},
        .DecNumCh  = 0,
        .DecChList = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    .ivaMap[1] =
    {
        .EncNumCh  = 0,
        .EncChList = {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31},
        .DecNumCh  = 12,
        .DecChList = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0, 0 , 0, 0},
    },
    .ivaMap[2] =
    {
        .EncNumCh  = 6,
        .EncChList = {10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        .DecNumCh  = 4,
        .DecChList = {12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
};

Void Chains_singleChCapNsfEncDecSwMsDis(Chains_Ctrl *chainsCfg)
{
    CaptureLink_CreateParams    capturePrm;
    CaptureLink_VipInstParams   *pCaptureInstPrm;
    CaptureLink_OutParams       *pCaptureOutPrm;
    NsfLink_CreateParams        nsfPrm;
    SwMsLink_CreateParams       swMsPrm;
    DisplayLink_CreateParams    displayPrm;
    EncLink_CreateParams        encPrm;
    DecLink_CreateParams        decPrm;
    IpcLink_CreateParams        ipcOutVpssPrm;
    IpcLink_CreateParams        ipcInVpssPrm;
    IpcLink_CreateParams        ipcOutVideoPrm;
    IpcLink_CreateParams        ipcInVideoPrm;

    UInt32 captureId, displayId;
    UInt32 nsfId, swMsId;
    UInt32 vipInstId;
    UInt32 encId, decId;
    UInt32 ipcOutVpssId, ipcInVpssId;
    UInt32 ipcOutVideoId, ipcInVideoId;

    UInt32 i;
    char   ch;

    CHAINS_INIT_STRUCT(CaptureLink_CreateParams, capturePrm);
    CHAINS_INIT_STRUCT(NsfLink_CreateParams, nsfPrm);
    CHAINS_INIT_STRUCT(SwMsLink_CreateParams, swMsPrm);
    CHAINS_INIT_STRUCT(DisplayLink_CreateParams,displayPrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcOutVpssPrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcInVpssPrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcOutVideoPrm);
    CHAINS_INIT_STRUCT(IpcLink_CreateParams,ipcInVideoPrm);
    CHAINS_INIT_STRUCT(EncLink_CreateParams, encPrm);
    CHAINS_INIT_STRUCT(DecLink_CreateParams, decPrm);

    captureId = SYSTEM_LINK_ID_CAPTURE;
    nsfId     = SYSTEM_LINK_ID_NSF_0;
    swMsId    = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
    displayId = SYSTEM_LINK_ID_DISPLAY_1;

    encId        = SYSTEM_LINK_ID_VENC_0;
    decId        = SYSTEM_LINK_ID_VDEC_0;

    ipcOutVpssId = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
    ipcInVideoId = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;
    ipcOutVideoId= SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0;
    ipcInVpssId  = SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0;

    System_linkControl(
        SYSTEM_LINK_ID_M3VIDEO,
        SYSTEM_COMMON_CMD_SET_CH2IVAHD_MAP_TBL,
        &systemVid_encDecIvaChMapTbl,
        sizeof(SystemVideo_Ivahd2ChMap_Tbl),
        TRUE
    );

    capturePrm.numVipInst               = 1;
    capturePrm.tilerEnable              = FALSE;
    capturePrm.numBufsPerCh             = CAPTURE_LINK_NUM_BUFS_PER_CH_DEFAULT;
    capturePrm.numExtraBufs             = 0;
    capturePrm.maxBlindAreasPerCh       = 0;
    capturePrm.isPalMode                = FALSE;
    capturePrm.enableSdCrop             = FALSE;
    capturePrm.doCropInCapture          = FALSE;

    for(vipInstId=0; vipInstId<capturePrm.numVipInst; vipInstId++)
    {
        pCaptureInstPrm                     = &capturePrm.vipInst[vipInstId];
        pCaptureInstPrm->vipInstId          = (SYSTEM_CAPTURE_INST_VIP0_PORTA+vipInstId*2)%SYSTEM_CAPTURE_INST_MAX;
        pCaptureInstPrm->videoDecoderId     = SYSTEM_DEVICE_VID_DEC_SII9233A_DRV;
        pCaptureInstPrm->inDataFormat       = SYSTEM_DF_YUV422P;
        pCaptureInstPrm->standard           = chainsCfg->displayRes[0];
        pCaptureInstPrm->numOutput          = 1;
        pCaptureInstPrm->numChPerOutput     = 1;
        pCaptureInstPrm->frameCaptureMode   = FALSE;
        pCaptureInstPrm->fieldsMerged       = FALSE;
        if (Chains_IsInterlaced(chainsCfg->displayRes[0])) {
            pCaptureInstPrm->frameCaptureMode = TRUE;
            pCaptureInstPrm->fieldsMerged   = TRUE;
        }

        pCaptureOutPrm                      = &pCaptureInstPrm->outParams[0];
        pCaptureOutPrm->dataFormat          = SYSTEM_DF_YUV422I_YUYV;
        pCaptureOutPrm->scEnable            = FALSE;
        pCaptureOutPrm->scOutWidth          = chainsCfg->channelConf[0].width;
        pCaptureOutPrm->scOutHeight         = chainsCfg->channelConf[0].height;
        pCaptureOutPrm->outQueId            = 0;
    }
    capturePrm.outQueParams[0].nextLink = nsfId;

    System_linkCreate(captureId, &capturePrm, sizeof(capturePrm));
    chainsCfg->displayRes[0] = capturePrm.vipInst[0].standard;
    Vsys_getResSize(chainsCfg->displayRes[0], &chainsCfg->channelConf[0].width, &chainsCfg->channelConf[0].height);
    Vsys_getResRate(chainsCfg->displayRes[0], &chainsCfg->channelConf[0].frameRate);

    nsfPrm.inQueParams.prevLinkId    = captureId;
    nsfPrm.inQueParams.prevLinkQueId = 0;
    nsfPrm.bypassNsf                 = FALSE;
    nsfPrm.tilerEnable               = TRUE;
    nsfPrm.numOutQue                 = 1;
    nsfPrm.outQueParams[0].nextLink  = ipcOutVpssId;
    nsfPrm.inputFrameRate            = chainsCfg->channelConf[0].frameRate;
    nsfPrm.outputFrameRate           = chainsCfg->channelConf[0].frameRate;
    nsfPrm.numBufsPerCh              = 8;

    ipcOutVpssPrm.inQueParams.prevLinkId    = nsfId;
    ipcOutVpssPrm.inQueParams.prevLinkQueId = 0;
    ipcOutVpssPrm.numOutQue                 = 1;
    ipcOutVpssPrm.numChPerOutQue[0]         = 1;
    ipcOutVpssPrm.outQueParams[0].nextLink  = ipcInVideoId;
    ipcOutVpssPrm.notifyNextLink            = TRUE;
    ipcOutVpssPrm.notifyPrevLink            = FALSE;
    ipcOutVpssPrm.noNotifyMode              = FALSE;
    ipcOutVpssPrm.inputFrameRate            = chainsCfg->channelConf[0].frameRate;
    ipcOutVpssPrm.outputFrameRate           = chainsCfg->channelConf[0].frameRate;
    ipcOutVpssPrm.equallyDivideChAcrossOutQues = FALSE;
    ipcOutVpssPrm.numChPerOutQue[0]         = 0;

    ipcInVideoPrm.inQueParams.prevLinkId    = ipcOutVpssId;
    ipcInVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcInVideoPrm.numOutQue                 = 1;
    ipcInVideoPrm.numChPerOutQue[0]         = 1;
    ipcInVideoPrm.outQueParams[0].nextLink  = encId;
    ipcInVideoPrm.notifyNextLink            = TRUE;
    ipcInVideoPrm.notifyPrevLink            = FALSE;
    ipcInVideoPrm.noNotifyMode              = FALSE;
    ipcInVideoPrm.inputFrameRate            = chainsCfg->channelConf[0].frameRate;
    ipcInVideoPrm.outputFrameRate           = chainsCfg->channelConf[0].frameRate;
    ipcInVideoPrm.equallyDivideChAcrossOutQues = FALSE;
    ipcInVideoPrm.numChPerOutQue[0]         = 0;

    for (i=0; i<1; i++) {
        encPrm.chCreateParams[i].format                             = chainsCfg->channelConf[i].encFormat;
        encPrm.chCreateParams[i].profile                            = chainsCfg->channelConf[i].encProfile;
        encPrm.chCreateParams[i].dataLayout                         = IVIDEO_FIELD_SEPARATED;
        encPrm.chCreateParams[i].fieldMergeEncodeEnable             = FALSE;
        encPrm.chCreateParams[i].enableAnalyticinfo                 = FALSE;
        encPrm.chCreateParams[i].enableWaterMarking                 = FALSE;
        encPrm.chCreateParams[i].encodingPreset                     = XDM_USER_DEFINED;
        encPrm.chCreateParams[i].rateControlPreset                  = IVIDEO_USER_DEFINED;
        encPrm.chCreateParams[i].enableHighSpeed                    = FALSE;
        encPrm.chCreateParams[i].defaultDynamicParams.intraFrameInterval  = chainsCfg->channelConf[i].intraFrameInterval;
        encPrm.chCreateParams[i].defaultDynamicParams.interFrameInterval  = 1;
        encPrm.chCreateParams[i].defaultDynamicParams.inputFrameRate      = chainsCfg->channelConf[i].frameRate;
        encPrm.chCreateParams[i].defaultDynamicParams.targetFrameRate     = chainsCfg->channelConf[i].encFrameRate;
        encPrm.chCreateParams[i].defaultDynamicParams.targetBitRate       = chainsCfg->channelConf[i].bitRate;
        encPrm.chCreateParams[i].defaultDynamicParams.mvAccuracy          = IVIDENC2_MOTIONVECTOR_QUARTERPEL;
        encPrm.chCreateParams[i].defaultDynamicParams.rcAlg               = chainsCfg->channelConf[i].rateCtrl;
        encPrm.chCreateParams[i].defaultDynamicParams.qpMin               = 0;
        encPrm.chCreateParams[i].defaultDynamicParams.qpMax               = 51;
        encPrm.chCreateParams[i].defaultDynamicParams.qpInit              = -1;
        encPrm.chCreateParams[i].defaultDynamicParams.vbrDuration         = 8;
        encPrm.chCreateParams[i].defaultDynamicParams.vbrSensitivity      = 0;
    }
    encPrm.inQueParams.prevLinkId   = ipcInVideoId;
    encPrm.inQueParams.prevLinkQueId= 0;
    encPrm.outQueParams.nextLink    = decId;
    encPrm.numBufPerCh[0]           = 8;

    Chains_displayCtrlInit(chainsCfg->displayRes);

    System_linkCreate(nsfId , &nsfPrm, sizeof(nsfPrm));
    System_linkCreate(ipcOutVpssId , &ipcOutVpssPrm , sizeof(ipcOutVpssPrm) );
    System_linkCreate(ipcInVideoId , &ipcInVideoPrm , sizeof(ipcInVideoPrm) );
    System_linkCreate(encId, &encPrm, sizeof(encPrm));

    for (i=0; i<1; i++) {
        decPrm.chCreateParams[i].format                               = chainsCfg->channelConf[i].encFormat;
        decPrm.chCreateParams[i].profile                              = IH264VDEC_PROFILE_ANY;
        decPrm.chCreateParams[i].targetMaxWidth                       = chainsCfg->channelConf[i].width;
        decPrm.chCreateParams[i].targetMaxHeight                      = chainsCfg->channelConf[i].height;
        decPrm.chCreateParams[i].fieldMergeDecodeEnable               = FALSE;
        decPrm.chCreateParams[i].defaultDynamicParams.targetFrameRate = chainsCfg->channelConf[i].encFrameRate;
        decPrm.chCreateParams[i].defaultDynamicParams.targetBitRate   = chainsCfg->channelConf[i].bitRate;
        decPrm.chCreateParams[i].numBufPerCh                          = 8;
    }
    decPrm.inQueParams.prevLinkId    = encId;
    decPrm.inQueParams.prevLinkQueId = 0;
    decPrm.outQueParams.nextLink     = ipcOutVideoId;
    decPrm.tilerEnable               = TRUE;

    ipcOutVideoPrm.inQueParams.prevLinkId    = decId;
    ipcOutVideoPrm.inQueParams.prevLinkQueId = 0;
    ipcOutVideoPrm.numOutQue                 = 1;
    ipcOutVideoPrm.outQueParams[0].nextLink  = ipcInVpssId;
    ipcOutVideoPrm.notifyNextLink            = TRUE;
    ipcOutVideoPrm.notifyPrevLink            = FALSE;
    ipcOutVideoPrm.noNotifyMode              = FALSE;
    ipcOutVideoPrm.inputFrameRate            = chainsCfg->channelConf[0].encFrameRate;
    ipcOutVideoPrm.outputFrameRate           = chainsCfg->channelConf[0].encFrameRate;
    ipcOutVideoPrm.equallyDivideChAcrossOutQues = FALSE;
    ipcOutVideoPrm.numChPerOutQue[0]         = 0;

    ipcInVpssPrm.inQueParams.prevLinkId    = ipcOutVideoId;
    ipcInVpssPrm.inQueParams.prevLinkQueId = 0;
    ipcInVpssPrm.numOutQue                 = 1;
    ipcInVpssPrm.outQueParams[0].nextLink  = swMsId;
    ipcInVpssPrm.notifyNextLink            = TRUE;
    ipcInVpssPrm.notifyPrevLink            = FALSE;
    ipcInVpssPrm.noNotifyMode              = FALSE;
    ipcInVpssPrm.inputFrameRate            = chainsCfg->channelConf[0].encFrameRate;
    ipcInVpssPrm.outputFrameRate           = chainsCfg->channelConf[0].encFrameRate;
    ipcInVpssPrm.equallyDivideChAcrossOutQues = FALSE;
    ipcInVpssPrm.numChPerOutQue[0]         = 0;

    swMsPrm.numSwMsInst               = 1;
    swMsPrm.swMsInstId[0]             = SYSTEM_SW_MS_SC_INST_SC5;
    swMsPrm.inQueParams.prevLinkId    = ipcInVpssId;
    swMsPrm.inQueParams.prevLinkQueId = 0;
    swMsPrm.outQueParams.nextLink     = displayId;
    swMsPrm.lineSkipMode              = FALSE;
    swMsPrm.enableLayoutGridDraw      = FALSE;
    swMsPrm.layoutPrm.outputFPS       = chainsCfg->channelConf[0].frameRate;
    swMsPrm.maxInputQueLen            = SYSTEM_SW_MS_DEFAULT_INPUT_QUE_LEN;
    swMsPrm.maxOutRes                 = chainsCfg->displayRes[0];
    swMsPrm.numOutBuf                 = 0;

    Chains_swMsGenerateLayoutParams(0, 2, &swMsPrm);

    displayPrm.inQueParams[0].prevLinkId    = swMsId;
    displayPrm.inQueParams[0].prevLinkQueId = 0;
    displayPrm.displayRes                   = chainsCfg->displayRes[0];

    System_linkCreate(decId, &decPrm, sizeof(decPrm));
    System_linkCreate(ipcOutVideoId, &ipcOutVideoPrm, sizeof(ipcOutVideoPrm));
    System_linkCreate(ipcInVpssId  , &ipcInVpssPrm  , sizeof(ipcInVpssPrm)  );
    System_linkCreate(swMsId, &swMsPrm, sizeof(swMsPrm));
    System_linkCreate(displayId, &displayPrm, sizeof(displayPrm));

    Chains_memPrintHeapStatus();

    {
        System_linkStart(displayId);
        System_linkStart(swMsId);
        System_linkStart(decId);
        System_linkStart(encId);
        System_linkStart(nsfId);
        System_linkStart(captureId);

        while(1)
        {
            ch = Chains_menuRunTime();
            if(ch=='0')
                break;
            if(ch=='v')
                System_linkControl(captureId, CAPTURE_LINK_CMD_FORCE_RESET, NULL, 0, TRUE);
            if(ch=='p')
                System_linkControl(captureId, CAPTURE_LINK_CMD_PRINT_ADV_STATISTICS, NULL, 0, TRUE);
        }

        System_linkStop(captureId);
        System_linkStop(nsfId);
        System_linkStop(encId);
        System_linkStop(decId);
        System_linkStop(swMsId);
        System_linkStop(displayId);
    }

    System_linkDelete(captureId);
    System_linkDelete(nsfId);
    System_linkDelete(ipcOutVpssId);
    System_linkDelete(ipcInVideoId);
    System_linkDelete(encId);
    System_linkDelete(decId);
    System_linkDelete(ipcOutVideoId);
    System_linkDelete(ipcInVpssId);
    System_linkDelete(swMsId);
    System_linkDelete(displayId);

    Chains_displayCtrlDeInit();
}
