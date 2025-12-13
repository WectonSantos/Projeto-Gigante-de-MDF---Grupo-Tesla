#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h> 

uint8_t vidas = 3;
uint8_t fimDeJogo = 0;
volatile char comandoAtual = 'T'; 

// 0 = normal, 1 = girando por 1s, 2 = bloqueado 5s
volatile uint8_t estado = 0;
volatile uint32_t contador = 0;
volatile uint8_t seqRequisitada = 0;

// ---------------- LEDS VIDA ----------------
void ledsVida(){
	DDRC |= (1 << PC0) | (1 << PC1) | (1 << PC2);

	PORTC |= (1<<PC0) | (1<<PC1) | (1<<PC2);
}

void atualizaLedsVida() {
	if (vidas >= 3){
		PORTC |= (1<<PC0) | (1<<PC1) | (1<<PC2);
	}
	else if (vidas == 2){
		PORTC &= ~(1<<PC0);
		PORTC |= (1<<PC1) | (1<<PC2);
	}
	else if (vidas == 1){
		PORTC &= ~((1<<PC0)|(1<<PC1));
		PORTC |= (1<<PC2);
	}
	else {
		PORTC &= ~( (1<<PC0) | (1<<PC1) | (1<<PC2) );
		fimDeJogo = 1;
	}
}


// ---------------- MOTORES (SEM PWM) ----------------
void motores(){
	DDRB |= (1<<PB3) | (1<<PB4);   // Motor direito
	DDRD |= (1<<PD5) | (1<<PD6);   // Motor esquerdo

	// Inicialmente desligado
	PORTB &= ~((1<<PB3)|(1<<PB4));
	PORTD &= ~((1<<PD5)|(1<<PD6));
}

void pararMotores(){
	PORTB &= ~((1<<PB3)|(1<<PB4));
	PORTD &= ~((1<<PD5)|(1<<PD6));
}

void ligaMotoresHorario(){
	// Motor esquerdo horário
	PORTD |=  (1<<PD5);
	PORTD &= ~(1<<PD6);

	// Motor direito horário
	PORTB |=  (1<<PB3);
	PORTB &= ~(1<<PB4);
}

void ligaMotoresAntiHorario(){
	// Esquerdo anti-horário
	PORTD |=  (1<<PD6);
	PORTD &= ~(1<<PD5);

	// Direito anti-horário
	PORTB |=  (1<<PB4);
	PORTB &= ~(1<<PB3);
}

void giraEsquerda(){
  	// Esquerdo horário
	PORTD |=  (1<<PD5);
	PORTD &= ~(1<<PD6);

	// Direito anti-horário
	PORTB &=  ~(1<<PB4);
	PORTB &= ~(1<<PB3);

}

void giraDireita(){
	// Esquerdo anti-horário
	PORTD &=  ~(1<<PD6);
	PORTD &= ~(1<<PD5);

	// Direito horário
	PORTB |=  (1<<PB3);
	PORTB &= ~(1<<PB4);
}

void girar360(){
	// Esquerdo horário
	PORTD |=  (1<<PD5);
	PORTD &= ~(1<<PD6);

	// Direito anti-horário
	PORTB |=  (1<<PB4);
	PORTB &= ~(1<<PB3);
}



// ---------------- LASER ----------------
void laser(){
	DDRC |= (1<<PC3);
}

void timer1_init(){
	TCCR1B |= (1<<WGM12);  
	OCR1A = 15624;
	TIMSK1 |= (1<<OCIE1A);
	TCCR1B |= (1<<CS12)|(1<<CS10);
	sei();
}

void timer1_stop(){
	TIMSK1 &= ~(1<<OCIE1A);
	TCCR1B = 0;
}

ISR(TIMER1_COMPA_vect){
	if (estado == 0){
		PORTC ^= (1<<PC3);
	}
}


// ---------------- TIMER2 - CONTROLE DE SEQUÊNCIA ----------------
void timer2_init(){
	TCCR2A = (1<<WGM21);
	OCR2A = 249;
	TIMSK2 |= (1<<OCIE2A);
	TCCR2B = (1<<CS22);
	sei();
}

void timer2_stop(){
	TIMSK2 &= ~(1<<OCIE2A);
	TCCR2B = 0;
}

ISR(TIMER2_COMPA_vect){
	if (estado == 0 && seqRequisitada == 0) return;

	contador++;

	if (estado == 1){
		if (contador >= 1000){
			pararMotores();
			PORTC &= ~(1<<PC3);
			timer1_stop();
			estado = 2;
			contador = 0;
		}
	}
	else if (estado == 2){
		if (contador >= 5000){
			estado = 0;
			seqRequisitada = 0;
			contador = 0;
			timer1_init();
			timer2_stop();
			PORTC &= ~(1<<PC3);
		}
	}
}


// ---------------- ADC / LDR ----------------
void adc_init(){
	ADMUX = (1<<REFS0);
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}

uint16_t adc_read(uint8_t canal){
	ADMUX = (ADMUX & 0xF0) | (canal & 0x0F);
	ADCSRA |= (1<<ADSC);
	while (ADCSRA & (1<<ADSC));
	return ADC;
}


// ---------------- SEQUÊNCIA LDR ----------------
void executaPedidoSequencia(){
	if (seqRequisitada) return;

	seqRequisitada = 1;
	estado = 1;
	contador = 0;

	girar360();
	vidas--;

	timer2_init();
}


// ---------------- UART ----------------
void UART_Init(){
	uint16_t ubrr = 103;
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
}

char UART_Receive(){
	while (!(UCSR0A & (1<<RXC0)));
	return UDR0;
}

void UART_Transmit(char data){
	while (!(UCSR0A&(1<<UDRE0)));
	UDR0 = data;
}

void UART_SendString(const char* s){
	while(*s) UART_Transmit(*s++);
}

void UART_SendInt(uint16_t v){
	char buf[10];
	itoa(v, buf, 10);
	UART_SendString(buf);
}


// ---------------- CONTROLE BLUETOOTH ----------------
void controle(){
	if (UCSR0A & (1<<RXC0))
		comandoAtual = UDR0;

	switch (comandoAtual) {
		case 'F': case 'f': ligaMotoresHorario(); break;
		case 'B': case 'b': ligaMotoresAntiHorario(); break;
		case 'L': case 'l': giraEsquerda(); break;
		case 'R': case 'r': giraDireita(); break;
		case 'T': case 't': pararMotores(); break;
		case 'X': case 'x': pararMotores(); break;

		default: break;
	}
}


// ---------------- BOTÃO RESET ----------------
void botaoReset_init(){
	DDRD &= ~(1<<PD3);
	PORTD |= (1<<PD3);
	EICRA |= (1<<ISC11);
	EIMSK |= (1<<INT1);
}

ISR(INT1_vect){
	if (estado == 2) return;
	vidas = 3;
	atualizaLedsVida();
}


// ---------------- FIM DE JOGO ----------------
void fim(){
	while(1){
		pararMotores();
		estado = 2;
		PORTC &= ~(1<<PC3);
		timer1_stop();
	}
}


// ------------------- MAIN -------------------
int main(void){
	ledsVida();
	motores();
	laser();
	adc_init();
	timer1_init();
	UART_Init();
	botaoReset_init();

	uint16_t valorLDR;
	uint8_t luzAlta = 0;

while (1)
{
    // BLOQUEIA LEITURA DO LDR DURANTE SEQUÊNCIA
    if (estado == 0) {

        valorLDR = adc_read(5);

        if (valorLDR > 900 && seqRequisitada == 0) {
            executaPedidoSequencia();
        }

        // permite controle bluetooth só no modo normal
        controle();
    }

    atualizaLedsVida();

    if (fimDeJogo)
        fim();
}

}
