/*
 * Copyright (C) 2012-2020  ASPEED Technology Inc.
 * Ryan Chen <ryan_chen@aspeedtech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>

#include <asm/arch/platform.h>


DECLARE_GLOBAL_DATA_PTR;

#define TIMER_LOAD_VAL 0xffffffff

/* macro to read the 32 bit timer */
#define READ_TIMER (*(volatile ulong *)(AST_TIMER_BASE))

static ulong timestamp;
static ulong lastdec;

void reset_timer_masked (void)
{
	/* reset time */
	lastdec = READ_TIMER;  /* capure current decrementer value time */
	timestamp = 0;	       /* start "advancing" time stamp from 0 */
}

int timer_init (void)
{
	*(volatile ulong *)(AST_TIMER_BASE + 4)    = TIMER_LOAD_VAL;
	*(volatile ulong *)(AST_TIMER_BASE + 0x30) = 0x3;		/* enable timer1 */

	/* init the timestamp and lastdec value */
	reset_timer_masked();

	return 0;
}
ulong get_timer_masked (void)
{
        ulong now = READ_TIMER;         /* current tick value */

        if (lastdec >= now) {           /* normal mode (non roll) */
                /* normal mode */
                timestamp += lastdec - now; /* move stamp fordward with absoulte diff ticks */
        } else {                        /* we have overflow of the count down timer */
                /* nts = ts + ld + (TLV - now)
                 * ts=old stamp, ld=time that passed before passing through -1
                 * (TLV-now) amount of time after passing though -1
                 * nts = new "advancing time stamp"...it could also roll and cause problems.
                 */
                timestamp += lastdec + TIMER_LOAD_VAL - now;
        }
        lastdec = now;

        return timestamp;
}

/*
 * timer without interrupts
 */
ulong get_timer (ulong base)
{
	return get_timer_masked () - base;
}

/* delay x useconds AND preserve advance timestamp value */
void __udelay (unsigned long usec)
{
		ulong tmo, tmp;

		if(usec >= 1000){				/* if "big" number, spread normalization to seconds */
				tmo = usec / 1000;		/* start to normalize for usec to ticks per sec */
				tmo *= CONFIG_SYS_HZ;			/* find number of "ticks" to wait to achieve target */
				tmo /= 1000;			/* finish normalize. */
		}else{							/* else small number, don't kill it prior to HZ multiply */
				tmo = usec * CONFIG_SYS_HZ;
				tmo /= (1000*1000);
		}

		tmp = get_timer (0);			/* get current timestamp */
		if( (tmo + tmp + 1) < tmp ) 	/* if setting this fordward will roll time stamp */
				reset_timer_masked ();	/* reset "advancing" timestamp to 0, set lastdec value */
		else
				tmo += tmp; 			/* else, set advancing stamp wake up time */

		while (get_timer_masked () < tmo)/* loop till event */
				/*NOP*/;
}


/* waits specified delay value and resets timestamp */
void udelay_masked (unsigned long usec)
{
	ulong tmo;
	ulong endtime;
	signed long diff;

	if (usec >= 1000) {		/* if "big" number, spread normalization to seconds */
		tmo = usec / 1000;	/* start to normalize for usec to ticks per sec */
		tmo *= CONFIG_SYS_HZ;		/* find number of "ticks" to wait to achieve target */
		tmo /= 1000;		/* finish normalize. */
	} else {			/* else small number, don't kill it prior to HZ multiply */
		tmo = usec * CONFIG_SYS_HZ;
		tmo /= (1000*1000);
	}

	endtime = get_timer_masked () + tmo;

	do {
		ulong now = get_timer_masked ();
		diff = endtime - now;
	} while (diff >= 0);
}

void reset_timer (void)
{
	reset_timer_masked ();
}



void set_timer (ulong t)
{
	timestamp = t;
}

/*
 * This function is derived from PowerPC code (read timebase as long long).
 * On M68K it just returns the timer value.
 */
unsigned long long get_ticks(void)
{
	return get_timer(0);
}

unsigned long usec2ticks(unsigned long usec)
{
	return get_timer(usec);
}

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On M68K it returns the number of timer ticks per second.
 */
ulong get_tbclk(void)
{
	ulong tbclk;

	tbclk = CONFIG_SYS_HZ;
	return tbclk;
}
