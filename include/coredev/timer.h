/**-------------------------------------------------------------------------
@file	timer.h

@brief	Generic timer class

This generic implementation combines all available core timer in to one
device list. Each timer is indexed by DevNo starting from 0..max. Indexing
low frequency timer to high frequency.

For example MCU having 2 low frequency timers and 4 high frequency timers
are indexed as follow:

DevNo : 0..1 are low frequency timers
DevNo : 2..5 are high frequency timers

There are 2 ways to determine the start DevNo for high frequency timer

int HFreqDevNoStart = TimerGetLowFreqDevCount();
int HFreqDevNoStart = TimerGetHighFreqDevNo();

@author	Hoang Nguyen Hoan
@date	Sep. 7, 2017

@license

MIT License

Copyright (c) 2017 I-SYST inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

----------------------------------------------------------------------------*/

#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>
#include <string.h>

/// Clock source used for the timer
typedef enum __Timer_Clock_Src {
	TIMER_CLKSRC_DEFAULT,	//!< Device default clock source (auto select best src)
	TIMER_CLKSRC_LFRC,		//!< Low frequency internal RC oscillator
    TIMER_CLKSRC_HFRC,		//!< High frequency internal RC oscillator
    TIMER_CLKSRC_LFXTAL,	//!< Low frequency crystal
    TIMER_CLKSRC_HFXTAL		//!< High frequency crystal
} TIMER_CLKSRC;

/// Timer interrupt enable type
typedef enum __Timer_Interrupt_Enable {
	TIMER_INTEN_NONE,		//!< No interrupt
	TIMER_INTEN_TICK,		//!< Enable tick count interrupt
	TIMER_INTEN_OVR			//!< Enable tick count overflow interrupt
} TIMER_INTEN;

/// Timer trigger type
typedef enum __Timer_Trigger_Type {
    TIMER_TRIG_TYPE_SINGLE,		//!< Single shot trigger
    TIMER_TRIG_TYPE_CONTINUOUS	//!< Continuous trigger
} TIMER_TRIG_TYPE;

#define TIMER_EVT_TICK                          (1<<0)   //!< Timer tick counter event
#define TIMER_EVT_COUNTER_OVR                   (1<<1)   //!< Timer overflow event
#define TIMER_EVT_TRIGGER0                		(1<<2)   //!< Periodic timer event start at this bit

#define TIMER_EVT_TRIGGER(n)              		(1<<(n+2))	//!< Trigger event id

//class Timer;
typedef struct __Timer_Handle	TIMER;

/**
 * @brief	Timer event handler type.
 *
 * @param	Timer	: Pointer reference to Timer class generating the event
 * @param	Evt		: Event ID for which this callback is activated
 */
//typedef void (*TIMER_EVTCB)(Timer * const pTimer, uint32_t Evt);
typedef void (*TIMER_EVTCB)(TIMER * const pTimer, uint32_t Evt);

/**
 * @brief	Timer trigger handler type
 *
 * @param	Timer	: Pointer reference to Timer class generating the event
 * @param	TrigNo	: Trigger ID for which this callback is activated
 * @param   pContext: Pointer to user context (user private data, could be a class or structure)
 */
//typedef void (*TIMER_TRIGCB)(Timer * const pTimer, int TrigNo, void * const pContext);
typedef void (*TIMER_TRIGCB)(TIMER * const pTimer, int TrigNo, void * const pContext);

#pragma pack(push, 4)

typedef struct __Timer_Trigger_Info {
	TIMER_TRIG_TYPE Type;	//!< Trigger type
	uint64_t nsPeriod;		//!< Trigger period in nanosecond
	TIMER_TRIGCB Handler;	//!< Trigger event callback
	void *pContext;         //!<< Pointer to user private data to be passed to callback
} TIMER_TRIGGER;

/// @brief	Timer configuration data.
///
/// NOTE : Interrupt priority should be as high as possible
/// Timing precision would be lost if other interrupt preempt
/// the timer interrupt.  Adjust IntPrio base on requirement
///
typedef struct __Timer_Config {
    int             DevNo;      //!< Device number. Usually is the timer number indexed at 0
    TIMER_CLKSRC    ClkSrc;     //!< Clock source.  Not all timer allows user select clock source
    uint32_t        Freq;       //!< Frequency in Hz, 0 - to auto select max timer frequency
    int             IntPrio;    //!< Interrupt priority. recommended to use highest
    							//!< priority if precision timing is required
    TIMER_EVTCB     EvtHandler; //!< Interrupt handler
} TIMER_CFG;

struct __Timer_Handle {
    int DevNo;				//!< Timer device number.
	uint32_t Freq;			//!< Frequency in Hz
	uint64_t nsPeriod;		//!< Period in nsec
	uint64_t Rollover;     	//!< Rollover counter adjustment
	uint32_t LastCount;		//!< Last counter read value
	TIMER_EVTCB EvtHandler;	//!< Pointer to user event handler callback
	void *pObj;				//!< Pointer reference to Timer class

	void (*Disable)(TIMER * const pTimerDev);
	bool (*Enable)(TIMER * const pTimerDev);
	void (*Reset)(TIMER * const pTimerDev);

	/**
     * @brief   Get the current tick count.
     *
     * This function read the tick count with compensated overflow roll over
     *
     * @return  Total tick count since last reset
     */
	uint64_t (*GetTickCount)(TIMER * const pTimerDev);

	/**
	 * @brief	Set timer main frequency.
	 *
	 * This function allows dynamically changing the timer frequency.  Timer
	 * will be reset and restarted with new frequency
	 *
	 * @param 	Freq : Frequency in Hz
	 *
	 * @return  Real frequency
	 */
	uint32_t (*SetFrequency)(TIMER * const pTimerDev, uint32_t Freq);

	/**
	 * @brief	Get maximum available timer trigger event for the timer.
	 *
	 * @return	count
	 */
	int (*GetMaxTrigger)(TIMER * const pTimerDev);

	/**
	 * @brief	Get first available timer trigger index.
	 *
	 * This function returns the first available timer trigger to be used to with
	 * EnableTimerTrigger
	 *
	 * @return	success : Timer trigger index
	 * 			fail : -1
	 */
	int (*FindAvailTrigger)(TIMER * const pTimerDev);

	/**
	 * @brief   Disable timer trigger event.
	 *
	 * @param   TrigNo : Trigger number to disable. Index value starting at 0
	 */
	void (*DisableTrigger)(TIMER * const pTimer, int TrigNo);

    /**
	 * @brief	Enable a specific nanosecond timer trigger event.
	 *
	 * @param   TrigNo : Trigger number to enable. Index value starting at 0
	 * @param   nsPeriod : Trigger period in nsec.
	 * @param   Type     : Trigger type single shot or continuous
	 * @param	Handler	 : Optional Timer trigger user callback
	 * @param   pContext : Optional pointer to user private data to be passed
	 *                     to the callback. This could be a class or structure pointer.
	 *
	 * @return  real period in nsec based on clock calculation
	 */
	uint64_t (*EnableTrigger)(TIMER * const pTimerDev, int TrigNo, uint64_t nsPeriod,
							  TIMER_TRIG_TYPE Type, TIMER_TRIGCB const Handler,
							  void * const pContext);
};

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Timer initialization.
 *
 * This is a required implementation specific to each architecture.
 *
 * @param	pTimer	: Pointer to Timer device private data (timer handle)
 * @param	pCfg	: Pointer timer configuration data.
 *
 * @return
 * 			- true 	: Success
 * 			- false : Otherwise
 */
bool TimerInit(TIMER * const pTimer, const TIMER_CFG * const pCfg);

int TimerGetLowFreqDevCount();
int TimerGetHighFreqDevCount();
int TimerGetHighFreqDevNo();

/**
 * @brief   Turn off timer.
 *
 * This is used to disable timer for power saving. Call Enable() to
 * re-enable timer instead of full initialization sequence
 *
 * @param	pTimer	: Pointer to Timer device private data (timer handle)
 */
static inline void TimerDisable(TIMER * const pTimer) { pTimer->Disable(pTimer); }

/**
 * @brief   Turn on timer.
 *
 * This is used to re-enable timer after it was disabled for power
 * saving.  It normally does not go through full initialization sequence
 *
 * @param	pTimer	: Pointer to Timer device private data (timer handle)
 *
 * @return	true : Success
 */
static inline bool TimerEnable(TIMER * const pTimer)  { return pTimer->Enable(pTimer); }

/**
 * @brief   Reset timer.
 *
 * @param	pTimer	: Pointer to Timer device private data (timer handle)
 */
static inline void TimerReset(TIMER * const pTimer) { pTimer->Reset(pTimer); }
static inline int TimerGetMaxTrigger(TIMER * const pTimer) { return pTimer->GetMaxTrigger(pTimer); }
static inline uint64_t TimerGetTickCount(TIMER * const pTimer) { return pTimer->GetTickCount(pTimer); }
static inline uint32_t TimerGetFrequency(TIMER * const pTimer) { return pTimer->Freq; }
static inline uint32_t TimerSetFrequency(TIMER * const pTimer, uint32_t Freq) {
	return pTimer->SetFrequency(pTimer, Freq);
}
/**
 * @brief   Disable timer trigger event.
 *
 * @param   TrigNo : Trigger number to disable. Index value starting at 0
 */
static inline void TimerDisableTrigger(TIMER * const pTimer, int TrigNo) { pTimer->DisableTrigger(pTimer, TrigNo); }
static inline uint64_t nsTimerEnableTrigger(TIMER * const pTimer, int TrigNo, uint64_t nsPeriod,
										 	TIMER_TRIG_TYPE Type, TIMER_TRIGCB const Handler,
											void * const pContext) {
	return pTimer->EnableTrigger(pTimer, TrigNo, nsPeriod, Type, Handler, pContext);
}
static inline uint64_t msTimerEnableTimerTrigger(TIMER * const pTimer,  int TrigNo, uint32_t msPeriod,
										 	TIMER_TRIG_TYPE Type, TIMER_TRIGCB const Handler,
											void * const pContext) {
	return (uint32_t)((uint64_t )pTimer->EnableTrigger(pTimer, TrigNo, (uint64_t)((uint64_t)msPeriod * 1000000ULL),
													   Type, Handler, pContext) / 1000000ULL);
}

/**
 * @brief   Get current timer counter in millisecond.
 *
 * This function return the current timer in msec since last reset.
 *
 * @return  Counter in millisecond
 */
static inline uint32_t TimerGetMilisecond(TIMER * const pTimer) {
	return pTimer->GetTickCount(pTimer) * pTimer->nsPeriod / 1000000LL;
}

/**
 * @brief   Convert tick count to millisecond.
 *
 * @param   Count : Timer tick count value
 *
 * @return  Converted count in millisecond
 */
static inline uint32_t TimerTickToMilisecond(TIMER * const pTimer, uint64_t Count) {
	return Count * pTimer->nsPeriod / 1000000LL;
}

/**
 * @brief   Get current timer counter in microsecond.
 *
 * This function return the current timer in usec since last reset.
 *
 * @return  Counter in microsecond
 */
static inline uint32_t TimerGetMicrosecond(TIMER * const pTimer) {
	return pTimer->GetTickCount(pTimer) * pTimer->nsPeriod / 1000LL;
}

/**
 * @brief   Convert tick count to microsecond.
 *
 * @param   Count : Timer tick count value
 *
 * @return  Converted count in microsecond
 */
static inline uint32_t TimerTickToMicrosecond(TIMER * const pTimer, uint64_t Count) {
	return Count * pTimer->nsPeriod / 1000LL;
}

/**
 * @brief   Get current timer counter in nanosecond.
 *
 * This function return the current timer in nsec since last reset.
 *
 * @return  Counter in nanosecond
 */
static inline uint32_t TimerGetNanosecond(TIMER * const pTimer) {
	return pTimer->GetTickCount(pTimer) * pTimer->nsPeriod;
}

/**
 * @brief   Convert tick count to nanosecond
 *
 * @param   Count : Timer tick count value
 *
 * @return  Converted count in nanosecond
 */
static inline uint32_t TimerTickToNanosecond(TIMER * const pTimer, uint64_t Count) {
	return Count * pTimer->nsPeriod;
}

#ifdef __cplusplus
}

/// Timer base class
class Timer {
public:

	Timer() { vTimer.pObj = this; }

	virtual operator TIMER * const () { return &vTimer; }

    /**
     * @brief   Timer initialization.
     *
     * This is specific to each architecture.
     *
     * @param	Cfg	: Timer configuration data.
     *
     * @return
     * 			- true 	: Success
     * 			- false : Otherwise
     */
    virtual bool Init(const TIMER_CFG &Cfg) { return TimerInit(&vTimer, &Cfg); }

    /**
     * @brief   Turn on timer.
     *
     * This is used to re-enable timer after it was disabled for power
     * saving.  It normally does not go through full initialization sequence
     */
    virtual bool Enable() { return vTimer.Enable(&vTimer); }

    /**
     * @brief   Turn off timer.
     *
     * This is used to disable timer for power saving. Call Enable() to
     * re-enable timer instead of full initialization sequence
     */
    virtual void Disable() { vTimer.Disable(&vTimer); }

    /**
     * @brief   Reset timer.
     */
    virtual void Reset() { vTimer.Reset(&vTimer); }

    /**
     * @brief   Get the current tick count.
     *
     * This function read the tick count with compensated overflow roll over
     *
     * @return  Total tick count since last reset
     */
	virtual uint64_t TickCount() { return vTimer.GetTickCount(&vTimer); }

	/**
	 * @brief   Get current timer frequency
	 *
	 * @return  Timer frequency in Hz
	 */
	virtual uint32_t Frequency(void) { return vTimer.Freq; }

	/**
	 * @brief	Set timer main frequency.
	 *
	 * This function allows dynamically changing the timer frequency.  Timer
	 * will be reset and restarted with new frequency
	 *
	 * @param 	Freq : Frequency in Hz
	 *
	 * @return  Real frequency
	 */
	virtual uint32_t Frequency(uint32_t Freq) { return vTimer.SetFrequency(&vTimer, Freq); }

	/**
	 * @brief	Get maximum available timer trigger event for the timer.
	 *
	 * @return	count
	 */
	int MaxTimerTrigger() { return vTimer.GetMaxTrigger(&vTimer); }

	/**
	 * @brief	Enable a specific nanosecond timer trigger event.
	 *
	 * @param   TrigNo : Trigger number to enable. Index value starting at 0
	 * @param   nsPeriod : Trigger period in nsec.
	 * @param   Type     : Trigger type single shot or continuous
	 * @param	Handler	 : Optional Timer trigger user callback
	 * @param   pContext : Optional pointer to user private data to be passed
	 *                     to the callback. This could be a class or structure pointer.
	 *
	 * @return  real period in nsec based on clock calculation
	 */
	virtual uint64_t EnableTimerTrigger(int TrigNo, uint64_t nsPeriod, TIMER_TRIG_TYPE Type,
	                                    TIMER_TRIGCB const Handler = NULL, void * const pContext = NULL) {
		return vTimer.EnableTrigger(&vTimer, TrigNo, nsPeriod, Type, Handler, pContext);
	}

    /**
	 * @brief	Enable millisecond timer trigger event.
	 *
	 * @param   TrigNo : Trigger number to enable. Index value starting at 0
	 * @param   msPeriod : Trigger period in msec.
	 * @param   Type     : Trigger type single shot or continuous
	 * @param	Handler	 : Optional Timer trigger user callback
	 * @param   pContext : Optional pointer to user private data to be passed
	 *                     to the callback. This could be a class or structure pointer.
	 *
	 * @return  real period in msec based on clock calculation
	 */
	virtual uint32_t EnableTimerTrigger(int TrigNo, uint32_t msPeriod, TIMER_TRIG_TYPE Type,
	                           	   	    TIMER_TRIGCB const Handler = NULL, void * const pContext = NULL)
	{
		return (uint32_t)((uint64_t )EnableTimerTrigger(TrigNo, (uint64_t)((uint64_t)msPeriod * 1000000ULL), Type, Handler, pContext) / 1000000ULL);
	}

	/**
	 * @brief	Enable nanosecond timer trigger event.
	 *
	 * @param   nsPeriod : Trigger period in nsec.
	 * @param   Type     : Trigger type single shot or continuous
	 * @param	Handler	 : Optional Timer trigger user callback
     * @param   pContext : Optional pointer to user private data to be passed
     *                     to the callback. This could be a class or structure pointer.
	 *
	 * @return  Timer trigger ID on success
	 * 			-1 : Failed
	 */
	virtual int EnableTimerTrigger(uint64_t nsPeriod, TIMER_TRIG_TYPE Type,
	                               TIMER_TRIGCB const Handler = NULL, void * const pContext = NULL);

	/**
	 * @brief	Enable millisecond timer trigger event.
	 *
	 * @param   msPeriod : Trigger period in msec.
	 * @param   Type     : Trigger type single shot or continuous
	 * @param	Handler	 : Optional Timer trigger user callback
     * @param   pContext : Optional pointer to user private data to be passed
     *                     to the callback. This could be a class or structure pointer.
	 *
	 * @return  Timer trigger ID on success
	 * 			-1 : Failed
	 */
	int EnableTimerTrigger(uint32_t msPeriod, TIMER_TRIG_TYPE Type,
	                       TIMER_TRIGCB const Handler = NULL, void * const pContext = NULL);

	/**
	 * @brief   Disable timer trigger event.
	 *
	 * @param   TrigNo : Trigger number to disable. Index value starting at 0
	 */
    virtual void DisableTimerTrigger(int TrigNo) { vTimer.DisableTrigger(&vTimer, TrigNo); }

    /**
     * @brief   Get current timer counter in millisecond.
     *
     * This function return the current timer in msec since last reset.
     *
     * @return  Counter in millisecond
     */
	virtual uint32_t mSecond() { return vTimer.GetTickCount(&vTimer) * vTimer.nsPeriod / 1000000LL; }

	/**
	 * @brief   Convert tick count to millisecond.
	 *
	 * @param   Count : Timer tick count value
	 *
	 * @return  Converted count in millisecond
	 */
	virtual uint32_t mSecond(uint64_t Count) { return Count * vTimer.nsPeriod / 1000000LL; }

	/**
     * @brief   Get current timer counter in microsecond.
     *
     * This function return the current timer in usec since last reset.
     *
     * @return  Counter in microsecond
     */
	virtual uint32_t uSecond() { return vTimer.GetTickCount(&vTimer) * vTimer.nsPeriod / 1000LL; }

	/**
	 * @brief   Convert tick count to microsecond.
	 *
	 * @param   Count : Timer tick count value
	 *
	 * @return  Converted count in microsecond
	 */
	virtual uint32_t uSecond(uint64_t Count) { return Count * vTimer.nsPeriod / 1000LL; }

	/**
     * @brief   Get current timer counter in nanosecond.
     *
     * This function return the current timer in nsec since last reset.
     *
     * @return  Counter in nanosecond
     */
	virtual uint32_t nSecond() { return vTimer.GetTickCount(&vTimer) * vTimer.nsPeriod; }

	/**
     * @brief   Convert tick count to nanosecond
     *
     * @param   Count : Timer tick count value
     *
     * @return  Converted count in nanosecond
     */
	virtual uint32_t nSecond(uint64_t Count) { return Count * vTimer.nsPeriod; }

	/**
	 * @brief	Get first available timer trigger index.
	 *
	 * This function returns the first available timer trigger to be used to with
	 * EnableTimerTrigger
	 *
	 * @return	success : Timer trigger index
	 * 			fail : -1
	 */
	int FindAvailTimerTrigger(void) { return vTimer.FindAvailTrigger(&vTimer); }

protected:

#if 0
	TIMER_EVTCB vEvtHandler;//!< Pointer to user event handler callback

    int      vDevNo;		//!< Timer device number
	uint32_t vFreq;			//!< Frequency in Hz
	uint64_t vnsPeriod;		//!< Period in nsec
	uint64_t vRollover;     //!< Rollover counter adjustment
	uint32_t vLastCount;	//!< Last counter read value
#endif
private:
	TIMER vTimer;
};

#endif  //  __cplusplus

#endif // __TIMER_H__
