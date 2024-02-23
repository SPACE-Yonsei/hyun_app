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
*******************************************************************************/

/**
 * @file
 *
 * Main header file for the SAMPLE application
 */

#ifndef HYUN_APP_H
#define HYUN_APP_H

/*
** Required header files.
*/
#include "cfe.h"
#include "cfe_error.h"
#include "cfe_evs.h"
#include "cfe_sb.h"
#include "cfe_es.h"

#include "hyun_app_perfids.h"
#include "hyun_app_msgids.h"
#include "hyun_app_msg.h"

/***********************************************************************/
#define HYUN_APP_PIPE_DEPTH 32 /* Depth of the Command Pipe for Application */

#define HYUN_APP_NUMBER_OF_TABLES 1 /* Number of Table(s) */

/* Define filenames of default data images for tables */
#define HYUN_APP_TABLE_FILE "/cf/hyun_app_tbl.tbl"

#define HYUN_APP_TABLE_OUT_OF_RANGE_ERR_CODE -1

#define HYUN_APP_TBL_ELEMENT_1_MAX 10
/************************************************************************
** Type Definitions
*************************************************************************/

/*
** Global Data
*/

/*
새로 사용할 패킷의 데이터 형식을 정의
*/

/*
SB 설명을 위한 패킷. 실제 Mission에서 쓰이지 않음.
*/

typedef struct
{
    /*
    ** Command interface counters...
    */
    uint8 CmdCounter;
    uint8 ErrCounter;

    /*
    ** Housekeeping telemetry packet...
    */
    HYUN_APP_HkTlm_t HkTlm;

    /*
    SB Tutorial에 사용되는 telemetry packet...
    */
    HYUN_APP_TUTORIAL_t TutorialPacket;

    /*
    Software Bus Pipe를 정의한다.
    */
    CFE_SB_PipeId_t HYUN_PIPE_1; /* Variable to hold Pipe ID (i.e.- Handle) */

    /*
    ** Run Status variable used in the main processing loop
    */
    uint32 RunStatus;

    /*
    ** Operational data (not reported in housekeeping)...
    */
    CFE_SB_PipeId_t CommandPipe;

    /*
    ** Initialization data (not reported in housekeeping)...
    */
    char   PipeName[CFE_MISSION_MAX_API_LEN];
    uint16 PipeDepth;

    CFE_EVS_BinFilter_t EventFilters[HYUN_APP_EVENT_COUNTS];
    CFE_TBL_Handle_t    TblHandles[HYUN_APP_NUMBER_OF_TABLES];

} HYUN_APP_Data_t;

/****************************************************************************/
/*
** Local function prototypes.
**
** Note: Except for the entry point (HYUN_APP_Main), these
**       functions are not called from any other source module.
*/

/*이 App은 설명이 목적이므로 함수 밑에 관련 변수들을 선언할 것
실제 구현할 때는 #define 문은 위쪽에 넣어두자!*/

void  HYUN_APP_Main(void);
int32 HYUN_APP_Init(void);

//Software Bus 설명용 함수
int32 HYUN_APP_SB_TUTORIAL(void);
/*
App이 Initalization 될 때, app은 cFE에 자신이 쓸 pipe들을 알려줘야 한다.
먼저 헤더 파일 (hyun_app.h)에 관련 변수들을 정의하자.
*/
#define HYUN_PIPE_1_NAME   "HYUN_PIPE_1" //Pipe의 이름을 정의한다
#define HYUN_PIPE_1_DEPTH  (10) //Pipe Depth는 한 Pipe에 얼마나 많은 message가 들어갈 수 있는지 정의한다
#define HYUN_APP_TUTORIAL_LIMIT (10) //message limit은 특정 message ID를 가진 message가 한 pipe에 얼마나 들어갈 수 있는지를 정의한다.


void  HYUN_APP_ProcessCommandPacket(CFE_SB_Buffer_t *SBBufPtr);
void  HYUN_APP_ProcessGroundCommand(CFE_SB_Buffer_t *SBBufPtr);
int32 HYUN_APP_ReportHousekeeping(const CFE_MSG_CommandHeader_t *Msg);
int32 HYUN_APP_ResetCounters(const HYUN_APP_ResetCountersCmd_t *Msg);
int32 HYUN_APP_Process(const HYUN_APP_ProcessCmd_t *Msg);
int32 HYUN_APP_Noop(const HYUN_APP_NoopCmd_t *Msg);
void  HYUN_APP_GetCrc(const char *TableName);

int32 HYUN_APP_TblValidationFunc(void *TblData);

bool HYUN_APP_VerifyCmdLength(CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength);

int32 HYUN_APP_TEST_SB_RCV(void);
int32 HYUN_APP_TEST_SB_SEND(void);
int32 HYUN_APP_TEST_SB_INIT(void);


#endif /* HYUN_APP_H */
