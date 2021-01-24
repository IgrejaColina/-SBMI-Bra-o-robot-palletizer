#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "lcd.h"
#include "lcd_drv.h"
#include "serial_printf.h"
#include <avr/eeprom.h>

////////////////////////////////////////////////////////////////////////////////  TIMER 2  /////////////////////////////////////////////////////////////////////
volatile uint8_t tim2_servo_cnt;
volatile uint8_t tim2_servo_check = 0;
volatile uint16_t aux = 0, aux2 = 0, r = 0, r_max = 0, g = 0, g_max = 0, b = 0, b_max = 0, cor;
void ler_vermelho()
{
	PORTD &= ~(1 << PD3); //vermelho
	PORTD &= ~(1 << PD5);
	aux2 = 1;
	aux = 0;

	while (aux != 10)
		;

	if (aux == 10)
	{
		aux2 = 2;
	}
}
void ler_verde()
{
	PORTD |= (1 << PD3); //verde
	PORTD |= (1 << PD5);
	aux = 0;

	while (aux != 10)
		;

	if (aux == 10)
	{
		aux2 = 3;
	}
}
void ler_azul()
{
	PORTD &= ~(1 << PD3); //azul
	PORTD |= (1 << PD5);
	aux = 0;
	while (aux != 10)
		;

	if (aux == 10)
	{
		aux2 = 0;
	}
}
void sensor_cor()
{

	ler_vermelho();
	while (aux2 != 2)
		;
	ler_verde();
	while (aux2 != 3)
		;
	ler_azul();
	while (aux2 != 0)
		;
}
int ler_cor()
{
	aux2 = 1;
	sensor_cor();

	while (r_max == 0 || g_max == 0 || b_max == 0)
		;

	if (r_max > g_max && r_max > b_max)
	{
		r_max = 0;
		g_max = 0;
		b_max = 0;
		return 1;
	}
	else if (g_max > r_max && g_max > b_max)
	{
		r_max = 0;
		g_max = 0;
		b_max = 0;
		return 2;
	}
	else if (b_max > r_max && b_max > g_max)
	{
		r_max = 0;
		g_max = 0;
		b_max = 0;
		return 3;
	}

	return 0;
}

void imprime_cor(int cor)
{
	if (cor == 1)
	{
		lcd_xy(0, 1);
		lcd_puts("Vermelho");
	}
	else if (cor == 2)
	{
		lcd_xy(0, 1);
		lcd_puts("Verde");
	}
	else if (cor == 3)
	{
		lcd_xy(0, 1);
		lcd_puts("Azul");
	}
	else if (cor == 0)
	{
		lcd_xy(0, 1);
		lcd_puts("Erro");
	}
	cor = 0;
}

ISR(TIMER2_OVF_vect)
{
	TCNT2 = 131; //conta 8ms
	if (aux2 == 1)
	{
		r = TCNT0;
		if (r > r_max)
			r_max = r;
	}
	else if (aux2 == 2)
	{
		g = TCNT0;
		if (g > g_max)
			g_max = g;
	}
	else if (aux2 == 3)
	{
		b = TCNT0;
		if (b > b_max)
			b_max = b;
	}
	if (aux < 20)
	{
		aux++;
	}
	TCNT0 = 0;
	if (tim2_servo_cnt < 131) //1.5 segundos
	{
		tim2_servo_cnt++;
	}

	if (tim2_servo_cnt == 131)
	{
		tim2_servo_check = 1;
	}
}

void tim2_servo_inif(void)
{
	tim2_servo_cnt = 0;
	tim2_servo_check = 0;
}

uint8_t tim2_servo_checkf(void)
{
	if (tim2_servo_check == 0)
	{
		return 0;
	}
	else
	{
		return 1;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////  SERVO  //////////////////////////////////////////////////
void servo_deg(int16_t angle, uint8_t n)
{
	if (n == 1)
	{
		PORTB &= ~(1 << PB5); //desliga linha dos servo3 e 4
		PORTB |= (1 << PB4);  //liga linha dos servo1 e 2
		OCR1A = angle;
	}
	else if (n == 2)
	{
		PORTB &= ~(1 << PB5);
		PORTB |= (1 << PB4);
		OCR1B = angle;
	}
	else if (n == 3)
	{
		PORTB &= ~(1 << PB4);
		PORTB |= (1 << PB5);
		OCR1A = angle;
	}
	else if (n == 4)
	{
		PORTB &= ~(1 << PB4);
		PORTB |= (1 << PB5);
		OCR1B = angle;
	}
}

uint8_t t_check_roda = 0;
uint8_t t_check_sobe = 0;
uint8_t t_check_estica = 0;
uint8_t t_check_trinca = 0;
uint8_t mem_roda = 0;
uint8_t mem_sobe = 0;
uint8_t mem_estica = 0;
uint8_t mem_trinca = 0;

uint8_t garra_roda(int8_t pos) //////////////rotação de baixo
{
	int angle = 0;
	angle = (200 / 9) * pos + 2999.5;
	if (((!t_check_roda) && (!t_check_sobe) && (!t_check_estica) && (!t_check_trinca)))
	{
		tim2_servo_inif();
		t_check_roda = 1;
	}

	if (tim2_servo_checkf() && t_check_roda == 1)
	{
		TCCR1A &= ~(1 << COM1B1); //ao mudar a linha dos transistores, se não fizesse isto, o outro motor mexia se
		TCCR1A |= (1 << COM1A1);

		servo_deg(angle, 1);
		t_check_roda = 0;
		return 1;
	}

	return 0;
}

uint8_t garra_sobe(int8_t pos)
{
	int angle = 0;

	angle = (200 / 9) * pos + 2999.5;
	if (((!t_check_sobe) && (!t_check_roda) && (!t_check_estica) && (!t_check_trinca)))
	{
		tim2_servo_inif();
		t_check_sobe = 1;
	}

	if (tim2_servo_checkf() && t_check_sobe == 1)
	{
		TCCR1A &= ~(1 << COM1A1);
		TCCR1A |= (1 << COM1B1);

		servo_deg(angle, 2);
		t_check_sobe = 0;
		return 1;
	}

	return 0;
}

uint8_t garra_estica(int8_t pos)
{
	int angle = 0;

	angle = (200 / 9) * pos + 2999.5;
	if (((!t_check_roda) && (!t_check_sobe) && (!t_check_estica) && (!t_check_trinca)))
	{
		tim2_servo_inif();
		t_check_estica = 1;
	}

	if (tim2_servo_checkf() && t_check_estica == 1)
	{
		TCCR1A &= ~(1 << COM1B1);
		TCCR1A |= (1 << COM1A1);
		servo_deg(angle, 3);
		t_check_estica = 0;
		return 1;
	}
	return 0;
}

uint8_t garra_trinca(int8_t pos)
{
	int angle = 0;

	angle = (200 / 9) * pos + 2999.5;
	if (((!t_check_roda) && (!t_check_sobe) && (!t_check_estica) && (!t_check_trinca)))
	{
		tim2_servo_inif();
		t_check_trinca = 1;
	}

	if (tim2_servo_checkf())
	{
		TCCR1A &= ~(1 << COM1A1);
		TCCR1A |= (1 << COM1B1);
		servo_deg(angle, 4);
		t_check_trinca = 0;
		return 1;
	}
	return 0;
}

uint8_t fsm0 = 0, next_fsm0 = 0, fsm1 = 0, next_fsm1 = 0, fsm2 = 0, next_fsm2 = 0, calibrado = 0;
uint8_t ver_mov = 0, pos1 = 0, pos2 = 0, pos3 = 0, pos4 = 0;
void regressa_coordenadas(uint8_t roda, uint8_t sobe, uint8_t estica, uint8_t trinca)
{
	if (!pos1)
	{
		garra_estica(estica);
		if (t_check_estica == 0)
		{
			pos1 = 1;
		}
	}
	else if (!pos2 && pos1)
	{
		garra_sobe(sobe);
		if (t_check_sobe == 0)
		{
			pos2 = 1;
		}
	}
	else if (!pos3 && pos2)
	{
		garra_roda(roda);
		if (t_check_roda == 0)
		{
			pos3 = 1;
		}
	}
	else if (!pos4 && pos3)
	{
		if (trinca != 91)
			garra_trinca(trinca);
		if (t_check_trinca == 0 || trinca == 91)
		{
			pos4 = 1;
		}
	}
}

void vai_coordenadas(uint8_t roda, uint8_t sobe, uint8_t estica, uint8_t trinca)
{
	if (!pos1)
	{
		garra_roda(roda);
		if (t_check_roda == 0)
		{
			pos1 = 1;
		}
	}
	else if (!pos2 && pos1)
	{
		garra_sobe(sobe);
		if (t_check_sobe == 0)
		{
			pos2 = 1;
		}
	}
	else if (!pos3 && pos2)
	{
		garra_estica(estica);
		if (t_check_estica == 0)
		{
			pos3 = 1;
		}
	}
	else if (!pos4 && pos3)
	{
		if (trinca != 91)
			garra_trinca(trinca);
		if (t_check_trinca == 0 || trinca == 91)
		{
			pos4 = 1;
		}
	}
}
/////////////////////////////////////////////////////////////////////////// BOTÕES //////////////////////////////////////////////////////

uint8_t read_button_S1(void)
{
	uint8_t v;
	v = PIND & (1 << 7);
	return !v;
}

uint8_t read_button_S2(void)
{
	uint8_t v;
	v = PIND & (1 << 6);
	return !v;
}

uint8_t read_button_S3(void)
{
	uint8_t v;
	v = PINB & (1 << 0);
	return !v;
}

/////////////////////////////////////////////////////////////////////////  MAIN  ///////////////////////////////////////////////////////

void io_init(void)
{
	DDRB |= (1 << PB1) | (1 << PB2); // ondas para o servo
	DDRB |= (1 << PB4) | (1 << PB5); // para os transistores
	DDRD &= ~(1 << PD6);			 //botão 2
	DDRD &= ~(1 << PD7);			 //botão 1
	DDRD &= ~(1 << PB0);			 //botão 3

	PORTB &= ~(1 << PB4);
	PORTB &= ~(1 << PB5);
	/* 1. Fast PWM mode 14: set WGM11, WGM12, WGM13 to 1 */
	/* 3. pre-scaler of 8 */
	/* 4. Set Fast PWM inverting mode */
	TCCR1A |= (1 << WGM11) | (1 << COM1A1) | (1 << COM1B1);
	TCCR1B |= (1 << WGM12) | (1 << WGM13) | (1 << CS11);

	/* 2. Set ICR1 register: PWM period */
	//ICR1 = 39999;

	//TIMER 2
	TCCR2A = 0;
	TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20); //Prescaler 1024
	TIMSK2 |= (1 << TOIE2);							   //ovf enabled
	TIFR2 |= (1 << TOV2);
	ICR1 = 39999;

	//Sensor de cor e LCD
	DDRD |= (1 << PD0) | (1 << PD2) | (1 << PD3) | (1 << PD5); // pins do sensor
	DDRD &= ~(1 << PD4);

	PORTD |= (1 << PD0); //scaling da frequencia
	PORTD &= ~(1 << PD2);

	//TIMER 0
	TCCR0B |= (1 << CS02) | (1 << CS01) | (1 << CS00); //Clock no RE do sensor
}

uint8_t fsm_troca_cores = 0, nextfsm_troca_cores = 0, fsm_lcd = 0, nextfsm_lcd = 0;
uint8_t s1_prev, s1, s2_prev, s2, s3, s3_prev;
int8_t sobe_angle = 0, roda_angle = 0, estica_angle = 0, trinca_angle = 0;
int8_t sobe_pos1 = 100, roda_pos1 = 100, estica_pos1 = 100; //posicoes das pecas
int8_t sobe_pos2 = 100, roda_pos2 = 100, estica_pos2 = 100;
int8_t sobe_pos3 = 100, roda_pos3 = 100, estica_pos3 = 100;
int8_t sobe_red = 100, roda_red = 100, estica_red = 100; //posicoes dos "caixotes"
int8_t sobe_green = 100, roda_green = 100, estica_green = 100;
int8_t sobe_blue = 100, roda_blue = 100, estica_blue = 100;
int8_t sobe_sensor = 100, roda_sensor = 100, estica_sensor = 100; //posicao sensor de cor
uint8_t tudo_definido = 0, pos1_def = 0, pos2_def = 0, pos3_def = 0, red_def = 0, green_def = 0, blue_def = 0, sensor_def = 0;
int8_t abrir = 0, fechar = 60;

/*	eeprom addresses

	0 --> sobe_sensor
	1 --> roda_sensor
	2 --> estica_sensor
	3 --> sensor_def
	4 --> sobe_pos1
	5 --> roda_pos1
	6 --> estica_pos1
	7 --> pos1_def
	8 --> sobe_pos2
	9 --> roda_pos2
	10 --> estica_pos2
	11 --> pos2_def
	12 --> sobe_pos3
	13 --> roda_pos3
	14 --> estica_pos3
	15 --> pos3_def
	16 --> sobe_red
	17 --> roda_red
	18 --> estica_red
	19 --> red_def
	20 --> sobe_green
	21 --> roda_green
	22 --> estica_green
	23 --> green_def
	24 --> sobe_blue
	25 --> roda_blue
	26 --> estica_blue
	27 --> blue_def
	28 --> tudo_definido
*/

int main(void)
{
	io_init();
	sei();
	printf_init();				 // Init the serial port to have the ability to printf
	printf("Serial I/O Demo\n"); // into a terminal

	while (1)
	{
		s1_prev = s1;
		s1 = read_button_S1(); //ler botão 1

		s2_prev = s2;
		s2 = read_button_S2(); //Ler botão 2

		s3_prev = s3;
		s3 = read_button_S3(); //Ler botão 3

		printf("0: %u 1: %u 2: %u lcd: %u troca cores %u vermelho R %d S %d E%d Verde R %d S %d E %d Azul R %d S %d E%d \n", fsm0, fsm1, fsm2, fsm_lcd, fsm_troca_cores, eeprom_read_byte((uint8_t *)17), eeprom_read_byte((uint8_t *)16), eeprom_read_byte((uint8_t *)18), eeprom_read_byte((uint8_t *)21), eeprom_read_byte((uint8_t *)20), eeprom_read_byte((uint8_t *)22), eeprom_read_byte((uint8_t *)25), eeprom_read_byte((uint8_t *)24), eeprom_read_byte((uint8_t *)26));

		switch (fsm0)
		{
		case 0:
			if (calibrado && (!s1_prev && s1))
			{
				next_fsm0 = 1;
				eeprom_update_byte((uint8_t *)28, 0);
			}
			if (calibrado && (!s2_prev && s2))
			{
				next_fsm0 = 2;
			}
			break;
		case 1:
			if (eeprom_read_byte((uint8_t *)28) == 1 && calibrado)
				next_fsm0 = 2;
			break;
		case 2:
			if (!s1_prev && s1 && fsm_troca_cores==0 && fsm_lcd == 2)
			{
				next_fsm0 = 3;
			}
			break;
		case 3:
			vai_coordenadas(eeprom_read_byte((uint8_t *)5), eeprom_read_byte((uint8_t *)4), eeprom_read_byte((uint8_t *)6), fechar); //vai pos 1
			if (pos1 && pos2 && pos3 && pos4)
			{
				next_fsm0 = 4;
				pos1 = 0;
				pos2 = 0;
				pos3 = 0;
				pos4 = 0;
				calibrado = 0;
			}
			break;
		case 4:
			if (calibrado) //vai para 0
			{
				vai_coordenadas(eeprom_read_byte((uint8_t *)1), eeprom_read_byte((uint8_t *)0), eeprom_read_byte((uint8_t *)2), fechar); //vai para o sensor
				if (pos1 && pos2 && pos3 && pos4)
				{
					next_fsm0 = 5;
					pos1 = 0;
					pos2 = 0;
					pos3 = 0;
					pos4 = 0;
					calibrado = 0;
				}
			}
			break;
		case 5:
			cor = 0;
			cor = ler_cor();
			if (cor == 1)
			{
				next_fsm0 = 6;
			}
			else if (cor == 2)
			{
				next_fsm0 = 7;
			}
			else if (cor == 3)
			{
				next_fsm0 = 8;
			}
			break;
		case 6:
			if (calibrado) //vai para 0
			{
				vai_coordenadas(eeprom_read_byte((uint8_t *)17), eeprom_read_byte((uint8_t *)16), eeprom_read_byte((uint8_t *)18), abrir); //vai para vermelho
				if (pos1 && pos2 && pos3 && pos4)
				{
					next_fsm0 = 9;
					pos1 = 0;
					pos2 = 0;
					pos3 = 0;
					pos4 = 0;
					calibrado = 0;
				}
			}
			break;
		case 7:
			if (calibrado) //vai para 0
			{
				vai_coordenadas(eeprom_read_byte((uint8_t *)21), eeprom_read_byte((uint8_t *)20), eeprom_read_byte((uint8_t *)22), abrir); //vai para verde
				if (pos1 && pos2 && pos3 && pos4)
				{
					next_fsm0 = 9;
					pos1 = 0;
					pos2 = 0;
					pos3 = 0;
					pos4 = 0;
					calibrado = 0;
				}
			}
			break;
		case 8:
			if (calibrado) //vai para 0
			{
				vai_coordenadas(eeprom_read_byte((uint8_t *)25), eeprom_read_byte((uint8_t *)24), eeprom_read_byte((uint8_t *)26), abrir); //vai para azul
				if (pos1 && pos2 && pos3 && pos4)
				{
					next_fsm0 = 9;
					pos1 = 0;
					pos2 = 0;
					pos3 = 0;
					pos4 = 0;
					calibrado = 0;
				}
			}
			break;
		case 9:
			if (calibrado) //vai para 0
			{
				vai_coordenadas(eeprom_read_byte((uint8_t *)9), eeprom_read_byte((uint8_t *)8), eeprom_read_byte((uint8_t *)10), fechar); //vai pos 2
				if (pos1 && pos2 && pos3 && pos4)
				{
					next_fsm0 = 10;
					pos1 = 0;
					pos2 = 0;
					pos3 = 0;
					pos4 = 0;
					calibrado = 0;
				}
			}
			break;
		case 10:
			if (calibrado) //vai para 0
			{
				vai_coordenadas(eeprom_read_byte((uint8_t *)1), eeprom_read_byte((uint8_t *)0), eeprom_read_byte((uint8_t *)2), fechar); //vai para o sensor
				if (pos1 && pos2 && pos3 && pos4)
				{
					next_fsm0 = 11;
					pos1 = 0;
					pos2 = 0;
					pos3 = 0;
					pos4 = 0;
					calibrado = 0;
				}
			}
			break;
		case 11:
			cor = 0;
			cor = ler_cor();
			if (cor == 1)
			{
				next_fsm0 = 12;
			}
			else if (cor == 2)
			{
				next_fsm0 = 13;
			}
			else if (cor == 3)
			{
				next_fsm0 = 14;
			}
			break;
		case 12:
			if (calibrado) //vai para 0
			{
				vai_coordenadas(eeprom_read_byte((uint8_t *)17), eeprom_read_byte((uint8_t *)16), eeprom_read_byte((uint8_t *)18), abrir); //vai para vermelho
				if (pos1 && pos2 && pos3 && pos4)
				{
					next_fsm0 = 15;
					pos1 = 0;
					pos2 = 0;
					pos3 = 0;
					pos4 = 0;
					calibrado = 0;
				}
			}
			break;
		case 13:
			if (calibrado) //vai para 0
			{
				vai_coordenadas(eeprom_read_byte((uint8_t *)21), eeprom_read_byte((uint8_t *)20), eeprom_read_byte((uint8_t *)22), abrir); //vai para verde
				if (pos1 && pos2 && pos3 && pos4)
				{
					next_fsm0 = 15;
					pos1 = 0;
					pos2 = 0;
					pos3 = 0;
					pos4 = 0;
					calibrado = 0;
				}
			}
			break;
		case 14:
			if (calibrado) //vai para 0
			{
				vai_coordenadas(eeprom_read_byte((uint8_t *)25), eeprom_read_byte((uint8_t *)24), eeprom_read_byte((uint8_t *)26), abrir); //vai para azul
				if (pos1 && pos2 && pos3 && pos4)
				{
					next_fsm0 = 15;
					pos1 = 0;
					pos2 = 0;
					pos3 = 0;
					pos4 = 0;
					calibrado = 0;
				}
			}
			break;
		case 15:
			if (calibrado) //vai para 0
			{
				vai_coordenadas(eeprom_read_byte((uint8_t *)13), eeprom_read_byte((uint8_t *)12), eeprom_read_byte((uint8_t *)11), fechar); //vai pos 3
				if (pos1 && pos2 && pos3 && pos4)
				{
					next_fsm0 = 16;
					pos1 = 0;
					pos2 = 0;
					pos3 = 0;
					pos4 = 0;
					calibrado = 0;
				}
			}
			break;
		case 16:
			if (calibrado) //vai para 0
			{
				vai_coordenadas(eeprom_read_byte((uint8_t *)1), eeprom_read_byte((uint8_t *)0), eeprom_read_byte((uint8_t *)2), fechar); //vai para o sensor
				if (pos1 && pos2 && pos3 && pos4)
				{
					next_fsm0 = 17;
					pos1 = 0;
					pos2 = 0;
					pos3 = 0;
					pos4 = 0;
					calibrado = 0;
				}
			}
			break;
		case 17:
			cor = 0;
			cor = ler_cor();
			if (cor == 1)
			{
				next_fsm0 = 18;
			}
			else if (cor == 2)
			{
				next_fsm0 = 19;
			}
			else if (cor == 3)
			{
				next_fsm0 = 20;
			}
			break;
		case 18:
			if (calibrado) //vai para 0
			{
				vai_coordenadas(eeprom_read_byte((uint8_t *)17), eeprom_read_byte((uint8_t *)16), eeprom_read_byte((uint8_t *)18), 0); //vai para vermelho
				if (pos1 && pos2 && pos3 && pos4)
				{
					next_fsm0 = 2;
					pos1 = 0;
					pos2 = 0;
					pos3 = 0;
					pos4 = 0;
					calibrado = 0;
				}
			}
			break;
		case 19:
			if (calibrado) //vai para 0
			{
				vai_coordenadas(eeprom_read_byte((uint8_t *)21), eeprom_read_byte((uint8_t *)20), eeprom_read_byte((uint8_t *)22), 0); //vai para verde
				if (pos1 && pos2 && pos3 && pos4)
				{
					next_fsm0 = 2;
					pos1 = 0;
					pos2 = 0;
					pos3 = 0;
					pos4 = 0;
					calibrado = 0;
				}
			}
			break;
		case 20:
			if (calibrado) //vai para 0
			{
				vai_coordenadas(eeprom_read_byte((uint8_t *)25), eeprom_read_byte((uint8_t *)24), eeprom_read_byte((uint8_t *)26), 0); //vai para azul
				if (pos1 && pos2 && pos3 && pos4)
				{
					next_fsm0 = 2;
					pos1 = 0;
					pos2 = 0;
					pos3 = 0;
					pos4 = 0;
					calibrado = 0;
				}
			}
			break;
		}
		switch (fsm1)
		{
		case 0:
			if (fsm2 == 0 && calibrado == 1 && eeprom_read_byte((uint8_t *)28) != 1 && fsm0 == 1)
				next_fsm1 = 1;
			break;
		case 1:
			garra_roda(roda_angle);
			if (!s2_prev && s2)
			{
				if (roda_angle <= 90)
				{
					roda_angle += 10;
				}
				if (roda_angle > 90)
				{
					roda_angle = -90;
				}
			}
			if (!s1_prev && s1)
			{
				next_fsm1 = 2;
				if (roda_sensor == 100)
				{
					roda_sensor = roda_angle;
					eeprom_update_byte((uint8_t *)1, roda_sensor);
				}
				else if (roda_pos1 == 100)
				{
					roda_pos1 = roda_angle;
					eeprom_update_byte((uint8_t *)5, roda_angle);
				}
				else if (roda_pos2 == 100)
				{
					roda_pos2 = roda_angle;
					eeprom_update_byte((uint8_t *)9, roda_angle);
				}
				else if (roda_pos3 == 100)
				{
					roda_pos3 = roda_angle;
					eeprom_update_byte((uint8_t *)13, roda_angle);
				}
				else if (roda_red == 100)
				{
					roda_red = roda_angle;
					eeprom_update_byte((uint8_t *)17, roda_red);
				}
				else if (roda_green == 100)
				{
					roda_green = roda_angle;
					eeprom_update_byte((uint8_t *)21, roda_green);
				}
				else if (roda_blue == 100)
				{
					roda_blue = roda_angle;
					eeprom_update_byte((uint8_t *)25, roda_blue);
				}
				roda_angle = 0;

				t_check_roda = 0;
			}
			break;

		case 2:
			garra_sobe(sobe_angle);

			if (!s2_prev && s2)
			{
				if (sobe_angle <= 90)
				{
					sobe_angle += 10;
				}
				if (sobe_angle > 90)
				{
					sobe_angle = -25;
				}
			}
			if (!s1_prev && s1)
			{
				next_fsm1 = 3;
				if (sobe_sensor == 100)
				{
					sobe_sensor = sobe_angle;
					eeprom_update_byte((uint8_t *)0, sobe_angle);
				}
				else if (sobe_pos1 == 100)
				{
					sobe_pos1 = sobe_angle;
					eeprom_update_byte((uint8_t *)4, sobe_angle);
				}
				else if (sobe_pos2 == 100)
				{
					sobe_pos2 = sobe_angle;
					eeprom_update_byte((uint8_t *)8, sobe_angle);
				}
				else if (sobe_pos3 == 100)
				{
					sobe_pos3 = sobe_angle;
					eeprom_update_byte((uint8_t *)12, sobe_angle);
				}
				else if (sobe_red == 100)
				{
					sobe_red = sobe_angle;
					eeprom_update_byte((uint8_t *)16, sobe_red);
				}
				else if (sobe_green == 100)
				{
					sobe_green = sobe_angle;
					eeprom_update_byte((uint8_t *)20, sobe_green);
				}
				else if (sobe_blue == 100)
				{
					sobe_blue = sobe_angle;
					eeprom_update_byte((uint8_t *)24, sobe_blue);
				}
				sobe_angle = 0;
				t_check_sobe = 0;
			}

			break;

		case 3:
			garra_estica(estica_angle);
			if (!s2_prev && s2)
			{
				if (estica_angle <= 70)
				{
					estica_angle += 10;
				}
				if (estica_angle > 70)
				{
					estica_angle = -35;
				}
			}
			if (!s1_prev && s1)
			{
				next_fsm1 = 0;
				if (estica_sensor == 100)
				{
					estica_sensor = estica_angle;
					eeprom_update_byte((uint8_t *)2, estica_sensor);
					sensor_def = 1;
					eeprom_update_byte((uint8_t *)3, sensor_def);
				}
				else if (estica_pos1 == 100)
				{
					estica_pos1 = sobe_angle;
					eeprom_update_byte((uint8_t *)6, estica_angle);
					pos1_def = 1;
					eeprom_update_byte((uint8_t *)7, pos1_def);
				}
				else if (estica_pos2 == 100)
				{
					estica_pos2 = estica_angle;
					eeprom_update_byte((uint8_t *)10, estica_angle);
					pos2_def = 1;
					eeprom_update_byte((uint8_t *)11, pos2_def);
				}
				else if (estica_pos3 == 100)
				{
					estica_pos3 = estica_angle;
					eeprom_update_byte((uint8_t *)14, estica_angle);
					pos3_def = 1;
					eeprom_update_byte((uint8_t *)15, pos3_def);
				}
				else if (estica_red == 100)
				{
					estica_red = estica_angle;
					eeprom_update_byte((uint8_t *)18, estica_red);
					red_def = 1;
					eeprom_update_byte((uint8_t *)19, red_def);
				}
				else if (estica_green == 100)
				{
					estica_green = estica_angle;
					eeprom_update_byte((uint8_t *)22, estica_green);
					green_def = 1;
					eeprom_update_byte((uint8_t *)23, green_def);
				}
				else if (estica_blue == 100)
				{
					estica_blue = estica_angle;
					eeprom_update_byte((uint8_t *)26, estica_blue);
					blue_def = 1;
					eeprom_update_byte((uint8_t *)27, blue_def);
					tudo_definido = 1;
					eeprom_update_byte((uint8_t *)28, 1);
				}
				estica_angle = 0;
				calibrado = 0;
				t_check_estica = 0;
			}
			break;
		}
		switch (fsm2)
		{
		case 0:
			if (fsm1 == 0 && calibrado == 0)
				next_fsm2 = 1;
			break;
		case 1:
			if (fsm0 == 0)
				regressa_coordenadas(0, 0, 0, abrir);
			else
				regressa_coordenadas(0, 0, 0, 91);
			lcd_init();
			lcd_xy(0, 0);
			lcd_puts("Posicao standard");
			if(fsm0==5 || fsm0==6 || fsm0==7 || fsm0==8 || fsm0==11 || fsm0==12 || fsm0==13 || fsm0==14 || fsm0==17 || fsm0==18 || fsm0==19 || fsm0==20){
					imprime_cor(cor);
				}
			if (pos1 && pos2 && pos3 && pos4)
			{
				next_fsm2 = 0;
				pos1 = 0;
				pos2 = 0;
				pos3 = 0;
				pos4 = 0;
				calibrado = 1;
				lcd_init();
			}
			break;
		}

		switch (fsm_troca_cores)
		{
		case 0:
			if (fsm0 ==2 && (fsm_lcd == 7))
			{
				nextfsm_troca_cores = 1;
			}

			break;
		case 1:
			lcd_init();
			lcd_puts("B1-red B2-green");
			lcd_xy(0, 1);
			lcd_puts("B3 -> blue");

			if (!s1_prev && s1)
			{
				nextfsm_troca_cores = 2;
			}

			if (!s2_prev && s2)
			{
				nextfsm_troca_cores = 3;
			}

			if (!s3_prev && s3)
			{
				nextfsm_troca_cores = 4;
			}
			break;
		case 2:
			lcd_init();
			lcd_xy(0, 0);
			lcd_puts("B1-> red->green");
			lcd_xy(0, 1);
			lcd_puts("B2-> red->blue");

			if (!s1_prev && s1)
			{
				int trocadex;

				trocadex = eeprom_read_byte((uint8_t*)21);//roda_green;
				roda_green = eeprom_read_byte((uint8_t*)17);//roda_red;
				roda_red = trocadex;
				eeprom_update_byte((uint8_t *)17, roda_red);
				eeprom_update_byte((uint8_t *)21, roda_green);

				trocadex = eeprom_read_byte((uint8_t*)20);//sobe_green;
				sobe_green = eeprom_read_byte((uint8_t*)16);//sobe_red;
				sobe_red = trocadex;
				eeprom_update_byte((uint8_t *)16, sobe_red);
				eeprom_update_byte((uint8_t *)20, sobe_green);

				trocadex = eeprom_read_byte((uint8_t *)22);//estica_green;
				estica_green = eeprom_read_byte((uint8_t*)18);//estica_red;
				estica_red = trocadex;
				eeprom_update_byte((uint8_t *)18, estica_red);
				eeprom_update_byte((uint8_t *)22, estica_green);

				nextfsm_troca_cores = 0;
				nextfsm_lcd = 2;
				
			}/*
vai_coordenadas(eeprom_read_byte((uint8_t *)17), eeprom_read_byte((uint8_t *)16), eeprom_read_byte((uint8_t *)18), abrir); //vai para vermelho

				vai_coordenadas(eeprom_read_byte((uint8_t *)21), eeprom_read_byte((uint8_t *)20), eeprom_read_byte((uint8_t *)22), abrir); //vai para verde

				vai_coordenadas(eeprom_read_byte((uint8_t *)25), eeprom_read_byte((uint8_t *)24), eeprom_read_byte((uint8_t *)26), abrir); //vai para azul*/

			if (!s2_prev && s2)
			{
				nextfsm_troca_cores = 3;
				int trocadex;

				trocadex = eeprom_read_byte((uint8_t*)25);//roda_blue;
				roda_blue = eeprom_read_byte((uint8_t*)17);//roda_red;
				roda_red = trocadex;
				eeprom_update_byte((uint8_t *)17, roda_red);
				eeprom_update_byte((uint8_t *)25, roda_blue);

				trocadex = eeprom_read_byte((uint8_t*)24);//sobe_blue;
				sobe_blue = eeprom_read_byte((uint8_t*)16);//sobe_red;
				sobe_red = trocadex;
				eeprom_update_byte((uint8_t *)16, sobe_red);
				eeprom_update_byte((uint8_t *)24, sobe_blue);

				trocadex = eeprom_read_byte((uint8_t*)26);//estica_blue;
				estica_blue = eeprom_read_byte((uint8_t*)18);//estica_red;
				estica_red = trocadex;
				eeprom_update_byte((uint8_t *)18, estica_red);
				eeprom_update_byte((uint8_t *)26, estica_blue);

				nextfsm_troca_cores = 0;
				nextfsm_lcd = 2;
			}
			break;
		case 3:
			lcd_init();
			lcd_xy(0, 0);
			lcd_puts("B1-> green->red");
			lcd_xy(0, 1);
			lcd_puts("B2->green->blue");

			if (!s1_prev && s1)
			{
				int trocadex;

				trocadex = eeprom_read_byte((uint8_t*)21);//roda_green;
				roda_green = eeprom_read_byte((uint8_t*)17); //roda_red;
				roda_red = trocadex;
				eeprom_update_byte((uint8_t *)17, roda_red);
				eeprom_update_byte((uint8_t *)21, roda_green);

				trocadex = eeprom_read_byte((uint8_t*)20);//sobe_green;
				sobe_green = eeprom_read_byte((uint8_t*)16);//sobe_red;
				sobe_red = trocadex;
				eeprom_update_byte((uint8_t *)16, sobe_red);
				eeprom_update_byte((uint8_t *)20, sobe_green);

				trocadex = eeprom_read_byte((uint8_t *)22);//estica_green;
				estica_green = eeprom_read_byte((uint8_t*)18);//estica_red;
				estica_red = trocadex;
				eeprom_update_byte((uint8_t *)18, estica_red);
				eeprom_update_byte((uint8_t *)22, estica_green);
				
				nextfsm_troca_cores = 0;
				nextfsm_lcd = 2;
			}

			if (!s2_prev && s2)
			{
				int trocadex;

				trocadex = eeprom_read_byte((uint8_t*)25);//roda_blue;
				roda_blue = eeprom_read_byte((uint8_t*)21);//roda_green;
				roda_green = trocadex;
				eeprom_update_byte((uint8_t *)21, roda_green);
				eeprom_update_byte((uint8_t *)25, roda_blue);

				trocadex = eeprom_read_byte((uint8_t*)20);//sobe_green;
				sobe_green = eeprom_read_byte((uint8_t*)24);//sobe_blue;
				sobe_blue = trocadex;
				eeprom_update_byte((uint8_t *)20, sobe_green);
				eeprom_update_byte((uint8_t *)24, sobe_blue);

				trocadex = eeprom_read_byte((uint8_t *)22);//estica_green;
				estica_green = eeprom_read_byte((uint8_t *)26);//estica_blue;
				estica_blue = trocadex;
				eeprom_update_byte((uint8_t *)22, estica_green);
				eeprom_update_byte((uint8_t *)26, estica_blue);

				nextfsm_troca_cores = 0;
				nextfsm_lcd = 2;
			}
			break;
		case 4:
			lcd_init();
			lcd_xy(0, 0);
			lcd_puts("B1-> blue->red");
			lcd_xy(0, 1);
			lcd_puts("B2->blue->green");

			if (!s1_prev && s1)
			{
				int trocadex;

				trocadex = eeprom_read_byte((uint8_t*)25);//roda_blue;
				roda_blue = eeprom_read_byte((uint8_t*)17);//roda_red;
				roda_red = trocadex;
				eeprom_update_byte((uint8_t *)17, roda_red);
				eeprom_update_byte((uint8_t *)25, roda_blue);

				trocadex = eeprom_read_byte((uint8_t*)24);//sobe_blue;
				sobe_blue = eeprom_read_byte((uint8_t*)16);//sobe_red;
				sobe_red = trocadex;
				eeprom_update_byte((uint8_t *)16, sobe_red);
				eeprom_update_byte((uint8_t *)24, sobe_blue);

				trocadex = eeprom_read_byte((uint8_t*)26);//estica_green;
				estica_blue = eeprom_read_byte((uint8_t*)18);//estica_red;
				estica_red = trocadex;
				eeprom_update_byte((uint8_t *)18, estica_red);
				eeprom_update_byte((uint8_t *)26, estica_blue);

				nextfsm_troca_cores = 0;
				nextfsm_lcd = 2;
			}

			if (!s2_prev && s2)
			{
				int trocadex;

				trocadex = eeprom_read_byte((uint8_t*)25);//roda_blue;
				roda_blue = eeprom_read_byte((uint8_t*)21);//roda_green;
				roda_green = trocadex;
				eeprom_update_byte((uint8_t *)21, roda_green);
				eeprom_update_byte((uint8_t *)25, roda_blue);

				trocadex = eeprom_read_byte((uint8_t*)20);//sobe_green;
				sobe_green = eeprom_read_byte((uint8_t*)24);//sobe_blue;
				sobe_blue = trocadex;
				eeprom_update_byte((uint8_t *)20, sobe_green);
				eeprom_update_byte((uint8_t *)24, sobe_blue);

				trocadex = eeprom_read_byte((uint8_t*)22);//estica_green;
				estica_green = eeprom_read_byte((uint8_t*)26);//estica_blue;
				estica_blue = trocadex;
				eeprom_update_byte((uint8_t *)22, estica_green);
				eeprom_update_byte((uint8_t *)26, estica_blue);

				nextfsm_troca_cores = 0;
				nextfsm_lcd = 2;
			}
			break;
		}

		switch (fsm_lcd)
		{
		case 0:
			if (fsm2 != 1)
			{
				lcd_init();
				lcd_xy(0, 0);
				lcd_puts("B1-> novas pos.");
				lcd_xy(0, 1);
				lcd_puts("B2-> memoria");
			}
			if (fsm0 == 1)
			{
				nextfsm_lcd = 1;
			}
			if (fsm0 == 2)
			{
				nextfsm_lcd = 2;
			}
			break;
		case 1:

			if (fsm0 == 1)
			{
				nextfsm_lcd = 3;
			}
			break;
		case 2:
			lcd_init();
			lcd_xy(0, 0);
			lcd_puts("B1-> Correr");
			lcd_xy(0, 1);
			lcd_puts("B2-> Troca cores");

			if (fsm0 == 3)
			{
				lcd_init();
				nextfsm_lcd = 4;
			}
			if (!s2_prev && s2)
			{
				nextfsm_lcd = 7;
			}
			if (!s3_prev && s3)
			{
				lcd_init();
				nextfsm_lcd = 5;
			}
			break;
		case 3:
			if (!(fsm2 == 1))
			{
				if (roda_sensor == 100)
				{
					lcd_init();
					lcd_xy(0, 0);
					lcd_puts("B1-> sobe");
					lcd_xy(0, 1);
					lcd_puts("B2-> + roda");
				}
				else if (sobe_sensor == 100)
				{
					lcd_init();
					lcd_xy(0, 0);
					lcd_puts("B1-> estica");
					lcd_xy(0, 1);
					lcd_puts("B2-> + sobe");
				}
				else if (estica_sensor == 100)
				{
					lcd_init();
					lcd_xy(0, 0);
					lcd_puts("B1-> Peca 1");
					lcd_xy(0, 1);
					lcd_puts("B2-> + estica");
				}
				else if (roda_pos1 == 100)
				{
					lcd_init();
					lcd_xy(0, 0);
					lcd_puts("B1-> sobe");
					lcd_xy(0, 1);
					lcd_puts("B2-> + roda");
				}
				else if (sobe_pos1 == 100)
				{
					lcd_init();
					lcd_xy(0, 0);
					lcd_puts("B1-> estica");
					lcd_xy(0, 1);
					lcd_puts("B2-> + sobe");
				}
				else if (estica_pos1 == 100)
				{
					lcd_init();
					lcd_xy(0, 0);
					lcd_puts("B1-> Peca 2");
					lcd_xy(0, 1);
					lcd_puts("B2-> + estica");
				}
				else if (roda_pos2 == 100)
				{
					lcd_init();
					lcd_xy(0, 0);
					lcd_puts("B1-> sobe");
					lcd_xy(0, 1);
					lcd_puts("B2-> + roda");
				}
				else if (sobe_pos2 == 100)
				{
					lcd_init();
					lcd_xy(0, 0);
					lcd_puts("B1-> estica");
					lcd_xy(0, 1);
					lcd_puts("B2-> + sobe");
				}
				else if (estica_pos2 == 100)
				{
					lcd_init();
					lcd_xy(0, 0);
					lcd_puts("B1-> Peca 3");
					lcd_xy(0, 1);
					lcd_puts("B2-> + estica");
				}
				else if (roda_pos3 == 100)
				{
					lcd_init();
					lcd_xy(0, 0);
					lcd_puts("B1-> sobe");
					lcd_xy(0, 1);
					lcd_puts("B2-> + roda");
				}
				else if (sobe_pos3 == 100)
				{
					lcd_init();
					lcd_xy(0, 0);
					lcd_puts("B1-> estica");
					lcd_xy(0, 1);
					lcd_puts("B2-> + sobe");
				}
				else if (estica_pos3 == 100)
				{
					lcd_init();
					lcd_xy(0, 0);
					lcd_puts("B1-> Vermelho");
					lcd_xy(0, 1);
					lcd_puts("B2-> + estica");
				}
				else if (roda_red == 100)
				{
					lcd_init();
					lcd_xy(0, 0);
					lcd_puts("B1-> sobe");
					lcd_xy(0, 1);
					lcd_puts("B2-> + roda");
				}
				else if (sobe_red == 100)
				{
					lcd_init();
					lcd_xy(0, 0);
					lcd_puts("B1-> estica");
					lcd_xy(0, 1);
					lcd_puts("B2-> + sobe");
				}
				else if (estica_red == 100)
				{
					lcd_init();
					lcd_xy(0, 0);
					lcd_puts("B1-> Verde");
					lcd_xy(0, 1);
					lcd_puts("B2-> + estica");
				}
				else if (roda_green == 100)
				{
					lcd_init();
					lcd_xy(0, 0);
					lcd_puts("B1-> sobe");
					lcd_xy(0, 1);
					lcd_puts("B2-> + roda");
				}
				else if (sobe_green == 100)
				{
					lcd_init();
					lcd_xy(0, 0);
					lcd_puts("B1-> estica");
					lcd_xy(0, 1);
					lcd_puts("B2-> + sobe");
				}
				else if (estica_green == 100)
				{
					lcd_init();
					lcd_xy(0, 0);
					lcd_puts("B1-> Azul");
					lcd_xy(0, 1);
					lcd_puts("B2-> + estica");
				}
				else if (roda_blue == 100)
				{
					lcd_init();
					lcd_xy(0, 0);
					lcd_puts("B1-> sobe");
					lcd_xy(0, 1);
					lcd_puts("B2-> + roda");
				}
				else if (sobe_blue == 100)
				{
					lcd_init();
					lcd_xy(0, 0);
					lcd_puts("B1-> estica");
					lcd_xy(0, 1);
					lcd_puts("B2-> + sobe");
				}
				else if (estica_blue == 100)
				{
					lcd_init();
					lcd_xy(0, 0);
					lcd_puts("B1-> Fim");
					lcd_xy(0, 1);
					lcd_puts("B2-> + estica");
				}
				else if (tudo_definido && fsm0 == 2)
				{
					nextfsm_lcd = 2;
				}
			}
			break;
		case 4:
			if (fsm2 != 1)
			{
				lcd_init();
				lcd_xy(0, 0);
				lcd_puts("A correr");
				if(fsm0==5 || fsm0==6 || fsm0==7 || fsm0==8 || fsm0==11 || fsm0==12 || fsm0==13 || fsm0==14 || fsm0==17 || fsm0==18 || fsm0==19 || fsm0==20){
					imprime_cor(cor);
				}
			}
			
			if (fsm0 == 2)
			{
				nextfsm_lcd = 2;
			}

			break;
		case 5:
			if (!s3_prev && s3)
			{
				lcd_init();
				cor=ler_cor();
				imprime_cor(cor);
			}
			
			if (!s1_prev && s1)
			{
				nextfsm_lcd = 2;
			}
			break;
		case 6:
			lcd_init();
			lcd_xy(0, 0);
			lcd_puts(""); 
			break;
		case 7:

			break;
		}

		if (fsm0 != next_fsm0)
		{
			fsm0 = next_fsm0;
		}
		if (fsm1 != next_fsm1)
		{
			fsm1 = next_fsm1;
		}
		if (fsm2 != next_fsm2)
		{
			fsm2 = next_fsm2;
		}
		if (fsm_troca_cores != nextfsm_troca_cores)
		{
			fsm_troca_cores = nextfsm_troca_cores;
		}
		if (fsm_lcd != nextfsm_lcd)
		{
			fsm_lcd = nextfsm_lcd;
		}
	}
}