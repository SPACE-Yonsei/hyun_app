/*******************************************************************************
**
**      GSC-18128-1, "Core Flight Executive Version 6.7"
**
**      Copyright (c) 2006-2019 United States Government as represented by
**      the Administrator of the National Aeronautics and Space Administration.
**      All Rights Reserved.
**
**      Licensed under the Apache License, Version 2.0 (the "License");
**      you may not use this file except in compliance with the License.
**      You may obtain a copy of the License at
**
**        http://www.apache.org/licenses/LICENSE-2.0
**
**      Unless required by applicable law or agreed to in writing, software
**      distributed under the License is distributed on an "AS IS" BASIS,
**      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**      See the License for the specific language governing permissions and
**      limitations under the License.
**
** File: hyun_app.c
**
** Purpose:
**   This file contains the source code for the Hyun_app.
**
*******************************************************************************/

/* CFS APP STUDY 
cFE App developer's guide (https://github.com/nasa/cFE/blob/64a6a59456fa9e47dc93e4bb9cecacc3d86d1862/docs/cFE%20Application%20Developers%20Guide.md)
HYUN_APP 은 SAMPLE APP의 코드를 guide에 따라 분석하고,
통신 등 필수 기능들을 추가로 구현한 후 자세히 원리를 기술할 것이다.
*/

/* BACKGROUND
cFS엔 5개의 Core Service가 있다.
ES : Executive Service
SB : Software Bus Service
EVS : Event Service
TBL : Table Service
TIME : Time Service
*/

/* 유의할 점
함수를 선언하면, header에도 같이 선언해주자!
App에 새로운 Data를 선언할 때는, 무조건 hyun_app.h에 있는 HYUN_APP_Data_t 에 추가한다.
main 함수를 제외한 함수들은 int32 형식으로 선언하고, 성공적으로 실행되었을 때 return CFE_SUCCESS; 을 해준다.
*/

/*
** Include Files:
*/
#include "hyun_app_events.h"
#include "hyun_app_version.h"
#include "hyun_app.h"
#include "hyun_app_table.h"

/* The sample_lib module provides the SAMPLE_LIB_Function() prototype */
#include <string.h>
#include "sample_lib.h"

/* This spacey.h provides essential data for CANSAT AAS 2024 mission */
#include "libs/spacey.h"

/*
** global data
앱의 Global data는 여기에 저장한다.
데이터를 추가하고 싶다면 hyun_app.h의 struct 문 안에 원하는 데이터를 삽입하면 된다.
*/
HYUN_APP_Data_t HYUN_APP_Data;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
/* HYUN_APP_Main() -- Application entry point and main process loop           */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
void HYUN_APP_Main(void)
{
    int32            status;
    CFE_SB_Buffer_t *SBBufPtr;
    /*
    ** Create the first Performance Log entry
    */
    CFE_ES_PerfLogEntry(HYUN_APP_PERF_ID);

    /*
    ** Perform application specific initialization
    ** If the Initialization fails, set the RunStatus to
    ** CFE_ES_RunStatus_APP_ERROR and the App will not enter the RunLoop
    */
    status = HYUN_APP_Init();
    if (status != CFE_SUCCESS)
    {
        HYUN_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    /*
    ** SAMPLE Runloop
    */
    while (CFE_ES_RunLoop(&HYUN_APP_Data.RunStatus) == true)
    {
        /*
        ** Performance Log Exit Stamp
        */
        CFE_ES_PerfLogExit(HYUN_APP_PERF_ID);

        /* Pend on receipt of command packet */
        status = CFE_SB_ReceiveBuffer(&SBBufPtr, HYUN_APP_Data.CommandPipe, CFE_SB_PEND_FOREVER);

        /*
        ** Performance Log Entry Stamp
        */
        CFE_ES_PerfLogEntry(HYUN_APP_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            HYUN_APP_ProcessCommandPacket(SBBufPtr);
            //printf("Hyun app ES RUNLOOP\n");
        }
        else
        {
            CFE_EVS_SendEvent(HYUN_APP_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "HYUN_APP: SB Pipe Read Error, App Will Exit");

            HYUN_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
        }
    }

    /*
    ** Performance Log Exit Stamp
    */
    CFE_ES_PerfLogExit(HYUN_APP_PERF_ID);

    CFE_ES_ExitApp(HYUN_APP_Data.RunStatus);

} /* End of HYUN_APP_Main() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
/*                                                                            */
/* HYUN_APP_SendToBus() --  Software Bus(SB)를 사용하는 법을 적은 함수        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/

int32 HYUN_APP_SB_TUTORIAL(void)
{
    // 주의. 이 함수는 설명용이니 호출하면 오류가 날 수도 있다.
    /*
    cFE App Dev Guide(위에 링크 있음) 6장에 자세한 내용 기술되어 있음.
    Software Bus(SB)의 주요 목적은 서브시스템이 패킷이 어디에서 라우팅되는지에 관계없이 
    패킷을 전송하고 패킷이 어디에서 왔는지에 대한 지식 없이 패킷을 수신할 수 있도록 하는 
    메커니즘을 제공하는 것

    모든 앱이 message를 보낼 수 있고, 받을 수 있기에, SB를 사용하면
    one-to-one, many-to-one, one-to-many, many-to-many 통신이 다 가능하다. 
    */

    /*
    SB로부터 message를 받으려면, 먼저 pipe를 만들어야 한다.
    Pipe는 처리되지 않은 SB message를 저장하는 Queue다.
    각 Pipe는 한개의 App만이 읽을 수 있다. 한 앱이 여러 개의 Pipe를 읽을 수 있다!

    Pipe에는 Pipe depth와 message limit이 있다.
    Pipe depth는 한 Pipe에 얼마나 많은 message가 들어갈 수 있는지,
    message limit은 특정 message ID를 가진 message가 한 pipe에 얼마나 들어갈 수 있는지
    를 정의한다.
    */

    /*
    Software Bus Message는 Message ID를 통해 구분된다. 
    따라서 Message ID는 각 message마다 고유하다.
    */

    /*
    App이 SB Message를 보내면, SB는 App의 Routing Table을 읽어서 Message가 어디로 보내져야 하는지 확인하고
    알맞은 Pipe에 message를 전달한다.
    App은 SB API를 통해 자신이 subscribe한 pipe에 있는 message를 읽어올 수 있다.
    */

    /* 구현 시작 */
    
    /*
    App이 Initalization 될 때, app은 cFE에 자신이 쓸 pipe들을 알려줘야 한다.
    먼저 헤더 파일 (hyun_app.h)에 관련 변수들을 정의하자.
    hyun_app.h -> HYUN_PIPE_1_DEPTH 선언
    hyun_app.h -> HYUN_PIPE_1_NAME 선언
    hyun_app.h -> HYUN_PIPE_1 선언
    */

    int32 Status; 

    /*
    pipe를 만들 때는 다음과 같이 한다.
    CFE_SB_CreatePipe 함수는 Pipe를 만드는 함수로
    (Pipe identifier, Pipe Depth, Pipe name) 으로 구성되어 있다.

    주의 : Pipe는 multithreading에 안전하지 않으니, 각 thread마다 pipe를 따로 쓰자.

    App이 종료되면 Pipe는 자동으로 삭제된다.
    */
    Status = CFE_SB_CreatePipe(&HYUN_APP_Data.HYUN_PIPE_1, HYUN_PIPE_1_DEPTH, HYUN_PIPE_1_NAME);
    
    /*
    Pipe를 만들었으니, 이제 Pipe에 Message를 달라고 요청해야한다. 이를 Subscription이라 한다.
    먼저 hyun_msgids.h 에 가서 고유한 Message ID를 할당하자. Message ID는 겹치기 않게 따로 관리해줘야 한다.
    hyun_msgids.h -> HYUN_APP_TUTORIAL_MID 선언
    hyun_app.h -> HYUN_APP_TUTORIAL_LIMIT 선언
    
    2024 CANSAT AAS 대회에 쓰일 Message ID는 여기에 정리해놨다.
    https://docs.google.com/spreadsheets/d/14jm9sYXvmYip1VJhmo1ljE0NgvM-0HKMU5wFBPCmGCI/edit?usp=sharing
    */
     Status = CFE_SB_SubscribeEx(HYUN_APP_TUTORIAL_MID, HYUN_APP_Data.HYUN_PIPE_1, CFE_SB_DEFAULT_QOS, HYUN_APP_TUTORIAL_LIMIT);
    /*
    CFE_SB_SubscribeEx에는 다음과 같은 값이 들어가야 한다.
    (받을 message의 MID, message를 받을 pipe, QoS, Message Limit)
    QoS값은 무조건 DEFAULT 값을 써야 한다.
    */

    /*
    만약 MID와 Pipe만 지정하고, 나머지는 기본값을 쓰고싶으면 CFE_SB_Subscribe를 쓰자.
    */
    //Status = CFE_SB_Subscribe(HYUN_APP_TUTORIAL_MID, HYUN_APP_Data.HYUN_PIPE_1);

    /*
    message를 unsubscribe하고 싶으면 CFE_SB_Unsubscribe를 사용할 수 있다.
    CFE_SB_Unsubscribe에는 다음과 같은 값이 들어간다.
    (message의 MID, message를 받던 pipe)
    */
    // Status = CFE_SB_Unsubscribe(HYUN_APP_TUTORIAL_MID, HYUN_APP_Data.HYUN_PIPE_1)

    /*
    이제 Message를 보내는 법을 알아보자.
    Message를 보내려면 일단 Message를 만들어야 한다.
    hyun_app_msg.h -> HYUN_APP_TUTORIAL_t 추가
    hyun_app_msg.h -> HYUN_APP_TUTORIAL_Payload_t 추가
    hyun_app.h -> HYUN_APP_Data에 Comdounter, Errcounter, TutorialPacket 추가
    */

    Status = CFE_MSG_Init(&HYUN_APP_Data.TutorialPacket.TlmHeader.Msg, HYUN_APP_TUTORIAL_MID, sizeof(HYUN_APP_Data.TutorialPacket));
    /*
    CFE_MSG_Init 함수는 Message를 만들어주는 역할이고, 다음과 같은 값이 들어간다.
    (SB Message Data Buffer의 주소, message의 MID, Buffer의 크기)
    */

    /*
    Message Header에는 5개의 Field가 있는데, 여기서는 안다룰거다
    자세한 내용은 Guide의 6.6장 참조
    */

    /*
    Message를 만들었으니 이제 보내자!
    이 Message가 보내진 시간을 삽입하기 위해 우리는 CFE_SB_TimeStampMsg를 사용할거다.
    */
    strncpy(HYUN_APP_Data.TutorialPacket.Payload.TextData, "Hello World\n", 20);
    // 보낼 메세지의 CmdCounter, Error Counter 설정
    
    CFE_SB_TimeStampMsg(&HYUN_APP_Data.TutorialPacket.TlmHeader.Msg); // Message에 현재 시간 넣기
    CFE_SB_TransmitMsg(&HYUN_APP_Data.TutorialPacket.TlmHeader.Msg, true);
    /*
    CFE_SB_TransmitMsg는 메세지를 보내는 역할을 하고, 다음과 같은 값이 들어간다.
    (Message Header의 주소, IsOrigination 값(Documentation 참조, 일단 true를 쓰자))
    */
   
    /*
    메세지를 보냈으니 받아보자
    CFE_SB_ReceiveBuffer는 다음과 같은 값이 들어간다.
    (Message를 받을 포인터, 받을 Pipe, Timeout 설정)

    Timeout 설정은 다음과 같다.
    CFE_SB_PEND_FOREVER -> 값이 안들어오면 영원히 기다린다
    값을 숫자로 설정 : 그 값만큼 시간이 지나면 CFE_SB_TIME_OUT을 반환함
    */

    CFE_SB_Buffer_t *SBBufPtr; //메세지를 받을 포인터 선언
    Status = CFE_SB_ReceiveBuffer(&SBBufPtr, HYUN_APP_Data.HYUN_PIPE_1, CFE_SB_PEND_FOREVER);
    
    if (Status == CFE_SUCCESS)
    {
        HYUN_APP_ProcessCommandPacket(SBBufPtr);
        //printf("TUTORIAL MSG RCV\n");
    }    
    return CFE_SUCCESS;
}

int32 HYUN_APP_TEST_SB_INIT(void)
{
    int32 Status = 0; 

    //status = CFE_SB_CreatePipe(&HYUN_APP_Data.CommandPipe, HYUN_APP_Data.PipeDepth, HYUN_APP_Data.PipeName);

    //Status = CFE_SB_CreatePipe(&HYUN_APP_Data.HYUN_PIPE_1, HYUN_PIPE_1_DEPTH, HYUN_PIPE_1_NAME);
    if (Status == CFE_SUCCESS){
        printf("CREATE PIPE SUCCESS\n");
    }


    //Status = CFE_SB_SubscribeEx(HYUN_APP_TUTORIAL_MID, HYUN_APP_Data.HYUN_PIPE_1, CFE_SB_DEFAULT_QOS, HYUN_APP_TUTORIAL_LIMIT);
    if (Status == CFE_SUCCESS){
        printf("SUBSCRIBE SUCCESS\n");    
    }

    //status = CFE_SB_Subscribe(HYUN_APP_SEND_HK_MID, HYUN_APP_Data.CommandPipe);
    return CFE_SUCCESS;
}

int32 HYUN_APP_TEST_SB_SEND(void)
{
    strncpy(HYUN_APP_Data.TutorialPacket.Payload.TextData, "Hello world!\n", sizeof(HYUN_APP_Data.TutorialPacket.Payload.TextData));

    // 보낼 메세지의 CmdCounter, Error Counter, Text Data 설정

    CFE_SB_TimeStampMsg(&HYUN_APP_Data.TutorialPacket.TlmHeader.Msg); // Message에 현재 시간 넣기
    CFE_SB_TransmitMsg(&HYUN_APP_Data.TutorialPacket.TlmHeader.Msg, true);
    printf("test SB send\n");
    return CFE_SUCCESS;
}

int32 HYUN_APP_TEST_SB_RCV(void)
{
    int32 Status;
    CFE_SB_Buffer_t *SBBufPtr; //메세지를 받을 포인터 선언
    printf("buffer made\n");
    Status = CFE_SB_ReceiveBuffer(&SBBufPtr, HYUN_APP_Data.HYUN_PIPE_1, CFE_SB_PEND_FOREVER);
    
    if (Status == CFE_SUCCESS)
    {
        HYUN_APP_ProcessCommandPacket(SBBufPtr);
        printf("TUTORIAL MSG RCV\n");
    }
    printf("test msg rcv\n");
    return CFE_SUCCESS;
}

int32 HYUN_APP_SEND_CHAR20_TO_RCVTEST(void)
{
    strncpy(HYUN_APP_Data.Char20msgPacket.Payload.TextData, "Hello World!\n", sizeof(HYUN_APP_Data.Char20msgPacket.Payload.TextData));
    // 보낼 메세지의 CmdCounter, Error Counter 설정
    
    CFE_SB_TimeStampMsg(&HYUN_APP_Data.Char20msgPacket.TlmHeader.Msg); // Message에 현재 시간 넣기
    HYUN_APP_Data.Char20msgPacket.Payload.CommandErrorCounter = HYUN_APP_Data.ErrCounter;
    HYUN_APP_Data.Char20msgPacket.Payload.CommandCounter      = HYUN_APP_Data.CmdCounter;

    CFE_SB_TransmitMsg(&HYUN_APP_Data.Char20msgPacket.TlmHeader.Msg, true);
    return CFE_SUCCESS;
}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
/*                                                                            */
/* HYUN_APP_Init() --  initialization                                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
int32 HYUN_APP_Init(void)
{

    int32 status;

    HYUN_APP_Data.RunStatus = CFE_ES_RunStatus_APP_RUN;

    /*
    ** Initialize app command execution counters
    */
    HYUN_APP_Data.CmdCounter = 0;
    HYUN_APP_Data.ErrCounter = 0;

    /*
    ** Initialize app configuration data
    */
    HYUN_APP_Data.PipeDepth = HYUN_APP_PIPE_DEPTH;

    strncpy(HYUN_APP_Data.PipeName, "HYUN_APP_CMD_PIPE", sizeof(HYUN_APP_Data.PipeName));
    HYUN_APP_Data.PipeName[sizeof(HYUN_APP_Data.PipeName) - 1] = 0;

    /*
    ** Initialize event filter table...
    */
    HYUN_APP_Data.EventFilters[0].EventID = HYUN_APP_STARTUP_INF_EID;
    HYUN_APP_Data.EventFilters[0].Mask    = 0x0000;
    HYUN_APP_Data.EventFilters[1].EventID = HYUN_APP_COMMAND_ERR_EID;
    HYUN_APP_Data.EventFilters[1].Mask    = 0x0000;
    HYUN_APP_Data.EventFilters[2].EventID = HYUN_APP_COMMANDNOP_INF_EID;
    HYUN_APP_Data.EventFilters[2].Mask    = 0x0000;
    HYUN_APP_Data.EventFilters[3].EventID = HYUN_APP_COMMANDRST_INF_EID;
    HYUN_APP_Data.EventFilters[3].Mask    = 0x0000;
    HYUN_APP_Data.EventFilters[4].EventID = HYUN_APP_INVALID_MSGID_ERR_EID;
    HYUN_APP_Data.EventFilters[4].Mask    = 0x0000;
    HYUN_APP_Data.EventFilters[5].EventID = HYUN_APP_LEN_ERR_EID;
    HYUN_APP_Data.EventFilters[5].Mask    = 0x0000;
    HYUN_APP_Data.EventFilters[6].EventID = HYUN_APP_PIPE_ERR_EID;
    HYUN_APP_Data.EventFilters[6].Mask    = 0x0000;

    /*
    ** Register the events
    */
    status = CFE_EVS_Register(HYUN_APP_Data.EventFilters, HYUN_APP_EVENT_COUNTS, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Hyun_app: Error Registering Events, RC = 0x%08lX\n", (unsigned long)status);
        return (status);
    }

    /*
    ** Initialize Packets
    */

    /*
    ** Initialize test packet (clear user data area).
    */

    status = CFE_MSG_Init(&HYUN_APP_Data.TutorialPacket.TlmHeader.Msg, HYUN_APP_MID_SBTEST_REQ, sizeof(HYUN_APP_Data.TutorialPacket));
    if (status == CFE_SUCCESS){
        printf("test sb init done!\n");
    }

    

    /*
    ** Initialize housekeeping packet (clear user data area).
    */
    CFE_MSG_Init(&HYUN_APP_Data.HkTlm.TlmHeader.Msg, HYUN_APP_MID_HOUSEKEEPING_RES, sizeof(HYUN_APP_Data.HkTlm));

    /*
    ** Initialize " Char20msgPacket " 
    */
    CFE_MSG_Init(&HYUN_APP_Data.Char20msgPacket.TlmHeader.Msg, HYUN_APP_MID_SENDTORCVTEST_RES, sizeof(HYUN_APP_Data.Char20msgPacket));


    /*
    ** Create Software Bus message pipe.
    */
    status = CFE_SB_CreatePipe(&HYUN_APP_Data.CommandPipe, HYUN_APP_Data.PipeDepth, HYUN_APP_Data.PipeName);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Hyun_app: Error creating pipe, RC = 0x%08lX\n", (unsigned long)status);
        return (status);
    }

    /*
    ** Subscribe to Housekeeping request commands
    */
    status = CFE_SB_Subscribe(HYUN_APP_MID_HOUSEKEEPING_REQ, HYUN_APP_Data.CommandPipe);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Hyun_app: Error Subscribing to HK request, RC = 0x%08lX\n", (unsigned long)status);
        return (status);
    }

    /*
    ** Subscribe to ground command packets
    */
    status = CFE_SB_Subscribe(HYUN_APP_MID_GROUNDCMD_REQ, HYUN_APP_Data.CommandPipe);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Hyun_app: Error Subscribing to Command, RC = 0x%08lX\n", (unsigned long)status);

        return (status);
    }

    /*
    ** Register Table(s)
    */
    status = CFE_TBL_Register(&HYUN_APP_Data.TblHandles[0], "HyunAppTable", sizeof(HYUN_APP_Table_t),
                              CFE_TBL_OPT_DEFAULT, HYUN_APP_TblValidationFunc);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Hyun_app: Error Registering Table, RC = 0x%08lX\n", (unsigned long)status);

        return (status);
    }
    else
    {
        status = CFE_TBL_Load(HYUN_APP_Data.TblHandles[0], CFE_TBL_SRC_FILE, HYUN_APP_TABLE_FILE);
    }


    CFE_EVS_SendEvent(HYUN_APP_STARTUP_INF_EID, CFE_EVS_EventType_INFORMATION, "Hyun_app Initialized.%s",
                      HYUN_APP_VERSION_STRING);

    return (CFE_SUCCESS);

} /* End of HYUN_APP_Init() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  HYUN_APP_ProcessCommandPacket                                    */
/*                                                                            */
/*  Purpose:                                                                  */
/*     This routine will process any packet that is received on the SAMPLE    */
/*     command pipe.                                                          */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void HYUN_APP_ProcessCommandPacket(CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;

    CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);
    //printf("hyun app process MID = 0x%x\n", (unsigned int)CFE_SB_MsgIdToValue(MsgId));

    switch (MsgId)
    {
        case HYUN_APP_MID_GROUNDCMD_REQ:
            HYUN_APP_ProcessGroundCommand(SBBufPtr);
            break;

        case HYUN_APP_MID_HOUSEKEEPING_REQ:
            HYUN_APP_ReportHousekeeping((CFE_MSG_CommandHeader_t *)SBBufPtr);
            break;

        default:
            CFE_EVS_SendEvent(HYUN_APP_INVALID_MSGID_ERR_EID, CFE_EVS_EventType_ERROR,
                              "SAMPLE: invalid command packet,MID = 0x%x", (unsigned int)CFE_SB_MsgIdToValue(MsgId));
            break;
    }

    return;

} /* End HYUN_APP_ProcessCommandPacket */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* HYUN_APP_ProcessGroundCommand() -- SAMPLE ground commands                */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
void HYUN_APP_ProcessGroundCommand(CFE_SB_Buffer_t *SBBufPtr)
{
    //printf("hyun app process ground command\n");
    CFE_MSG_FcnCode_t CommandCode = 0;

    CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &CommandCode);

    /*
    ** Process "known" Hyun_app ground commands
    */
    switch (CommandCode)
    {
        case HYUN_APP_NOOP_CC:
            if (HYUN_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(HYUN_APP_NoopCmd_t)))
            {
                HYUN_APP_Noop((HYUN_APP_NoopCmd_t *)SBBufPtr);
            }

            break;

        case HYUN_APP_RESET_COUNTERS_CC:
            if (HYUN_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(HYUN_APP_ResetCountersCmd_t)))
            {
                HYUN_APP_ResetCounters((HYUN_APP_ResetCountersCmd_t *)SBBufPtr);
            }
            break;

        case HYUN_APP_PROCESS_CC:
            if (HYUN_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(HYUN_APP_ProcessCmd_t)))
            {
                HYUN_APP_Process((HYUN_APP_ProcessCmd_t *)SBBufPtr);
            }
            break;

        /* default case already found during FC vs length test */
        default:
            CFE_EVS_SendEvent(HYUN_APP_COMMAND_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Invalid ground command code: CC = %d", CommandCode);
            break;
    }

    return;

} /* End of HYUN_APP_ProcessGroundCommand() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  HYUN_APP_ReportHousekeeping                                          */
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function is triggered in response to a task telemetry request */
/*         from the housekeeping task. This function will gather the Apps     */
/*         telemetry, packetize it and send it to the housekeeping task via   */
/*         the software bus                                                   */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
int32 HYUN_APP_ReportHousekeeping(const CFE_MSG_CommandHeader_t *Msg)
{
    //printf("hyun app report housekeeping\n");
    int i;

    /*
    ** Get command execution counters...
    */
    HYUN_APP_Data.HkTlm.Payload.CommandErrorCounter = HYUN_APP_Data.ErrCounter;
    HYUN_APP_Data.HkTlm.Payload.CommandCounter      = HYUN_APP_Data.CmdCounter;

    /*
    ** Send housekeeping telemetry packet...
    */
    CFE_SB_TimeStampMsg(&HYUN_APP_Data.HkTlm.TlmHeader.Msg);
    CFE_SB_TransmitMsg(&HYUN_APP_Data.HkTlm.TlmHeader.Msg, true);

    /*
    ** Manage any pending table loads, validations, etc.
    */
    for (i = 0; i < HYUN_APP_NUMBER_OF_TABLES; i++)
    {
        CFE_TBL_Manage(HYUN_APP_Data.TblHandles[i]);
    }



    /*
    for test. delete later
    */
    HYUN_APP_SEND_CHAR20_TO_RCVTEST();

    //HYUN_APP_TEST_SB_RCV();

    return CFE_SUCCESS;

} /* End of HYUN_APP_ReportHousekeeping() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* HYUN_APP_Noop -- SAMPLE NOOP commands                                        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
int32 HYUN_APP_Noop(const HYUN_APP_NoopCmd_t *Msg)
{
    //printf("hyun app noop\n");
    HYUN_APP_Data.CmdCounter++;

    //CFE_EVS_SendEvent(HYUN_APP_COMMANDNOP_INF_EID, CFE_EVS_EventType_INFORMATION, "SAMPLE: NOOP command %s",HYUN_APP_VERSION);

    return CFE_SUCCESS;

} /* End of HYUN_APP_Noop */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  HYUN_APP_ResetCounters                                               */
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function resets all the global counter variables that are     */
/*         part of the task telemetry.                                        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
int32 HYUN_APP_ResetCounters(const HYUN_APP_ResetCountersCmd_t *Msg)
{

    HYUN_APP_Data.CmdCounter = 0;
    HYUN_APP_Data.ErrCounter = 0;

    CFE_EVS_SendEvent(HYUN_APP_COMMANDRST_INF_EID, CFE_EVS_EventType_INFORMATION, "SAMPLE: RESET command");

    return CFE_SUCCESS;

} /* End of HYUN_APP_ResetCounters() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  HYUN_APP_Process                                                     */
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function Process Ground Station Command                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
int32 HYUN_APP_Process(const HYUN_APP_ProcessCmd_t *Msg)
{
    int32               status;
    HYUN_APP_Table_t *TblPtr;
    const char *        TableName = "HYUN_APP.HyunAppTable";

    /* Sample Use of Table */

    status = CFE_TBL_GetAddress((void *)&TblPtr, HYUN_APP_Data.TblHandles[0]);

    if (status < CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Hyun_app: Fail to get table address: 0x%08lx", (unsigned long)status);
        return status;
    }

    CFE_ES_WriteToSysLog("Hyun_app: Table Value 1: %d  Value 2: %d", TblPtr->Int1, TblPtr->Int2);

    HYUN_APP_GetCrc(TableName);

    status = CFE_TBL_ReleaseAddress(HYUN_APP_Data.TblHandles[0]);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Hyun_app: Fail to release table address: 0x%08lx", (unsigned long)status);
        return status;
    }

    /* Invoke a function provided by HYUN_APP_LIB */
    SAMPLE_LIB_Function();

    return CFE_SUCCESS;

} /* End of HYUN_APP_ProcessCC */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* HYUN_APP_VerifyCmdLength() -- Verify command packet length                   */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
bool HYUN_APP_VerifyCmdLength(CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength)
{
    bool              result       = true;
    size_t            ActualLength = 0;
    CFE_SB_MsgId_t    MsgId        = CFE_SB_INVALID_MSG_ID;
    CFE_MSG_FcnCode_t FcnCode      = 0;

    CFE_MSG_GetSize(MsgPtr, &ActualLength);

    /*
    ** Verify the command packet length.
    */
    if (ExpectedLength != ActualLength)
    {
        CFE_MSG_GetMsgId(MsgPtr, &MsgId);
        CFE_MSG_GetFcnCode(MsgPtr, &FcnCode);

        CFE_EVS_SendEvent(HYUN_APP_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                          "Invalid Msg length: ID = 0x%X,  CC = %u, Len = %u, Expected = %u",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId), (unsigned int)FcnCode, (unsigned int)ActualLength,
                          (unsigned int)ExpectedLength);

        result = false;

        HYUN_APP_Data.ErrCounter++;
    }

    return (result);

} /* End of HYUN_APP_VerifyCmdLength() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* HYUN_APP_TblValidationFunc -- Verify contents of First Table      */
/* buffer contents                                                 */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 HYUN_APP_TblValidationFunc(void *TblData)
{
    int32               ReturnCode = CFE_SUCCESS;
    HYUN_APP_Table_t *TblDataPtr = (HYUN_APP_Table_t *)TblData;

    /*
    ** Sample Table Validation
    */
    if (TblDataPtr->Int1 > HYUN_APP_TBL_ELEMENT_1_MAX)
    {
        /* First element is out of range, return an appropriate error code */
        ReturnCode = HYUN_APP_TABLE_OUT_OF_RANGE_ERR_CODE;
    }

    return ReturnCode;

} /* End of HYUN_APP_TBLValidationFunc() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* HYUN_APP_GetCrc -- Output CRC                                     */
/*                                                                 */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void HYUN_APP_GetCrc(const char *TableName)
{
    int32          status;
    uint32         Crc;
    CFE_TBL_Info_t TblInfoPtr;

    status = CFE_TBL_GetInfo(&TblInfoPtr, TableName);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Hyun_app: Error Getting Table Info");
    }
    else
    {
        Crc = TblInfoPtr.Crc;
        CFE_ES_WriteToSysLog("Hyun_app: CRC: 0x%08lX\n\n", (unsigned long)Crc);
    }

    return;

} /* End of HYUN_APP_GetCrc */
