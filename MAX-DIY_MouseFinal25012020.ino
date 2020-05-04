/* Это прошивка для Arduino Pro Micro для автоматизации использования модулей и оружия в Crossout
 *  
 * Она подразумевает наличие :
 * - трёх кнопок которые нажимает юзер сообщая программе исправен ли модуль (Мдл1, Мдл2, Мдл3)
 *  (в начале программы считается что все модули исправны, клик по кнопке модуля означает что модуль выбит)
 * - двух кнопок которые отвечают за активацию этих модулей и переходы по режимам (Кн1, Кн2)
 * - трёх светодиодов, сигнализирующих о том, исправен ли модуль(led1,led2,led3)
 * 
 * Все переменные, отвечающие за пины подключения кнопок и светодиодов представлены в начале программы,
 * переназначайте на здоровье :D
 * 
 * Есть 3 режима:
 * 1) Нормальный режим. Активируется после даблклика Кн1 в начале программы
 *  - активен только когда исправны все модули
 *  - по очереди прожимает активацию первого, второго и третьего модулей 
 *  - по одиночному клику на Кн1 прожимает только что нажатый модуль ещё раз
 *  - по двойному клику на Кн1 переходит на один модуль назад
 *  - по одиночному клику на Кн2 активирует модуль №4
 *  - по двойному клику на Кн2 переходит в Лениво-Аварийный Режим (ЛАР)
 *  - горят все светодиоды
 *   
 * 2) Аварийный режим. Активируется когда уничтожен хотя бы один из модулей
 *  - активен пока есть хотя бы один уничтоженный модуль
 *  - модули с первого по третий активируются по нажатию Кн1
 *  - по одиночному клику на Кн2 активирует модуль №4
 *  - по двойному клику на Кн2 переходит в Лениво-Аварийный Режим (ЛАР)
 *  - горят светодиоды исправных модулей
 * 
 * 3) Лениво-Аварийный Режим (ЛАР)
 *  - активен до двойного клика Кн2
 *  - по очереди прожимает кнопки первого, вторго, третьего и четвёртого модулей
 *  - светодиоды исправных модулей мигают
 *  - по одиночному клику на Кн1 прожимает только что нажатый модуль ещё раз
 *  - по двойному клику на Кн1 переходит на один модуль назад
 *  
 * Если зажать Кн1 на Btn1Hold секунд - устройство перезапускается
 * 
 * Написано SozDM в 2020 году
*/

#include "Keyboard.h"
#include "GyverButton.h"

#define KeyDelay 4150            //задержка между автоматическими нажатиями кнопок модулей (мсек)
#define HighBright 240           //яркость светодиода (ШИМ, от 1 до 255)
#define Btn1Hold 3               //число проходов isStep() чтобы кнопка считалась удержанной (сек)
#define LazyEmerBlink 500        //как часто мигают диоды в ЛАР (мсек)

//short Btn[5] = {4, 5, 2, 3, 6};     //отладочный вариант: номера выводов для кнопок
short Btn[5] = {2, 3, 4, 5, 6};     //основной вариант: номера выводов для кнопок
short led[3] = {7, 8, 9};           //номера выводов для светодиодов

GButton Btn1(Btn[0]);
GButton Btn2(Btn[1]);
GButton Mdl1(Btn[2]);
GButton Mdl2(Btn[3]);
GButton Mdl3(Btn[4]);

bool banner = 0;                      //если 0 то надо вывести в порт сообщение о перезапуске

unsigned long B1Time = 0;             //время, прошедшее с момента нажатия B1

char CharOfModule[5] = {'0','1','2','3','4'};   //Здесь можно поменять кнопки, которые отвечают за модули с 1го по 4й
                                                //первый элемент массива не используется

short MdlClick[3] = {0, 0, 0};

bool MdlsLost [4] = {0, 0, 0, 0};           //номера модулей, которые выбил противник
bool WasReset = 0;                          //нужно для немедленного начала действия нормального
                                            //режима после выхода из аварийного

short CurrentKey = 1;             //то, какая клавиша модуля ("1", "2" или "3") сейчас нажата на клавиатуре
short NextMode = 0;               //перейти в режим : 1 - NormMode, 2 - EmerMode, 3 - LazyEmerMode, 4 - программа идёт с начала

void setup()
{
  //инициализация кнопок
  for (short i = 0; i < 5; i ++) pinMode(Btn[i], INPUT_PULLUP);

  //инициализация светодиодов
  for (short i = 0; i < 5; i ++) pinMode(led[i], OUTPUT);
  
  Keyboard.begin();
  Serial.begin(9600);
}

void loop() {                               //основной цикл программы. для перезагрузки NextMode == 4

  Keyboard.releaseAll();

  for (short i = 0; i < 3; i ++) digitalWrite(led[i], LOW);

  Btn1.tick();
  Btn2.tick();
  Mdl1.tick();
  Mdl2.tick();
  Mdl3.tick();

  if (WasReset == 1)    NextMode = 1;
  else NextMode = 4;

  if (banner == 0)
  {
    Serial.println("0.программа запущена");
    banner = 1;
  }

  for (short i = 0; i < 3; i ++) MdlsLost[i] = 0;

  if (Btn1.isDouble())                   //если даблклик по первой кнопке - то идём в нормальный режим
  {
    Serial.println("0.кнопка 1 даблклик");
    NextMode = 1;
  }

  while (NextMode != 4)             //меню выбора режима :D если NextMode == 4 то программа пойдёт с начала
  {
    if (NextMode == 1)
    {
      Serial.println("0->1.переход в нормальный режим");
      NextMode = NormMode();
    }

    if (NextMode == 2)
    {
      Serial.println("0->2.переход в аварийный режим");
      NextMode = EmerMode();
    }

    if (NextMode == 3)
    {
      Serial.println("0->3.переход в лениво аварийный режим");
      LazyEmerMode();
    }
  }
  if(B1Time > millis()) B1Time = 0;
  
}


//нормальный режим работы. NextMode == 1
int NormMode()                                
{
  unsigned long t = millis();

  //нужно ли начинать цикл модули с 1го по 3й
  bool Start123 = 0;                          

  for (short i = 0; i < 3; i++) digitalWrite(led[i], HIGH);

  CurrentKey = 1;
  if (WasReset == 0)
  {
    Serial.println("1.Нажимаю LShift");
    Keyboard.press(KEY_LEFT_SHIFT);
  }
  Serial.println("1.нажимаю кнопку 'w'");
  Keyboard.press('w');
  if (WasReset == 0)
  {
    Serial.println("1.отпускаю LShift");
    Keyboard.release(KEY_LEFT_SHIFT);
  }

  bool Btn1isSingle = 0;

  while (NextMode == 1)
  {
    Btn1.tick();
    Btn2.tick();
    Mdl1.tick();
    Mdl2.tick();
    Mdl3.tick();

    if (Btn1.isSingle()) Btn1isSingle = 1;
    else Btn1isSingle = 0;

    //если кнопка1 была нажата - то начинаем перебирать модули
    if (Btn1isSingle == 1 or  WasReset == 1) Start123 = 1;

    if (Start123 == 1)
    {
      if (Btn1isSingle == 1)          //если 1я кнопка прожата 1 раз - возврат к текущему модулю
      {
        CurrentKey -= 1;
        t = (millis() - KeyDelay - 1);
      }

      if (Btn1.isDouble())          //если даблклик по 1й кнопке - возврат на один модуль назад
      {
        CurrentKey -= 2;
        t = (millis() - KeyDelay - 1);
      }

      if (millis() > (t + KeyDelay))
      {
        CurrentKey ++;

        if (CurrentKey == 0) CurrentKey = 3;

        if (CurrentKey == -1) CurrentKey = 2;

        if (CurrentKey >= 4) CurrentKey = 1;

        Keyboard.release('w');
        Keyboard.print(CharOfModule[CurrentKey]);
        Keyboard.press('w');
        Serial.print("1.активирован модуль ");
        Serial.println(CurrentKey);
        t = millis();
      }
    }

    if (Btn2.isSingle())
    {
      Serial.println("1.активирован модуль 4");
      Keyboard.release('w');
      Keyboard.print(CharOfModule[4]);
      Keyboard.press('w');
    }

    if (Btn2.isDouble())
    {
      Serial.println("1->3.переход в лениво аварийный режим");
      NextMode = 3;
    }
    
    //если 3сек зажата 1я кнопка - то перезапуск программы
    if (Btn1.isStep()) B1Time ++;
    if (Btn1.isRelease())B1Time = 0;
    if (B1Time == Btn1Hold)
    {
      Serial.println("1->0.перезапуск программы");
      NextMode = 4;
      WasReset = 0;
    }

    if (Mdl1.isSingle())
    {
      Serial.println("1.уничтожен модуль 1");
      MdlsLost[0] = 1;
      MdlClick[0] ++;
    }
    if (Mdl2.isSingle())
    {
      Serial.println("1.уничтожен модуль 2");
      MdlsLost[1] = 1;
      MdlClick[1] ++;
    }
    if (Mdl3.isSingle())
    {
      Serial.println("1.уничтожен модуль 3");
      MdlsLost[2] = 1;
      MdlClick[2] ++;
    }

    if (MdlsLost[0] == 1 || MdlsLost[1] == 1 || MdlsLost[2] == 1) //если какой-то модуль уничтожен
    {                                                             //то переход в аварийный режим
      Serial.println("1->2.переход в аварийный режим");
      NextMode = 2;
    }
  }
  Serial.println("1.отпускаю кнопку 'w'");
  Keyboard.release('w');
  banner = 0;                                                     //показать надпись о перезапуске
  return NextMode;
}

int EmerMode()           //Emergency Mode - аварийный режим. NextMode == 2
{
  CurrentKey = 1;
  Serial.println("2.нажимаю кнопку 'w'");
  Keyboard.press('w');

  for (short i = 0; i < 3; i ++)        //если модуль был уничтожен - то повторный клик по нему
  {                                     //говорит о том, что он исправен
    if (MdlsLost[i] == 1) MdlClick[i] = 1;
  }

  while (NextMode == 2)
  {
    Btn1.tick();
    Btn2.tick();
    Mdl1.tick();
    Mdl2.tick();
    Mdl3.tick();

    for (short i = 0; i < 3; i ++)
    {
      if (MdlsLost[i] == 1)digitalWrite(led[i], LOW);           //гашу светодиод неисправного модуля
      if (MdlsLost[i] == 0)digitalWrite(led[i], HIGH);          //зажигаю светодиод исправного модуля
    }

    if (Mdl1.isSingle())                        //проверка не была ли нажата кнопка модуля
    {
      MdlClick[0] ++;
      if (MdlClick[0] == 2) Serial.println("2.восстановлен модуль 1");
      else Serial.println("2.уничтожен модуль 1");
      MdlsLost[0] = 1;
    }
    
    if (Mdl2.isSingle())
    {
      MdlClick[1] ++;
      if (MdlClick[1] == 2) Serial.println("2.восстановлен модуль 2");
      else Serial.println("2.уничтожен модуль 2");
      MdlsLost[1] = 1;
    }
    if (Mdl3.isSingle())
    {
      MdlClick[2] ++;
      if (MdlClick[2] == 2) Serial.println("2.восстановлен модуль 3");
      else Serial.println("2.уничтожен модуль 3");
      MdlsLost[2] = 1;
    }

    for (short i = 0; i < 3; i ++)
    {
      if (MdlClick[i] == 2)   //если выбитый модуль прожимается ещё раз
      {                       //то он считается исправным
        MdlClick[i] = 1;
        MdlsLost[i] = 0;
      }
    }

    if (MdlsLost[0] == 0 && MdlsLost[1] == 0 && MdlsLost[2] == 0)   //если все модули восстановлены
    {                                                               //то переход в обычный режим
      NextMode = 1;
      WasReset = 1;
    }

    if (Btn1.isSingle())                     //управление модулями в ручном режиме
    {
      for (short i = 0; i < 2; i++)
      {
        for (int j = 0; j < 3; j++)
        {
          if (MdlsLost[CurrentKey - 1] == 0)   //если нажата 1я кнопка
          {                                    //то активируется живой модуль
            Serial.print("2.активирован модуль ");
            Serial.println(CurrentKey);
            Keyboard.release('w');
            Keyboard.print(CharOfModule[CurrentKey]);
            Keyboard.press('w');
            digitalWrite(led[i], HighBright);
            j = 3;
            i = 2;
          }
          CurrentKey ++;
          if (CurrentKey > 3) CurrentKey = 1;
        }
      }
    }

    //если 3сек зажата 1я кнопка - то перезапуск программы
    if (Btn1.isStep()) B1Time ++;
    if (Btn1.isRelease())B1Time = 0;
    if (B1Time == Btn1Hold)
    {
      Serial.println("2->0.перезапуск программы");
      NextMode = 4;
      WasReset = 0;
    }

    if (Btn2.isSingle())
    {
      Serial.println("2.активирован модуль 4");
      Keyboard.release('w');
      Keyboard.print('4');
      Keyboard.press('w');
    }

    if (Btn2.isDouble())
    {
      Serial.println("2->3.переход в лениво авариный режим");
      NextMode = 3;
    }
  }

  for (short i = 0; i < 3; i ++)MdlClick[i] = 0;

  banner = 0;
  return (NextMode);
}

void LazyEmerMode()       //Lazy Emergency Mode - ленивый аварийный режим. NextMode == 3
{
  bool LedIsOn = 0;                              //для мигания светодиодами
  unsigned long tled = millis();                 //для мигания светодиодами
  
  CurrentKey = 1;
  Serial.println("3.нажимаю кнопку 'w'");
  Keyboard.press('w');
  unsigned long t = millis();
  bool Stay = 1;

  //если модуль был уничтожен - считаем что по нему уже кликнул юзер. второй клик по модулю его восстаовит
  for (short i = 0; i < 3; i ++)      
  {                                     
    if (MdlsLost[i] == 1) MdlClick[i] = 1;
  }

  while (Stay == 1)
  {
    Btn1.tick();
    Btn2.tick();
    Mdl1.tick();
    Mdl2.tick();
    Mdl3.tick();

    if (Mdl1.isSingle())                        //проверка не была ли нажата кнопка модуля
    {
      MdlClick[0] += 1;
      Serial.println("3.уничтожен модуль 1");
      MdlsLost[0] = 1;
    }
    if (Mdl2.isSingle())
    {
      MdlClick[1] += 1;
      Serial.println("3.уничтожен модуль 2");
      MdlsLost[1] = 1;
    }
    if (Mdl3.isSingle())
    {
      MdlClick[2] += 1;
      Serial.println("3.уничтожен модуль 3");
      MdlsLost[2] = 1;
    }


    //если выбитый модуль прожимается ещё раз - то он считается восстановленным
    for (short i = 0; i < 3; i ++)
    {
      if (MdlClick[i] == 2)   
      {                       
        MdlClick[i] = 1;
        MdlsLost[i] = 0;
        digitalWrite(led[i], HIGH);
        Serial.print("3.восстановлен модуль ");
        Serial.println(i);
      }
    }

    if (Btn2.isDouble())
    {
      Serial.println("3->0.выход из лениво аварийного режима");
      Stay = 0;
    }

    //мигаем светодиодами исправных модулей
    if (millis() > (tled + LazyEmerBlink))
    {
      for (short i = 0; i < 3; i ++)
      {
        if (MdlsLost[i] != 1) digitalWrite(led[i], HIGH);
      }
      if (LedIsOn == 0)
      {
        Serial.println("3.светодиоды загорелись");
        LedIsOn = 1;
      }
    }
    if (millis() > (tled + (LazyEmerBlink * 2)))
    {
      for (short i = 0; i < 3; i ++)
      {
        digitalWrite(led[i], LOW);
      }
      Serial.println("3.светодиоды потухли");
      tled = millis();
      LedIsOn = 0;
    }

    if (millis() > (t + KeyDelay))
    {
      for (short i = 0; i < 2; i++)
      {
        for (int j = 0; j < 3; j++)
        {
          if (MdlsLost[CurrentKey - 1] == 0)
          {
            Serial.print("3.активирован модуль ");
            Serial.println(CurrentKey);
            Keyboard.release('w');
            Keyboard.print(CharOfModule[CurrentKey]);
            Keyboard.press('w');
            j = 3;
            i = 2;
          }
          CurrentKey ++;
          if (CurrentKey > 4) CurrentKey = 1;
        }
      }
      t = millis();
    }

    //если 3сек зажата 1я кнопка - то перезапуск программы
    if (Btn1.isStep())B1Time ++;
    if (Btn1.isRelease())B1Time = 0;
    if (B1Time == Btn1Hold)
    {
      Serial.println("3->0.перезапуск программы");
      Stay = 0;
      WasReset = 0;
    }
  }
  NextMode = 1;
  banner = 0;
}
