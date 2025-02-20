
#include "x_types.hpp"
#include "x_files.hpp"
#include "gcnaud_audio.hpp"
#include "gcnaud_spu.hpp"
#include "gcnaud_container.hpp"
#include "gcnaud_cfx.hpp"
#include "gcnaud_host.hpp"
#include "gcnaud_hoststream.hpp"
#include "x_threads.hpp"

gcn_audio_vars  g_gcnAudio;
#define INITIAL_SURROUND_DELAY  1   // Number of update cycles before we kick in the surround channel

void gcnaudio_PeriodicUpdate(void);

//-----------------------------------------------------------------------------
static void gcnaudio_Lock(void)
{
    g_gcnAudio.m_pMutex->Enter();
}

//-----------------------------------------------------------------------------
static void gcnaudio_Unlock(void)
{
    g_gcnAudio.m_pMutex->Exit();
}

//-----------------------------------------------------------------------------
void    gcnaudio_DupState(cfx_state *pDest,cfx_state *pParent,cfx_state *pOurs)
{
    ASSERT(pDest);
    ASSERT(pParent);
    ASSERT(pOurs);

    pDest->m_Volume     = (pParent->m_Volume  * pOurs->m_Volume ) / AUD_FIXED_POINT_1;
    pDest->m_Pitch      = (pParent->m_Pitch   * pOurs->m_Pitch  ) / AUD_FIXED_POINT_1;
    pDest->m_RearVol    = (pParent->m_RearVol * pOurs->m_RearVol) / AUD_FIXED_POINT_1;
    pDest->m_Pan        = pParent->m_Pan;
}

//-----------------------------------------------------------------------------
void *gcnaudio_Init(audio_init *pInit)
{
    (void)pInit;
    x_memset(&g_gcnAudio,0,sizeof(g_gcnAudio));

	g_gcnAudio.m_pMutex = new xmutex;

    g_gcnAudio.m_TimerTick       = 0;
    g_gcnAudio.m_pUpdateThread  = new xthread(gcnaudio_PeriodicUpdate,"Audio Periodic Update",8192,1);

    //
    // We need to keep the initialization order from the bottom up
    //
    spu_Init();
    voice_Init();
    voicestream_Init(AUD_MAX_STREAMS,AUD_STREAM_BUFFER_SIZE);
    cfx_Init();
    gcncontainer_Init();

    g_gcnAudio.m_MasterState.m_Keyoff0 = 0;
    g_gcnAudio.m_MasterState.m_Keyoff1 = 0;
    g_gcnAudio.m_MasterState.m_Keyon0  = 0;
    g_gcnAudio.m_MasterState.m_Keyon1  = 0;
    g_gcnAudio.m_MasterState.m_State.m_LeftVolume = 0;
    g_gcnAudio.m_MasterState.m_State.m_RightVolume = 0;
    gcnaudio_Unlock();
    return NULL;
}


void gcnaudio_Kill(void)
{
}

f32 audio_time,audio_max;
xbool DumpAudioList = FALSE;
f32 dt;
//-----------------------------------------------------------------------------
void gcnaudio_PeriodicUpdate(void)
{
    f32 DeltaTime;

    cfx *pThisCfx,*pNextCfx;
    s32 status;
    f32 ProcessingTime;
    cfx_state State;
    s32 NumProcessed;
    xtimer timer;

    audio_time = 0;
    audio_max  = 0;
    timer.Reset();
    timer.Start();
    while (1)
    {
        gcnaudio_Lock();
        DeltaTime = timer.ReadMs();
        dt = DeltaTime;
        timer.Reset();
        timer.Start();
        if (DeltaTime < 0)
        {
            BREAK;
        }

#ifdef AUDIO_DEBUG
        {
            static s32 i=1;
            static s32 tick=0;
            static s32 min,max;
            static s32 ProcessingAvg;

            i--;

            if (DeltaTime > max)
                max = DeltaTime;
            if (DeltaTime < min)
                min = DeltaTime;

            ProcessingAvg+=ProcessingTime;
            if (i<=0)
            {
                printf("[%d] Min=%d,max=%d,current=%d,processing=%d\n",tick,min,max,DeltaTime,ProcessingAvg/100);
                tick++;
                i=100;
                min = 10000;
                max = 0;
                ProcessingAvg=0;
            }
        }
#endif

        spu_ReadMaster(&g_gcnAudio.m_CurrentMasterState);
        pThisCfx = g_gcnAudio.m_pRootCfx;
        NumProcessed = 0;
        g_gcnAudio.m_VoicesToKill = 0;
        g_gcnAudio.m_LowestKillPriority = 256;
        if (DumpAudioList)
        {
            x_DebugMsg("Audio List dump for tick %d\n",g_gcnAudio.m_TimerTick);
        }
        while (pThisCfx)
        {
            NumProcessed++;
            State = pThisCfx->m_State;

            status = cfx_Update(pThisCfx,&State,DeltaTime);
            // If the cfx expired on this update, then we need to unlink it from the
            // master list
            if (status)
            {
                cfx_RemoveFromUpdate(pThisCfx);
                pNextCfx = pThisCfx->m_pNext;
                cfx_Free(pThisCfx);
            }
            else
            {
                if (DumpAudioList)
                {
                    x_DebugMsg("IOP Audio: id=0x%x,cfxid=%d, flags=%04x, count=%d\n",pThisCfx->m_Identifier,
                                                        pThisCfx->m_State.m_CfxId,
                                                        pThisCfx->m_Flags,
                                                        pThisCfx->m_Count);
                }
                pNextCfx = pThisCfx->m_pNext;
            }
            pThisCfx = pNextCfx;
        }

#if 0
        {
            static s32 i=10;
            static s32 max=0,min=10;

            i--;
            if (NumProcessed < min) min=NumProcessed;
            if (NumProcessed > max) max=NumProcessed;
            if (i<=0)
            {
                s32 memfree;
                i=100;
                memfree = QueryTotalFreeMemSize() - (6 *1048576);
                //printf("AudioMgr: Processs min=%d,max=%d; %d bytes free\n",min,max,memfree);
                min=1000;
                max=0;
            }
        }
#endif

        if (g_gcnAudio.m_VoicesToKill)
        {
            voice_Cull(g_gcnAudio.m_VoicesToKill);
        }
        g_gcnAudio.m_TimerTick += DeltaTime;
        //
        // Now calculate the time for the next audio update taking in to
        // account any slew caused by this audio update cycle.
        //
        ProcessingTime = timer.ReadMs();
        audio_time = ProcessingTime;
        if (audio_time > audio_max)
        {
            audio_max = audio_time;
        }

        spu_WriteMaster(&g_gcnAudio.m_MasterState);

        DumpAudioList = FALSE;
        gcnaudio_Unlock();
        DeltaTime = 10.5f - timer.ReadMs();
        if ( (DeltaTime <=0.0f) || (DeltaTime > 11.0f) )
        {
            DeltaTime = 10.0f;
        }
        x_DelayThread((s32)DeltaTime);
    }
}


//-----------------------------------------------------------------------------
void gcnaudio_ReadBuffer(void *pData,s32 length)
{
    x_memcpy(pData,g_gcnAudio.m_pInBuffer,length);
    g_gcnAudio.m_pInBuffer+=length;

}

//-----------------------------------------------------------------------------
void gcnaudio_WriteBuffer(void *pData,s32 length)
{
    //
    // Check for overflow and just wrap if we do.
    //
    x_memcpy(g_gcnAudio.m_pOutBuffer,pData,length);
    g_gcnAudio.m_pOutBuffer += length;
}

//-----------------------------------------------------------------------------
void gcnaudio_InitBuffer(byte *pBuffer)
{
    g_gcnAudio.m_pInBuffer = pBuffer;
    g_gcnAudio.m_pOutBuffer = pBuffer;
}
//-----------------------------------------------------------------------------
void gcnaudio_Update(u8 *pBuffer)
{
    s8 Command;
    cfx *pCfx;
    s32 done;
    f32 volume;

    cfx_state UpdatePacket;

    gcnaudio_InitBuffer(pBuffer);

    done = FALSE;
    //audio_Lock();
    while (!done)
    {
        gcnaudio_ReadBuffer(&Command,sizeof(s8));

        switch (Command)
        {
        case AUD_COMMAND_UPDATE_CFX:
            gcnaudio_ReadBuffer(&UpdatePacket,sizeof(cfx_state));
            if (UpdatePacket.m_CfxId != -1)
            {
                volume = gcncontainer_GetVolume(UpdatePacket.m_CfxId & 0xff000000);
//                UpdatePacket.m_Volume = (UpdatePacket.m_Volume * volume) / AUD_FIXED_POINT_1;
            }

            pCfx = cfx_Find(UpdatePacket.m_Id);
            if (!pCfx)
            {
                // An update was requested of a cfx that had already completed.
                // So we tell the host that it's completed again (it may have
                // missed it on the last run if the output buffer was full)
                if ( (UpdatePacket.m_Id <= g_gcnAudio.m_TopCfxId) &&
                     (UpdatePacket.m_UniqueId <= g_gcnAudio.m_TopUniqueId) )
                {
#if 0
                    if (UpdatePacket.m_CfxId == 0x200010F)
                    {
                        Kprintf("[%d] %08x: Thrust update requested but has been stopped\n",g_gcnAudio.m_TimerTick,UpdatePacket.m_Id);
                    }
#endif
                    gcnaudio_WriteBuffer(&UpdatePacket.m_Id,sizeof(s32));
                    break;
                }
                else
                {
                    // Spawn new cfx if the update we're interested is for
                    // a cfx that isn't "alive" yet.
                    if ( (UpdatePacket.m_CfxId != -1) && (UpdatePacket.m_UniqueId >= g_gcnAudio.m_TopUniqueId) )
                    {
#if 0
                        if (UpdatePacket.m_CfxId == 0x200010F)
                        {
                            Kprintf("[%d] %08x: Thrust effect started\n",g_gcnAudio.m_TimerTick,UpdatePacket.m_Id);
                        }
#endif

                        gcnaudio_Lock();
                        pCfx = cfx_Alloc(UpdatePacket.m_CfxId,&UpdatePacket);
                        pCfx->m_Identifier = UpdatePacket.m_Id;
                        ASSERT(UpdatePacket.m_Id > g_gcnAudio.m_TopCfxId);
                        g_gcnAudio.m_TopCfxId = UpdatePacket.m_Id;
                        ASSERT(pCfx);
                        pCfx->m_pNext = g_gcnAudio.m_pRootCfx;
                        g_gcnAudio.m_pRootCfx = pCfx;
                        if (UpdatePacket.m_UniqueId > g_gcnAudio.m_TopUniqueId)
                        {
                            g_gcnAudio.m_TopUniqueId = UpdatePacket.m_UniqueId;
                        }
                        gcnaudio_Unlock();
                    }
                }
            }
            else
            {
                if (UpdatePacket.m_CfxId==-1)
                {
#if 0
                    if (pCfx->m_OriginalState.m_CfxId == 0x200010F)
                    {
                        Kprintf("[%d] %08x: Thrust effect stop requested\n",g_gcnAudio.m_TimerTick,UpdatePacket.m_Id);
                    }
#endif
                    pCfx->m_Status = CFXSTAT_STOPPING;
                }
                cfx_UpdateState(pCfx,&UpdatePacket);
            }
            break;
        case AUD_COMMAND_END:
            done = TRUE;
            break;

        case AUD_COMMAND_UPDATE_MASTER:
            gcnaudio_ReadBuffer(&g_gcnAudio.m_MasterState.m_State,sizeof(g_gcnAudio.m_MasterState.m_State));
            break;

        default:
            ASSERT(FALSE);
        }
    }

    // End of killed cfx id's passed back is ended with a 0. We will never
    // have a valid cfx id of 0
    done = 0;
    gcnaudio_WriteBuffer(&done,sizeof(s32));
//    audio_Unlock();
}

#if 0
//-----------------------------------------------------------------------------
void *gcnaudio_Dispatch(u32 Command,void *Data,s32 Size)
{
    s32 *pData;
    static s32 SimpleReturn;

    (void)Size;
    pData = (s32 *)Data;

    switch(Command)
    {
//-------------------------------------------------
    case AUDCMD_INIT:
        return gcnaudio_Init((audio_init *)pData);
        break;
//-------------------------------------------------
    case AUDCMD_KILL:

        break;
//-------------------------------------------------
    case AUDCMD_LOADCONTAINER:
        return gcncontainer_Load((char *)Data);
        break;
//-------------------------------------------------
    case AUDCMD_GETCONTAINERHEADER:
        return gcncontainer_Header((container_hdr_request *)Data);
        break;
//-------------------------------------------------
    case AUDCMD_UNLOADCONTAINER:
        SimpleReturn = gcncontainer_Unload(*(s32 *)Data);
        return &SimpleReturn;
        break;
//-------------------------------------------------
    case AUDCMD_CONTAINERVOLUME:
        gcncontainer_SetVolume(pData[0],pData[1]);
        return &SimpleReturn;
        break;
//-------------------------------------------------
    case AUDCMD_UPDATE:
        gcnaudio_Update((u8*)Data);
        return Data;
        break;
//-------------------------------------------------
    case AUDCMD_STREAM_INIT:
        gcnaudio_Lock();
        hoststream_Init(*pData);
        gcnaudio_Unlock();
        break;
//-------------------------------------------------
    case AUDCMD_STREAM_KILL:
        gcnaudio_Lock();
        hoststream_Kill();
        gcnaudio_Unlock();
        break;
//-------------------------------------------------
    case AUDCMD_STREAM_SEND_DATA_LEFT:
        gcnaudio_Lock();
        SimpleReturn = hoststream_Update((u8 *)pData,Size,0);
        gcnaudio_Unlock();
        return &SimpleReturn;
        break;
//-------------------------------------------------
    case AUDCMD_STREAM_SEND_DATA_RIGHT:
        gcnaudio_Lock();
        hoststream_Update((u8 *)pData,Size,1);
        gcnaudio_Unlock();
        return &SimpleReturn;
        break;
//-------------------------------------------------
    default:
        x_DebugMsg("AudioDispatch: Invalid audio dispatch code 0x%08x\n",Command);
        break;
    }
    return Data;
}
#endif
