#include <msp430.h>
#include <signal.h>

enum state {
  undefined,
  header,
  red,
  green,
};

static enum state lastState = undefined;
static unsigned char receivedChar = 0;
const static unsigned char maxDutyCycle = 255;

int main(void) {
  /* init watchdog & clock */
  WDTCTL = WDTPW | WDTHOLD; /* disable watchdog */
  BCSCTL1 = CALBC1_1MHZ;    /* Set DCO */
  DCOCTL = CALDCO_1MHZ;

  /* init pin modes and selections */

  /* UART */
  P1SEL = BIT1 + BIT2;
  P1SEL2 = BIT1 + BIT2;

  /* pins 2.1(green) and 2.5(blue) */
  P2DIR = BIT1 | BIT5;
  P2OUT = 0x00;
  P2SEL = BIT1 | BIT5;

  /* pin 1.6(red) */
  P1DIR |= BIT0 | BIT6;
  P1OUT = 0x00;
  P1SEL |= BIT6;

  /* init timers */
  TA0CTL |= TACLR;
  TA1CTL |= TACLR;

  TA1CCR0 = maxDutyCycle - 1;
  TA0CCR0 = maxDutyCycle - 1;

  TA0CTL = TASSEL_2 + ID_3; /* SMCLK, up mode */
  TA1CTL = TASSEL_2 + ID_3; /* SMCLK, up mode */

  /* init hardware UART */
  UCA0CTL1 |= UCSSEL_2; /* SMCLK */
  UCA0BR0 = 104;        /* 1MHz 9600 */
  UCA0BR1 = 0;          /* 1MHz 9600 */
  UCA0MCTL = UCBRS0;    /* Modulation UCBRSx = 1 */
  UCA0CTL1 &= ~UCSWRST; /* **Initialize USCI state machine** */
  IE2 |= UCA0RXIE;      /* Enable USCI_A0 RX interrupt */

  /* advanced timer settings */
  TA1CTL |= MC_1; /* up mode for both */
  TA0CTL |= MC_1;

  /* set/reset mode */
  TA1CCTL1 = OUTMOD_3;
  TA0CCTL1 = OUTMOD_3;
  TA1CCTL2 = OUTMOD_3;

  __bis_SR_register(LPM0_bits + GIE); /* Enter LPM0, interrupts enabled */

  for (;;) {
  }
}

interrupt(USCIAB0RX_VECTOR) USCI0RX_ISR() {
  receivedChar = UCA0RXBUF;
  switch (lastState) {
    case undefined:
      if (receivedChar == 0xFF) {
        lastState = header;
        P1OUT ^= BIT0; /* toggle LED to indicate received header */
      }
      break;
    case header:
      TA0CCR1 = receivedChar; /* sets red PWM */
      lastState = red;
      break;
    case red:
      TA1CCR1 = receivedChar; /* sets green PWM */
      lastState = green;
      break;
    case green:
      TA1CCR2 = receivedChar; /* sets blue PWM */
      lastState = undefined;
      break;
  }
}
