/*
 * File:   main.c
 * Author: dtek0068
 *
 * Created on 09 December 2020, 17:41
 */
    #define F_CPU 3333333
    #define SERVO_PWM_PERIOD (0x1046)
    #define SERVO_MAX (0x0138)
    #define SERVO_MIN (0x00dc)
    #define THRESHOLD (0x0208)  

    #include <avr/io.h>
    #include <util/delay.h>
    #include <avr/interrupt.h>
    
    //Value from LDR is stored here.
    //Shared with ISR
    volatile uint16_t adc_value;
    
    //Global variable where duty is stored.
    //SERVO_MAX = Maximum duty value (0 degrees)
    //SERVO_MIN = Minimum duty value
    uint16_t duty = SERVO_MAX;
    
    //Initialise functions
    void servo_init(void);
    void adc_init(void);
    void press_space(void);
    void release_space(void);
    
    
    // Initialise servo with TCA
    void servo_init(void)
    {
        // Route TCA0 PWM waveform to PORTB
        PORTMUX.TCAROUTEA |= PORTMUX_TCA0_PORTB_gc;

        // Setting PB2 as digital output
        PORTB.DIRSET = PIN2_bm;

        // TCA0 prescaler value to 208 kHz
        TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV16_gc;

        // Set single-slop PWM generation mode
        TCA0.SINGLE.CTRLB |= TCA_SINGLE_WGMODE_SINGLESLOPE_gc;

        // Set PWM period to 20 ms
        TCA0.SINGLE.PERBUF = SERVO_PWM_PERIOD;

        // Setting servo arm position to max value
        TCA0.SINGLE.CMP2BUF = SERVO_MAX;

        // Enable Compare Channel 2
        TCA0.SINGLE.CTRLB |= TCA_SINGLE_CMP2EN_bm;

        // Enable TCA0 
        TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;
    }
    
    // Initialise LDR with ADC 
    void adc_init(void)
    {
        // Setting PE0 as input
        PORTE.DIRCLR = PIN0_bm;
        
        // Disable input buffer
        PORTE.PIN0CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        
        // Disable pull-up
        PORTD.PIN0CTRL &= ~PORT_PULLUPEN_bm;
        
        // Setting prescaler of 16 and setting VDD reference voltage
        ADC0.CTRLC |= ADC_PRESC_DIV16_gc | ADC_REFSEL_VDDREF_gc; 
        
        // Selecting ADC channel to AN8 (PE0)
        ADC0.MUXPOS = ADC_MUXPOS_AIN8_gc;
        
        // Enabling ADC
        ADC0.CTRLA |= ADC_ENABLE_bm;
        
        // Enabling result ready interrupt
        ADC0.INTCTRL = ADC_RESRDY_bm;
        
        // Enabling free run mode
        ADC0.CTRLA |= ADC_FREERUN_bm;
        
        // Starting ADC conversion 
        ADC0.COMMAND = ADC_STCONV_bm;
        
        // Enabling global interrupts
        sei();
    }

    int main(void)
    { 
        // Initialise servo
        servo_init();
        
        // Initialise LDR
        adc_init();

        while (1)
        {
            // When conversion is done
            if(ADC0.INTFLAGS & ADC_RESRDY_bm)
            {
                // If LDR value is higher than threshold (cactus is detected)
                if(adc_value >= THRESHOLD)                     
                {
                    // Jump (Hit space)
                    press_space();  
                }
                //If not, release space press
                else
                {
                    // Release space
                    release_space();
                }   
            }
        }    
    }

    // ISR for result ready interrupt
    ISR(ADC0_RESRDY_vect)
    {
        // Read LDR value
        adc_value = ADC0.RES;
        
        // Clear interrupt flag
        ADC0.INTFLAGS = ADC_RESRDY_bm;
        
    }
    
    // Servo hits space (jump command)
    void press_space(void)
    {
        // Changing servo arm position to min
        while(duty>SERVO_MIN)
        {
            TCA0.SINGLE.CMP2BUF = duty;
            duty = duty-1;
            _delay_ms(1);
        }
    }
    
    // Servo release space
    void release_space(void)
    {
        // Setting servo arm position to max
        while(duty<SERVO_MAX)
        {
            TCA0.SINGLE.CMP2BUF = duty;
            duty = duty+1;
            _delay_ms(1);
        }
        
    }