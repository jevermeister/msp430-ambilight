/* MSP430 Ambilight control code
  Copyright (C) [2011]  [Jan Schlemminger]
  This program is free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of 
  the License, or (at your option) any later version.
  This program is distributed in the hope that it will be useful, 
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
  See the GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <msp430.h>
#include <signal.h>

enum state
{
  undefined,
  header,
  red,
  green,
};

enum state g_laststate = undefined;
unsigned char g_receivedchar = 0;

const unsigned int g_maxdutycycle = 255;

/* prototypes */
unsigned int calcdutycycle (unsigned int value);

int
main (void)
{
  /* init watchdog & clock */
  WDTCTL = WDTPW | WDTHOLD;	/* disable watchdog */
  BCSCTL1 = CALBC1_1MHZ;	/* Set DCO */
  DCOCTL = CALDCO_1MHZ;

  /* init pin modes and selections */

  /* UART */
  P1SEL = BIT1 + BIT2;
  P1SEL2 = BIT1 + BIT2;

  /* pins 2.1(red) and 2.5(blue) */
  P2DIR = BIT1 | BIT5;
  P2OUT = 0x00;
  P2SEL = BIT1 | BIT5;

  /* pin 1.6(green) */
  P1DIR |= BIT0 | BIT6;
  P1OUT = 0x00;
  P1SEL |= BIT6;

  /* init timers */
  TA0CTL |= TACLR;
  TA1CTL |= TACLR;

  TA1CCR0 = g_maxdutycycle - 1;
  TA0CCR0 = g_maxdutycycle - 1;

  TA0CTL = TASSEL_2 + ID_3;	/* SMCLK, up mode */
  TA1CTL = TASSEL_2 + ID_3;	/* SMCLK, up mode */

  /* init hardware UART */
  UCA0CTL1 |= UCSSEL_2;		/* SMCLK */
  UCA0BR0 = 104;		/* 1MHz 9600 */
  UCA0BR1 = 0;			/* 1MHz 9600 */
  UCA0MCTL = UCBRS0;		/* Modulation UCBRSx = 1 */
  UCA0CTL1 &= ~UCSWRST;		/* **Initialize USCI state machine** */
  IE2 |= UCA0RXIE;		/* Enable USCI_A0 RX interrupt */

  /* advanced timer settings */
  TA1CTL |= MC_1;		/* up mode for both */
  TA0CTL |= MC_1;

  /* set/reset mode */
  TA1CCTL1 = OUTMOD_3;
  TA0CCTL1 = OUTMOD_3;
  TA1CCTL2 = OUTMOD_3;

  __bis_SR_register (LPM0_bits + GIE);	/* Enter LPM0, interrupts enabled */

  for (;;)
    {

    }
}

interrupt (USCIAB0RX_VECTOR) USCI0RX_ISR ()
{
  g_receivedchar = UCA0RXBUF;
  if (g_receivedchar == 0xFF && g_laststate == undefined)
    {
      g_laststate = header;
      /* P1OUT ^=BIT0; */
    }
  else if (g_laststate != undefined)
    {
      switch (g_laststate)
	{
	case header:
	  TA1CCR1 = calcdutycycle (g_receivedchar);	/* red */
	  g_laststate = red;
	  break;
	case red:
	  TA0CCR1 = calcdutycycle (g_receivedchar);	/* green */
	  g_laststate = green;
	  break;
	case green:
	  TA1CCR2 = calcdutycycle (g_receivedchar);	/* blue */
	  g_laststate = undefined;
	  break;
	}
    }
}

unsigned int
calcdutycycle (unsigned int value)
{
  if (value < 0)
    {
      return 0;
    }
  else if (value > g_maxdutycycle)
    {
      return g_maxdutycycle;
    }
  return value;
}
