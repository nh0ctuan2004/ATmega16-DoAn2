#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>

// --- Dinh nghia DHT11 (PORTB - PB4) ---
#define DHT_PORT PORTB
#define DHT_DDR  DDRB
#define DHT_PIN  PINB
#define DHT_BIT  PB4 // Cam chan Data vao PB4

// --- Dinh Nghia LCD PORT A ---
#define LCD_PORT PORTA
#define LCD_DDR  DDRA
#define LCD_RS   1 // PA1
#define LCD_RW   2 // PA2
#define LCD_EN   3 // PA3
// D4-D7 noi vào PA4-PA7

// --- Dinh nghia nut nhan (PORTB) ---
#define BTN_1 PB0 // Auto Xuoi + Hieu ung 1
#define BTN_2 PB1 // Chinh tay Tang + Hieu ung 2
#define BTN_3 PB2 // Chinh tay Giam + Hieu ung 3
#define BTN_4 PB3 // Auto Nguoc + Hieu ung 4

// --- Bien toan cuc ---
int count = 0;       // Gia tri so hien thuc (0-9)
int current_mode = 0; // Che do hien tai (0,1,2,3,4)
int timer_tick = 0;   // Bien dem thoi gian cho Auto
int sensor_timer = 0; // Dem thoi gian doc cam bien
// Bien luu nhiet do, do am
uint8_t dht_temp = 0;
uint8_t dht_hum = 0;
uint16_t air_quality = 0; // BIEN LUU GIA TRI MQ-135
// ### KHAI BAO HAM ###
void INIT();
void LED7_OUT(unsigned char num);
int delay_and_control(int ms); // Ham quan trong nhat
// Cac ham LCD
void LCD_Init();
void LCD_Command(unsigned char cmnd);
void LCD_Char(unsigned char data);
void LCD_String(const char *str);
void LCD_Goto(unsigned char x, unsigned char y);
void Update_LCD_Screen(); // Hàm cap nhat màn hình
// DHT11
uint8_t DHT_Read_Data(uint8_t *temperature, uint8_t *humidity);
// ADC (MQ-135)
void ADC_Init();                   
uint16_t ADC_Read(uint8_t channel); 
// -Các hieu ung LED don-
void Effect_1_Running();
void Effect_2_Filling();
void Effect_3_Alternating();
void Effect_4_Strobe();

int main()
{
	INIT();
	current_mode = 0; // Mac dinh vào là so 0, không làm gì
	count = 0;
	
	// Man hinh chao mung
	LCD_Goto(0, 2);
	LCD_String("20224189-K67");
	LCD_Goto(1, 0);
	LCD_String("18-Le Minh Tuan");
	_delay_ms(2000); // Cho cam bien nong
	//Doc thu 1 lan dau tien
	DHT_Read_Data(&dht_temp, &dht_hum);
	_delay_ms(1000);
	
	while(1)
	{
		// Luôn hien thi so ra LED 7 thanh
		LED7_OUT(count);
		
		// Chay hieu ung voi Mode
		switch(current_mode)
		{
			case 0:
			PORTD = 0xFF; // Tat het 8 LED don (Active Low)
			// Goi hàm delay de quét nút nhan, nhung không chay hieu ung gi
			delay_and_control(100);
			break;
			case 1: Effect_1_Running(); break;
			case 2: Effect_2_Filling(); break;
			case 3: Effect_3_Alternating(); break;
			case 4: Effect_4_Strobe(); break;
		}
	}
	return 0;
}

// Tra ve 1 neu có nút an, tra ve 0 neu bình thuong
int delay_and_control(int ms)
{
	Update_LCD_Screen();
	// Chia nho thoi gian delay thành tung goi 10ms
	for(int i = 0; i < ms; i += 10)
	{
		_delay_ms(10);
		
		// --- LOGIC DOC CAM BIEN ---
		sensor_timer++;
		if(sensor_timer % 200 == 0) {
			DHT_Read_Data(&dht_temp, &dht_hum);
			air_quality = ADC_Read(2); // Doc MQ-135 o chan PA2
		}
		if(sensor_timer >= 400) {
			sensor_timer = 0;
		}
		// 1. XU LY DEM TU DONG (Cho Mode 1 va 4)
		if(current_mode == 1 || current_mode == 4)
		{
			timer_tick++;
			if(timer_tick >= 100) // 10ms * 100 = 1000ms = 1 Giây
			{
				timer_tick = 0;
				if(current_mode == 1) { // Auto Xuôi
					count++;
					if(count > 9) count = 0;
				}
				else { // Auto Nguoc
					count--;
					if(count < 0) count = 9;
				}
				LED7_OUT(count); // Cap nhat so moi giay
				Update_LCD_Screen();
			}
		}

		// 2. XU LY NUT NHAN (CHUYEN MODE & CHINH TAY)
		
		// --- Nut 1: Auto Xuoi ---
		if(bit_is_clear(PINB, BTN_1)) {
			current_mode = 1;
			timer_tick = 0; // Reset b? ??m giây
			return 1; // Bao hieu thoat hieu ?ng c?
		}
		
		// --- Nút 4: Auto Nguoc ---
		if(bit_is_clear(PINB, BTN_4)) {
			current_mode = 4;
			timer_tick = 0;
			return 1;
		}

		// --- Nút 2: Tang tay (Manual Up) ---
		if(bit_is_clear(PINB, BTN_2)) {
			_delay_ms(20); // Chong rung
			if(bit_is_clear(PINB, BTN_2)) {
				// Neu chua o Mode 2 thì chuyen Mode, neu dang o Mode 2 roi thì tang so
				current_mode = 2;
				count++;
				if(count > 9) count = 0;
				LED7_OUT(count); // Hien so ngay
				Update_LCD_Screen();
				while(bit_is_clear(PINB, BTN_2)); // Chua nha nút
				return 1; // Reset hieu ung ?? ch?y l?i t? ??u
			}
		}

		// --- Nút 3: Gi?m tay (Manual Down) ---
		if(bit_is_clear(PINB, BTN_3)) {
			_delay_ms(20);
			if(bit_is_clear(PINB, BTN_3)) {
				current_mode = 3;
				count--;
				if(count < 0) count = 9;
				LED7_OUT(count);
				Update_LCD_Screen();
				while(bit_is_clear(PINB, BTN_3));
				return 1;
			}
		}
	}
	return 0; // Het thoi gian delay ma khong co nut nao duoc an
}

//Cac ham hieu ung (Su dung delay_and_control) ---

void Effect_1_Running() // Chay duoi
{
	for(int i = 0; i < 8; i++) {
		PORTD = (unsigned char)~(1 << i); // Active Low
		if(delay_and_control(200)) return; // Neu có nút an -> thoat ngay
	}
}

void Effect_2_Filling() // Sang den
{
	unsigned char pattern = 0;
	for(int i = 0; i < 8; i++) {
		pattern |= (1 << i);
		PORTD = (unsigned char)~pattern;
		if(delay_and_control(200)) return;
	}
	PORTD = 0xFF; // Tat
	if(delay_and_control(200)) return;
}

void Effect_3_Alternating() // Chan~ le?
{
	PORTD = (unsigned char)~0x55; // Cha~n
	if(delay_and_control(300)) return;
	PORTD = (unsigned char)~0xAA; // Le?
	if(delay_and_control(300)) return;
}

void Effect_4_Strobe() // Nhay loanxa (cho chu so dem nguoc)
{
	PORTD = 0x00; // Sang het
	if(delay_and_control(50)) return;
	PORTD = 0xFF; // Tat het
	if(delay_and_control(50)) return;
}

// --- KHOI TAO ---
void INIT()
{
	DDRC = 0xFF; PORTC = 0xFF;
	DDRD = 0xFF; PORTD = 0xFF;
	DDRB = 0x00; PORTB = 0xFF;
	
	//Khoi tao LCD
	ADC_Init();
	LCD_Init();
}

//HAM LED 7 THANH
void LED7_OUT(unsigned char num)
{
	unsigned char current_dot = PORTC & 0B00000000;
	unsigned char ma_led = 0xFF;
	switch(num) {
		case 0: ma_led = 0B10000000; break;
		case 1: ma_led = 0B11100011; break;
		case 2: ma_led = 0B01000100; break;
		case 3: ma_led = 0B01000001; break;
		case 4: ma_led = 0B00100011; break;
		case 5: ma_led = 0B00010001; break;
		case 6: ma_led = 0B00010000; break;
		case 7: ma_led = 0B11000011; break;
		case 8: ma_led = 0B00000000; break;
		case 9: ma_led = 0B00000001; break;
	}
	ma_led &= 0B11110111;
	PORTC = ma_led | current_dot;
}

// --- CAC HAM XU LY LCD (4-BIT) ---

void LCD_Enable() {
	LCD_PORT |= (1 << LCD_EN);
	_delay_us(10);
	LCD_PORT &= ~(1 << LCD_EN);
	_delay_us(50);
}

void LCD_Send4Bit(unsigned char Data) {
	// Chi thay doi 4 bit cao (PA4-PA7), giu nguyen PA0-PA3
	// PA0 là Input (VR1), PA1-PA3 la Control
	unsigned char current_control = LCD_PORT & 0x0F;
	LCD_PORT = (Data & 0xF0) | current_control;
	LCD_Enable();
}

void LCD_Command(unsigned char cmnd) {
	LCD_PORT &= ~(1 << LCD_RS); // RS = 0 (Lenh)
	// LCD_PORT &= ~(1 << LCD_RW); // RW = 0 (Ghi) // GND
	LCD_Send4Bit(cmnd);      // Gui 4 bit cao
	LCD_Send4Bit(cmnd << 4); // Gui 4 bit thap
	_delay_ms(2);
}

void LCD_Char(unsigned char data) {
	LCD_PORT |= (1 << LCD_RS);  // RS = 1 (Du lieu)
	// LCD_PORT &= ~(1 << LCD_RW); // RW = 0 (Ghi) //GND
	LCD_Send4Bit(data);
	LCD_Send4Bit(data << 4);
	_delay_us(50);
}

void LCD_Init() {
	// PA1-PA7 là Output
	LCD_DDR |= 0xFA; //PA2 la input
	LCD_PORT &= 0x05; // Xoa output, giu PA0 (neu có pullup)
	
	_delay_ms(20);
	LCD_Command(0x33); //Soft reset
	LCD_Command(0x32); //Chuyen sang 4 bit
	LCD_Command(0x28); // 4-bit, 2 lines
	LCD_Command(0x0C); // Bat hien thi, tat con tr?
	LCD_Command(0x01); // Xoa man hinh
	_delay_ms(2);
}

void LCD_String(const char *str) {
	for(int i = 0; str[i] != 0; i++) LCD_Char(str[i]);
}

void LCD_Goto(unsigned char x, unsigned char y) {
	unsigned char address[] = {0x80, 0xC0};
	LCD_Command(address[x] + y);
}

// Ham hien thi thong tin ra man hinh
void Update_LCD_Screen() {
	// --- Dong 1: Mode X: Cnt: Y ---
	LCD_Goto(0, 0);
	
	LCD_String("(M");             // Mo ngoac va chu M
	LCD_Char(current_mode + '0'); // So che do
	LCD_String(")");        // Dong ngoac va nhan dem
	LCD_Char(count + '0');        // So dem
	
	LCD_String(" A=");
	if(air_quality >= 1000) LCD_Char((air_quality/1000)+'0');
	LCD_Char(((air_quality/100)%10)+'0');
	LCD_Char(((air_quality/10)%10)+'0');
	LCD_Char((air_quality%10)+'0');
	if(air_quality > 400) LCD_String(" BAD");
	else LCD_String(" OK ");
	// In them vai khoang trang cuoi dong de xoa ky tu rac neu so nhay lung tung
	LCD_String("           ");

	// --- DONG 2: T: XX H: YY ---
	// MAU HIEN THI: "T:25C H:70%     "
	LCD_Goto(1, 0);
	
	LCD_String("  T=");
	LCD_Char((dht_temp / 10) + '0'); // HANG CHUC
	LCD_Char((dht_temp % 10) + '0'); // HANG DON VI
	LCD_Char(0xDF); // Ky hieu "do PC"
	LCD_Char('C'); // DON VI DO C
	
	LCD_String(" H=");
	LCD_Char((dht_hum / 10) + '0');
	LCD_Char((dht_hum % 10) + '0');
	LCD_Char('%'); // DON VI %
	
	// XOA KY TU THUA CUOI DONG CHO SACH DEP
	LCD_String("      ");
}

// --- Ham giao tiep DHT11 ---
uint8_t DHT_Read_Data(uint8_t *temperature, uint8_t *humidity)
{
	uint8_t bits[5];
	uint8_t i, j = 0;

	// 1. Gui tin hieu Start
	DHT_DDR |= (1 << DHT_BIT);   // Output
	DHT_PORT &= ~(1 << DHT_BIT); // Keo xuong 0
	_delay_ms(20);               // Giu it nhat 18ms
	DHT_PORT |= (1 << DHT_BIT);  // Keo len 1
	DHT_DDR &= ~(1 << DHT_BIT);  // Chuyen sang Input de nghe

	// 2. Cho` DHT11 phan hoi (Check Pulse)
	_delay_us(40);
	if (DHT_PIN & (1 << DHT_BIT)) return 0; // Error: Khong thay DHT keo xuong
	_delay_us(80);
	if (!(DHT_PIN & (1 << DHT_BIT))) return 0; // Loi: Khong thay DHT keo len
	_delay_us(80);

	// 3. Doc 40 bits du lieu (5 bytes)
	for (j = 0; j < 5; j++) {
		uint8_t result = 0;
		for (i = 0; i < 8; i++) {
			while (!(DHT_PIN & (1 << DHT_BIT))); // Cho` bit bat dau len 1
			_delay_us(30); // Doi 30us de xem no la 0 hay 1
			if (DHT_PIN & (1 << DHT_BIT)) result |= (1 << (7 - i)); // Neu van la 1 => Bit 1
			while (DHT_PIN & (1 << DHT_BIT)); // Cho xuong 0 cho bit tiep theo
		}
		bits[j] = result;
	}

	// 4. Kiem tra Checksum (Byte 5 = Tong 4 byte dau)
	if ((uint8_t)(bits[0] + bits[1] + bits[2] + bits[3]) == bits[4]) {
		*humidity = bits[0];    // Byte 0 la do am nguyen
		*temperature = bits[2]; // Byte 2 la nhiet do nguyen
		return 1; // Success
	}
	return 0; // Sai Checksum
}
// HAM KHOI TAO ADC
void ADC_Init() {
	// REFS0=1: Vref = AVCC (5V)
	ADMUX = (1<<REFS0);
	// ADEN=1: Bat ADC. Chia xung 128 (8MHz/128 = 64kHz)
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}
// HAM DOC ADC
uint16_t ADC_Read(uint8_t channel) {
	channel &= 0b00000111; // Loc kenh 0-7
	ADMUX = (ADMUX & 0xF8) | channel;
	ADCSRA |= (1<<ADSC); // Bat dau chuyen doi
	while(!(ADCSRA & (1<<ADIF))); // Cho xong
	ADCSRA |= (1<<ADIF); // Xoa co
	return (ADC); // Tra ve gia tri 0-1023
}