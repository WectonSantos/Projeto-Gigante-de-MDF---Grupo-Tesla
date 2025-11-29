/*
 * projeto-prog-hardware.c
 *
 * Created: 13/10/2025 22:14:15
 * Author : Wecton
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h> // APENAS PARA RETORNAR OS VALORES INT SEM ERROS NO BLUETOOTH, UTILIZANDO itoa

uint8_t vidas = 3;
uint8_t fimDeJogo = 0;
volatile char comandoAtual = 'T'; 

// SEQUÊNCIA: 0 = NORMAL, 1 = GIRANDO 1S, 2 = BLOQUEADO 5S
volatile uint8_t estado = 0;
volatile uint32_t contador = 0; // // CONTADOR EM MS
volatile uint8_t seqRequisitada = 0;// FLAG DE REQUISIÇÃO DA SEQUÊNCIA

// ROTINA QUE CONFIGURA OS LEDS DE VIDA
void ledsVida(){
	DDRC |= (1 << PC0); // LED 1 (SAÍDA)
	DDRC |= (1 << PC1); // LED 2 (SAÍDA)
	DDRC |= (1 << PC2); // LED 3 (SAÍDA)
	
	PORTC |= (1<< PC0); // LIGANDO LED 1
	PORTC |= (1<< PC1); // LIGANDO LED 1
	PORTC |= (1<< PC2); // LIGANDO LED 1
}
// ROTINA QUE CONFIGURA OS MOTORES
void motores(){ 
	DDRB |= (1 << PB1); // MOTOR 1 SENTIDO HORÁRIO (IN1)
	DDRB |= (1 << PB2); // MOTOR 1 SENTIDO ANTI-HORÁRIO (IN2)
	DDRD |= (1 << PD5); // MOTOR 1 PWM (ENABLE)
	
	DDRB |= (1 << PB3); // MOTOR 2 SENTIDO HORÁRIO (IN3)
	DDRB |= (1 << PB4); // MOTOR 2 SENTIDO ANTI-HORÁRIO (IN4)
	DDRD |= (1 << PD6); // MOTOR 2 PWM	(ENABLE)
	
	//TESTE
    // IN1–IN4 como saída
    DDRB |= (1<<PB1)|(1<<PB2)|(1<<PB3)|(1<<PB4);
    // ENA (PD5) e ENB (PD6) como saída, mas sem setar PORTD ainda
    DDRD |= (1<<PD5)|(1<<PD6);
	//TESTE
}

// BOTÕES PREVIAMENTE QUE SERÃO TROCADOS PELO VALOR RECEBIDO DO CONTROLE
void botoes(){
	DDRB &= ~(1 << PB6);
	DDRB &= ~(1 << PB0);
	DDRD &= ~(1 << PD4);
}

// ROTINA QUE CONFIGURA O LASER
void laser(){
	DDRC |= (1 << PC3); // LASER PORTA C3 (CTC)
}

// ROTINA LDR

void adc_init(void) {
	ADMUX = (1 << REFS0); 		
	//ADMUX &= ~(1 << ADLAR);   // alinhamento à direita (modo normal)

	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // prescaler 128 (16MHz/128=125kHz)
	//DIDR0 = (1 << ADC4D);
	//DDRC &= ~(1 << PC4); 
}

uint16_t adc_read(uint8_t canal) {
	//ADMUX = (1 << REFS0) | (canal & 0x0F);
	ADMUX = (ADMUX & 0xF0) | (canal & 0x0f);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADC; 
}

void atualizaLedsVida() {
	if (vidas >= 3){
		PORTC |= (1 << PC0) | (1 << PC1) | (1 << PC2);
	}else if (vidas == 2) {
		PORTC |= (1 << PC1) | (1 << PC2);
		PORTC &= ~(1 << PC0);
	}else if (vidas == 1) {
		PORTC |= (1 << PC2);
		PORTC &= ~((1 << PC0) | (1 << PC1));
	}else{
		PORTC &= ~((1 << PC0) | (1 << PC1) | (1 << PC2));
		fimDeJogo = 1;
	}
}
// FIM ROTINA LDR

// ROTINA DOS MOTORES
void ligaMotoresHorario(){		
	// DEFINE DIREÇÃO: IN1=1, IN2=0 ; IN3=1, IN4=0
	PORTB |=  (1<<PB1)|(1<<PB3);
	PORTB &= ~((1<<PB2)|(1<<PB4));

	// LIGA PWM
	OCR0B = 200;  
	OCR0A = 200;   
}

void ligaMotoresAntiHorario(){
	// DEFINE DIREÇÃO: IN1=0, IN2=1 ; IN3=0, IN4=1
	PORTB |=  (1<<PB2)|(1<<PB4);
	PORTB &= ~((1<<PB1)|(1<<PB3));

	// LIGA PWM
	OCR0B = 200; 
	OCR0A = 200; 	
}

void ligaMotoresMeiaForca(){
	// DEFINE DIREÇÃO
	PORTB |=  (1<<PB1)|(1<<PB3);
	PORTB &= ~((1<<PB2)|(1<<PB4));

	// APLICANDO 50% DE FATOR DE CICLO
	OCR0B = 128;   // motor 1
	OCR0A = 128;   // motor 2
}

void giraEsquerda() {
	// MOTOR 1: ANTI-HORÁRIO (PB4)
	// MOTOR 2: HORÁRIO (PB5)
	PORTB |= (1 << PB1) | (1 << PB3);
	PORTB &= ~((1 << PB2) | (1 << PB4));

	// LIGA PWM
	OCR0B = 200;   // motor 1
	OCR0A = 200;   // motor 2
}

void giraDireita() {
	// MOTOR 1: HORÁRIO (PB7)
	// MOTOR 2: ANTI-HORÁRIO (PB0)
	PORTB |= (1 << PB1) | (1 << PB6);
	PORTB &= ~((1 << PB3) | (1 << PB4));

	// LIGA PWM
	OCR0B = 200;   // motor 1
	OCR0A = 200;   // motor 2
}

void girar360() {
	// Motor 1 horário: IN1=1, IN2=0
	PORTB |=  (1<<PB1);
	PORTB &= ~(1<<PB2);

	// Motor 2 anti-horário: IN3=0, IN4=1
	PORTB &= ~(1<<PB3);
	PORTB |=  (1<<PB4);

	// PWM
	OCR0A = 200;  // motor 2
	OCR0B = 200;  // motor 1
}


void curvaDireita(){
	PORTB |= (1<<PB1)|(1<<PB3);
	PORTB &= ~((1<<PB2)|(1<<PB4));	
	// LIGA PWM
	OCR0A = 255;
	OCR0B = 77;
}
void curvaEsquerda(){
	PORTB |= (1<<PB1)|(1<<PB3);
	PORTB &= ~((1<<PB2)|(1<<PB4));
	// LIGA PWM
	OCR0A = 77;
	OCR0B = 255;
}

void pararMotores() {
	OCR0A = 0;
	OCR0B = 0;
	PORTB &= ~((1 << PB1) | (1 << PB6) | (1 << PB3) | (1 << PB4));
}

void verificaSentido(){
	if (PIND & (1 << PD4)) {
		ligaMotoresHorario();
	}
	else if (PINB & (1 << PB0)) {
		ligaMotoresAntiHorario();
	}//else if (PINB & (1 << PB6)){
		//ligaMotoresMeiaForca();  
	//}
	else {
		OCR0A = 0;
		OCR0B = 0;
		PORTB &= ~((1 << PB7) | (1 << PB5) | (1 << PB4) | (1 << PB0));
	}	
}
//FIM ROTINA DOS MOTORES

//ROTINA LASER MODO CTC
void timer1_init() {
	TCCR1B |= (1 << WGM12);  // CTC
	OCR1A = 15624;           // 1 SEGUNDO P/ PRESCALER 1024
	TIMSK1 |= (1 << OCIE1A); 
	TCCR1B |= (1 << CS12) | (1 << CS10); // PRESCALER 1024
	sei();
}

// DESLIGA O TIMER1 (PARA NÃO ALTERAR O LASER DURANTE BLOQUEIO)
void timer1_stop() {
	TIMSK1 &= ~(1 << OCIE1A);
	TCCR1B = 0;
}
// PISCA LASER SE ESTIVER EM MODO NORMAL == SEM BLOQUEIO
ISR(TIMER1_COMPA_vect) {
	if (estado == 0){
		PORTC ^= (1 << PC3);
	}
	//PORTC ^= (1 << PC3);  // ALTERNA ESTADO DO LED
}

// TICKS DE 1 MS PARA GERENCIAR A SEQUÊNCIA SEM DELAY
void timer2_init(void){
	TCCR2A = (1 << WGM21); // CTC
	OCR2A = 249; // COM PRESCALER 64 => 1MS
	TIMSK2 |= (1 << OCIE2A);
	TCCR2B = (1 << CS22); // PRESCALER 64
	sei();
}
// DESLIGA O TIMER2
void timer2_stop(void){
	TIMSK2 &= ~(1 << OCIE2A);
	TCCR2B = 0;
}

// CONTROLE DA SEQUÊNCIA (1S GIRANDO, 5S BLOQUEADO)
ISR(TIMER2_COMPA_vect){

	if (estado == 0 && seqRequisitada == 0) return; // SE NÃO HOUVER SEQUENCIA NÃO FAZ NADA

	contador++; 

	if (estado == 1) {
		// LDR RECEBEU VALOR ALTO E ESPERA 1S PARA COMPLETAR O GIRO 360
		if (contador >= 1000) {
			// APÓS O GIRO PARA MOTORES, DESLIGA LASER, INICIA BLOQUEIO
			pararMotores(); 
			PORTC &= ~(1 << PC3);// DESLIGA LASER
			timer1_stop(); // GARANTE QUE O TIMER1 NÃO VAI MAIS PISCAR
			estado = 2; // ALTERNA PARA O ESTADO DE BLOQUEIO
			contador = 0; 
		}
	} else if (estado == 2) { // ESTÁ BLOQUEADO, AGUARDA 5S
		if (contador >= 5000) {
			// AO SAIR DO BLOQUEIO RETORNA AO MODO NORMAL
			estado = 0;
			contador = 0;
			seqRequisitada = 0;
			timer1_init(); // REATIVA O PISCA DO LASER
			PORTC &= ~(1 << PC3);
			timer2_stop();
		}
	}
}
void executaPedidoSequencia(void){
	if (seqRequisitada) return; // JÁ TEM UMA SEQUÊNCIA ATIVA
	seqRequisitada = 1;
	estado = 1; // ENTRA NO GIRO DE 360 POR 1S
	contador = 0;
	comandoAtual = 'a';
	girar360();
	timer2_init(); // INICIA CONTAGEM  
}
//ROTINA CONTROLE BLUETOOTH
// INICIALIZA UART EM 9600 BPS
void UART_Init(void) {
	uint16_t ubrr = 103; // 9600 BPS p/ 16 MHz
	UBRR0H = (unsigned char)(ubrr >> 8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);       // HABILITA RX E TX
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);     // 8 BITS DE DADOS
}
// RECEBE UM CARACTERA
char UART_Receive(void) {
	while (!(UCSR0A & (1 << RXC0))); // ESPERA DADO CHEGAR
	return UDR0;
}
// ENVIA 1 CARACTERE PELA UART
void UART_Transmit(char data) {
	while (!(UCSR0A & (1 << UDRE0))); // ESPERA BUFFER ESVAZIAR
	UDR0 = data;
}

// ENVIA STRING PELA UART
void UART_SendString(const char *str) {
	while (*str) {
		UART_Transmit(*str++);
	}
}

// ENVIA INT COMO TEXTO PARA UART

void UART_SendInt(uint16_t value) {
	char buffer[10];
	itoa(value, buffer, 10);     // VALOR ? BUFFER ? BASE 10
	UART_SendString(buffer);
}

void controle() {

	if (UCSR0A & (1<<RXC0)) {   // SÓ LÊ SE TIVER ALGO EM UART
		comandoAtual = UDR0;    // SALVA NOVO COMANDO
	}

	switch (comandoAtual) {

		case 'F':
		case 'f':
		ligaMotoresHorario();
		break;

		case 'B':
		case 'b':
		ligaMotoresAntiHorario();
		break;

		case 'L':
		case 'l':
		giraEsquerda();
		break;

		case 'R':
		case 'r':
		giraDireita();
		break;

		case 'T':   
		case 't':
		pararMotores();
		break;
		
		case 'X':
		case 'x':
			if (estado == 2) return;
			vidas = 3;
			atualizaLedsVida();
		default:
		// comando inválido -> não muda nada
		break;
	}
}
//FIM ROTINA CONTROLE BLUETOOTH

// RESET
void botaoReset_init() {
	DDRD &= ~(1 << PD3);   // ENTRADA
	PORTD |= (1 << PD3);   // PULL-UP

	EICRA |= (1 << ISC11); // BORDA DE DESCIDA
	EIMSK |= (1 << INT1);  // HABILITE INT1
}

ISR(INT1_vect) {
	// IMPEDE RESET DURANTE O BLOQUEIO
	if (estado == 2) return;
	vidas = 3;
	atualizaLedsVida();
}


void fim(){
	while(1){
		pararMotores();
		estado = 2;
		PORTC &= ~(1 << PC3);// DESLIGA LASER
		timer1_stop(); // GARANTE QUE O TIMER1 NÃO VAI MAIS PISCAR
	}
}

int main(void)
{
	// SETANDO TODAS AS PORTAS
	ledsVida(); 
	motores(); 
	//botoes(); 
	laser(); 
	
	// ROTINA LDR
	adc_init();
	uint16_t valorLDR;
	uint8_t luzAlta = 0; 
	// FIM ROTINA LDR

    // FAST PWM NO TIMER 0
    TCCR0A = (1<<COM0A1)|(1<<COM0B1)|(1<<WGM01)|(1<<WGM00);
    TCCR0B = (1<<CS01);    // PRESCALER = 8

    //GARANTE QUE VAI COMEÇAR COM MOTORES DESLIGADOS
    OCR0A = 0; 
    OCR0B = 0;  
	
	//LASER
	timer1_init();

	// CONTROLE 
	UART_Init();
	
    // GARANTE SAÍDAS LIMPAS
	PORTB &= ~((1 << PB1) | (1 << PB2) | (1 << PB3) | (1 << PB4));	
	
	// RESET
	
	botaoReset_init();

    while (1) 
    {
		//ROTINA LDR
		valorLDR = adc_read(5); // LÊ O ADC5 (PC5)
		// debug ldr
		UART_SendString("LDR = ");
		UART_SendInt(valorLDR);
		UART_SendString("\r\n");	
		// debug ldr

		if (valorLDR > 900) {
			luzAlta = 1; // LDR RECEBEU VALOR ALTO
			executaPedidoSequencia();
		}else{
			luzAlta = 0;
		}
		if (luzAlta) {
			// só perde vida se NÃO estiver executando a sequência
			if (vidas > 0 && seqRequisitada == 0 && estado == 0) {
				vidas--;
				executaPedidoSequencia();  // inicia o giro + bloqueio
			}

			} else {
			luzAlta = 0;
		}
		atualizaLedsVida();
		
		// SOMENTE ACEITA CONTROLES QUANDO NÃO ESTIVER EM BLOQUEIO
        if (estado == 0){
			verificaSentido();
			if (PINB & (1 << PB6)){
		        executaPedidoSequencia();
				vidas--;
	        }			
			controle();  // CHAMA CONTROLE    
		}
		if(fimDeJogo == 1){
			fim();
		}
		
									
    }
}