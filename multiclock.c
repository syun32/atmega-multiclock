#define F_CPU 16000000UL
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#define SWPORT PORTC
#define SWDDR DDRC
#define SWPIN PINC		//switch port
#define LEDPORT PORTA
#define LEDDDR DDRA
#define LEDPIN PINA		//LED port

#define SWX(x) ((SWPIN & 1<<(x-1)) == 1<<(x-1))   //Switch X PRESSED

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>

unsigned int sec = 0, msec = 0;


// USART 초기화 함수
void USART_Init(unsigned int ubrr) {
	UBRR0H = (unsigned char)(ubrr >> 8); //0으로 초기화
	UBRR0L = (unsigned char)ubrr; //그대로 출력
	UCSR0B = (1 << RXEN0) | (1 << TXEN0); //송수신 가능
	UCSR0C = (3 << UCSZ0);

}

// USART 데이터 전송 함수
void USART_Transmit(char data) {
	while (!((UCSR0A) & (1 << UDRE0))); //입력이 안들어오면 반복
	UDR0 = data; //UDR0에 데이터 저장
}

//USART STRING 전송 함수
void USART_Transmit_String(char *str) {
	while (*str != '\0') USART_Transmit(*str++);
}

// USART 데이터 수신 함수
char USART_Receive() {
	while (!(UCSR0A & (1 << RXC0))); //입력이 안들어오면 반복
	return UDR0; //UDR0 반환
}

//LCD에 char 문자 출력 하는 함수
void LCD_Transmit(int x, int y, char data) {

	USART_Transmit_String("$G,"); //커서 지정
	USART_Transmit(x + 48); //x 번째 줄
	USART_Transmit(',');
	USART_Transmit(y + 48); //y 번째 칸
	USART_Transmit('\r'); //종료
	USART_Transmit_String("$T,"); //문자열 출력
	USART_Transmit(data); //data 문자 출력
	USART_Transmit('\r');

}

//LCD에 String 문자열 출력 하는 함수
void LCD_Transmit_String(int x, int y, char *string) {
	USART_Transmit_String("$G,");
	USART_Transmit(x + '0');
	USART_Transmit(',');
	USART_Transmit(y + '0');
	USART_Transmit('\r');
	USART_Transmit_String("$T,");
	USART_Transmit_String(string);
	USART_Transmit('\r');
}

//TIMER interrupt 초기화
void Timer_Init() {
	TCCR0 = (1 << WGM00) | (1 << COM01) | (4 << CS0);
	TIMSK = (1 << OCIE0);
}

ISR(TIMER0_COMP_vect) {
	msec++;
	if (msec == 1000) {
		msec = 0;
		sec++;
	}
	if (sec == 100) {
		sec = 0;
	}
}

void front() {     //초기화면 띄우기
	LCD_Transmit_String(1, 1, "1CLOCK 2TIMER   ");
	LCD_Transmit_String(2, 1, "3STOPWATCH 4GAME");

}

//시간을 시분초로 문자열에 저장하여 LCD에 출력
void LCD_Time(int x, int y, int t) {
	char caTime[] = "00:00:00";
	caTime[0] = '0' + (t / 3600) / 10;
	caTime[1] = '0' + (t / 3600) % 10;
	caTime[3] = '0' + (t % 3600) / 60 / 10;
	caTime[4] = '0' + (t % 3600) / 60 % 10;
	caTime[6] = '0' + (t % 3600) % 60 / 10;
	caTime[7] = '0' + (t % 3600) % 60 % 10;
	LCD_Transmit_String(x, y, caTime);
}

void clk();
void stopwatch();
void timer();
void gm();		//기능 함수의 선언

int main(void)
{
	USART_Init(MYUBRR);		//초기화
	USART_Transmit_String("$I\r");
	Timer_Init();
	LEDDDR = 0xFF;
	SREG = 0x80;
	int state = 0;
	while (1)
	{
		switch (state) {		//기능 함수 실행
		case 0:
			front();
			break;
		case 1:
			clk();
			break;
		case 2:
			timer();
			break;
		case 3:
			stopwatch();
			break;
		case 4:
			gm();
			break;
		default:
			break;
		}
		if (SWX(1)) state = 1;		//switch로 메뉴 선택
		else if (SWX(2)) state = 2;
		else if (SWX(3)) state = 3;
		else if (SWX(4)) state = 4;
		else state = 0;          //break로 빠져나오는 경우 state는 보통 0이 됨->초기화면
		_delay_ms(100);
	}
}

void clk() {
stt:
	USART_Transmit_String("$I\r");
	LCD_Transmit_String(1, 1, "1 GO TIME_SET");
	LCD_Transmit_String(2, 1, "8 HOME");		//clock 초기화면
	unsigned int u_sec = 0, cnt = 0;
	unsigned int t_hour = 0, t_min = 0, t_sec = 0, tt = 0;

	while (1) {
		if (SWX(1)) {
			while (1) {		//Timer start
				if (SWX(1)) {
					if (cnt == 1) {
						LCD_Transmit_String(2, 1, "8 END");		//clock 실행 중
					}
				}
				else {
					if (cnt == 0) {
						USART_Transmit_String("$I\r");
						while (1) {		//time set
							tt = t_sec + t_min * 60 + t_hour * 3600;
							LCD_Transmit_String(1, 1, "SET  ");
							LCD_Time(1, 6, tt);
							LCD_Transmit_String(2, 1, "1hr 2min 3s 4set");
							if (SWX(1)) {		//hour++
								if (t_hour == 24) {
									t_hour = 0;
								}
								else {
									t_hour++;
								}
							}
							if (SWX(2)) {		//minute++
								if (t_min == 60) {
									t_min = 0;
								}
								else {
									t_min++;
								}
							}
							if (SWX(3)) {		//second++
								if (t_sec == 60) {
									t_sec = 0;
								}
								else {
									t_sec++;
								}
							}
							if (SWX(4)) {		//set, tt에 설정 시간 저장
								tt = t_sec + t_min * 60 + t_hour * 3600;
								cnt = 1;
								sec = 0;
								u_sec = 0;
								USART_Transmit_String("$I\r");
								break;
							}
						}
						cnt = 1;
					}
					else {
						u_sec = tt + sec;	//설정 시간+흐르는 시간
						LCD_Transmit_String(1, 1, "TIME ");
						LCD_Time(1, 6, u_sec);
						LCD_Transmit_String(2, 1, "8 END");		//clock 화면
					}
				}//no switch go menu
				if (SWX(8)) {
					goto stt;
				}
			}
		}
		else if (SWX(8)) {		//Go home
			return;
		}
		_delay_ms(50);
	}
	USART_Transmit_String("$I\r");
}

void timer() {
stt:
	USART_Transmit_String("$I\r");
	LCD_Transmit_String(1, 1, "1 GO TIMER");
	LCD_Transmit_String(2, 1, "8 HOME");		//timer menu
	unsigned int u_sec = 0, cnt = 0;
	unsigned int t_hour = 0, t_min = 0, t_sec = 0, tt = 0;

	while (1) {
		if (SWX(1)) {
			while (1) {   //timer start
				if (SWX(1)) {
					if (cnt == 1) {
						LCD_Transmit_String(2, 1, "8 END");		//timer 실행 중
					}
				}
				else {
					if (cnt == 0) {
						USART_Transmit_String("$I\r");
						while (1) {		//time set
							tt = t_sec + t_min * 60 + t_hour * 3600;
							LCD_Transmit_String(1, 1, "SET  ");
							LCD_Time(1, 6, tt);
							LCD_Transmit_String(2, 1, "1hr 2min 3s 4set");
							if (SWX(1)) {		//hour++
								if (t_hour == 24) {
									t_hour = 0;
								}
								else {
									t_hour++;
								}
							}
							if (SWX(2)) {		//minute++
								if (t_min == 60) {
									t_min = 0;
								}
								else {
									t_min++;
								}
							}
							if (SWX(3)) {		//second++
								if (t_sec == 60) {
									t_sec = 0;
								}
								else {
									t_sec++;
								}
							}
							if (SWX(4)) {		//time set
								tt = t_sec + t_min * 60 + t_hour * 3600;	//설정 시간 저장
								cnt = 1;
								sec = 0;		//time 초기화
								u_sec = 0;
								USART_Transmit_String("$I\r");
								break;
							}
						}
						cnt = 1;
					}
					else {
						u_sec = tt - sec;		//설정 시간-흐르는 시간
						LCD_Transmit_String(1, 1, "TIME ");
						LCD_Time(1, 6, u_sec);
						LCD_Transmit_String(2, 1, "8END");
						if (u_sec == 0) {		//남은 시간 0
							LCD_Transmit_String(1, 1, "TIME OUT!       ");
							for (int i = 0; i < 4; i++) {		//LED blink
								LEDPORT = 0xFF;
								_delay_ms(300);
								LEDPORT = 0x00;
								_delay_ms(300);
							}
							_delay_ms(2000);
							goto stt;		//go menu
						}
					}
				}//no switch
				if (SWX(8)) {
					goto stt;
				}
			}
		}
		else if (SWX(8)) {		//go home
			return;
		}
		_delay_ms(50);
	}
}

void stopwatch() {
	unsigned int u_sec = 0, cnt = 0;
	USART_Transmit_String("$I\r");
	USART_Transmit_String("$C\r");


	while (1) {		//stopwatch
		LCD_Transmit_String(1, 1, "STWC ");
		LCD_Time(7, 1, u_sec);
		LCD_Transmit_String(2, 1, "1STOP 2CLR 8HOME");		//stwc main

		if (SWX(1)) {
			if (cnt == 1) {		//when time go, stop
				cnt = 0;
			}
			else {				//when time stop, go
				sec = u_sec;
				cnt = 1;
			}
		}
		if (SWX(2)) {		//time 초기화
			sec = 0;
			u_sec = 0;
		}
		else {
			if (cnt == 1) {
				u_sec = sec;    //시간 동기화
			}

		}

		//code for stopwatch
		if (SWX(8)) {
			return;
		}
		_delay_ms(100);
	}
}

void gm() {
stt:
	USART_Transmit_String("$I\r");
	LCD_Transmit_String(1, 1, "1 START");
	USART_Transmit('\r');
	LCD_Transmit_String(2, 1, "8 HOME");		//menu
	USART_Transmit('\r');
	int lv = 1, i, r, ans;
	int b[6];
	char scr[4];
	unsigned sd = 0;
	while (1) {	
		if (SWX(1)) {		//game start
			srand(sd);		//random seed
			for (lv = 1;; lv++) {		//round
				sprintf(scr, "%d\0", lv);
				b[0] = b[1] = b[2] = 0;
				for (i = 0; i < lv; i++) {
					USART_Transmit_String("$I\r");
					LCD_Transmit_String(1, 1, scr);
					r = rand() % 7 + 1;
					b[3] = r % 2;
					r >>= 1;
					b[4] = r % 2;
					r >>= 1;
					b[5] = r % 2;		//0 or 1
					b[0] += b[3];
					b[1] += b[4];
					b[2] += b[5];		//정답 저장
					if (b[3])LCD_Transmit(1, 5, 'O');
					if (b[4])LCD_Transmit(1, 7, 'O');
					if (b[5])LCD_Transmit(1, 9, 'O');		//random하게 저장된 값이 1이라면, 0 출력
					USART_Transmit('\r');
					_delay_ms(500);
				}
				USART_Transmit_String("$I\r");
				LCD_Transmit_String(1, 1, scr);
				LCD_Transmit_String(2, 1, "ANSWER?");
				USART_Transmit('\r');
				while (1) {		//switch로 정답 입력
					if (SWX(1)) {
						ans = 0;
						break;
					}
					else if (SWX(2)) {
						ans = 1;
						break;
					}
					else if (SWX(3)) {
						ans = 2;
						break;
					}
					else _delay_ms(100);
				}
				USART_Transmit_String("$I\r");
				if (b[3])LCD_Transmit(1, 5, 'O' + b[0]);
				if (b[4])LCD_Transmit(1, 7, 'O' + b[1]);
				if (b[5])LCD_Transmit(1, 9, 'O' + b[2]);
				USART_Transmit('\r');
				if (b[ans] >= b[0] && b[ans] >= b[1] && b[ans] >= b[2]) {		//정답을 맞춘 경우
					LCD_Transmit_String(1, 1, "GOOD");
					_delay_ms(2000);
				}
				else {		//정답을 틀린 경우
					LCD_Transmit_String(1, 1, "GAME OVER");
					_delay_ms(2000);
					goto stt;
				}
			}
		}
		else if (SWX(8)) {		//go home
			return;
		}
		_delay_ms(50);
		sd++;		//random seed 변경
	}
}