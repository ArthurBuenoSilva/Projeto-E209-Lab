#define MOTOR (1 << PD3)
#define BOTAO (1 << PD2)

#define PARADO 0
#define FUNCIONANDO 1

unsigned short int estado = PARADO;
unsigned int count_1 = 0;
unsigned int count_5 = 0;
unsigned int count_10 = 0;
unsigned int adc_result;
float tensao;

void ADC_init(void) {
  // Configurando Vref para VCC = 5V
  ADMUX = (1 << REFS0);
  /*
    ADC ativado e preescaler de 128
    16MHz / 128 = 125kHz
    ADEN = ADC Enable, ativa o ADC
    ADPSx = ADC Prescaler Select Bits
    1 1 1 = clock / 128
  */
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

int ADC_read(uint8_t ch) {
  char i;
  int ADC_temp = 0;                     // ADC temporário, para manipular leitura
  int ADC_read = 0;                     // ADC_read
  ch &= 0x07;
  ADMUX = (ADMUX & 0xF8) | ch;
  ADCSRA |= (1 << ADSC);                // Faça uma conversão
  while (!(ADCSRA & (1 << ADIF)));       // Aguarde a conversão do sinal
  for (i = 0; i < 8; i++)                // Fazendo a conversão 8 vezes para maior precisão
  {
    ADCSRA |= (1 << ADSC);               // Faça uma conversão
    while (!(ADCSRA & (1 << ADIF)));     // Aguarde a conversão do sinal
    ADC_temp = ADCL;                     // lê o registro ADCL
    ADC_temp += (ADCH << 8);             // lê o registro ADCH
    ADC_read += ADC_temp;                // Acumula o resultado (8 amostras) para média
  }
  ADC_read = ADC_read >> 3;              // média das 8 amostras ( >> 3 é o mesmo que /8)
  return ADC_read;
}

int main() {
  Serial.begin(9600);
  ADC_init();

  // Ativa interrupção no Timer 0 - A
  TCCR0A = (1 << WGM01) + (0 << WGM00);
  TCCR0B = (1 << CS01) + (1 << CS00);
  TIMSK0 = (1 << OCIE0A);
  OCR0A = 249;

  // Ativa o PWM no Comparador B - Timer 2
  TCCR2A = (1 << COM1A1) + (1 << COM1B1) + (1 << WGM11) + (1 << WGM10);
  TCCR2B = (1 << CS12) + (1 << CS10);
  OCR2B = 0;

  DDRD = MOTOR;
  PORTD = BOTAO;
  PORTD |= MOTOR;

  sei();

  for(;;) {
    adc_result = ADC_read(ADC4D);
    tensao = (adc_result * 5.0) / 1023.0;

    if (tensao >= 1.25) {
      OCR2B = int((tensao * 255.0) / 5.0);
    } else {
      estado = PARADO;
      OCR2B = 0;
    }
  } 
}

ISR(TIMER0_COMPA_vect) {
  count_1++;

  if (count_1 >= 1000 && count_1 <= 1200) {
    if ((PIND & BOTAO) == 0)
      estado = FUNCIONANDO;

    count_1 = 0;
  }

  count_5++;
  if (count_5 >= 5000 && count_5 <= 5500) {
    if (Serial.available() > 0) { // Verifica se há dados disponíveis na porta serial
      char dadoRecebido = Serial.read(); // Lê um caractere da porta serial

      if (dadoRecebido == 'M') {
        Serial.print("Concentracao de gas: ");
        Serial.print((tensao / 5.0) * 100.0);
        Serial.println("%");
      }
    }

    count_5 = 0;
  }

  count_10++;
  if (count_10 >= 10000 && count_10 <= 11000) {
    if (Serial.available() > 0) { // Verifica se há dados disponíveis na porta serial
      char dadoRecebido = Serial.read(); // Lê um caractere da porta serial

      if (dadoRecebido == 'S') {
        Serial.print("Estado do motor: ");
        
        if (estado == PARADO)
          Serial.println("Parado");
        else if (estado == FUNCIONANDO)
          Serial.println("Funcionando");
      }
    }

    count_10 = 0;
  }
}
